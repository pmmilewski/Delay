/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"

#include <algorithm>
#include "PluginEditor.h"
#include "ProtectYourEars.h"

//==============================================================================
DelayAudioProcessor::DelayAudioProcessor()
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                       ),
    params(apvts)
{
    lowCutFilter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    highCutFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    distortionWaveShaper.functionToUse = [] (float x) {
        return std::tanh(x);
    };
}

DelayAudioProcessor::~DelayAudioProcessor()
{
}

//==============================================================================
const juce::String DelayAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DelayAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DelayAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DelayAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DelayAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DelayAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DelayAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DelayAudioProcessor::setCurrentProgram ([[maybe_unused]] int index)
{
}

const juce::String DelayAudioProcessor::getProgramName ([[maybe_unused]] int index)
{
    return {};
}

void DelayAudioProcessor::changeProgramName ([[maybe_unused]] int index, [[maybe_unused]] const juce::String& newName)
{
}

//==============================================================================
void DelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    params.prepareToPlay(sampleRate);
    params.reset();

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;

    double numSamples = Parameters::maxDelayTime / 1000.0 * sampleRate;
    int maxDelayInSamples = static_cast<int>(std::ceil(numSamples));
    delayLineL.setMaximumDelayInSamples(maxDelayInSamples);
    delayLineR.setMaximumDelayInSamples(maxDelayInSamples);
    delayLineL.reset();
    delayLineR.reset();

    feedbackL = 0.0f;
    feedbackR = 0.0f;

    lowCutFilter.prepare(spec);
    lowCutFilter.reset();
    lastLowCut = -1.0f;
    lastLowCutQ = -1.0f;

    highCutFilter.prepare(spec);
    highCutFilter.reset();
    lastHighCut = -1.0f;
    lastHighCutQ = -1.0f;

    distortionWaveShaper.prepare(spec);
    distortionWaveShaper.reset();

    tempo.reset();

    levelL.store(0.0f);
    levelR.store(0.0f);
}

void DelayAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mono = juce::AudioChannelSet::mono();
    const auto stereo = juce::AudioChannelSet::stereo();
    const auto mainIn = layouts.getMainInputChannelSet();
    const auto mainOut = layouts.getMainOutputChannelSet();

    //DBG("isBusesLayoutSupported, in: " << mainIn.getDescription() << ", out: " << mainOut.getDescription());

    if (mainIn == mono && mainOut == mono) { return true; }
    if (mainIn == mono && mainOut == stereo) { return true; }
    if (mainIn == stereo && mainOut == stereo) { return true; }

    return false;
}
#endif

void DelayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, [[maybe_unused]] juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    {
        buffer.clear (i, 0, buffer.getNumSamples());
    }

    params.update();
    tempo.update(getPlayHead());

    float syncedTimeL = static_cast<float>(tempo.getMillisecondsForNoteLength(params.delayNoteL));
    syncedTimeL = std::min(syncedTimeL, Parameters::maxDelayTime);
    float syncedTimeR = static_cast<float>(tempo.getMillisecondsForNoteLength(params.delayNoteR));
    syncedTimeR = std::min(syncedTimeR, Parameters::maxDelayTime);

    float sampleRate = static_cast<float>(getSampleRate());

    auto mainInput = getBusBuffer(buffer, true, 0);
    auto mainInputChannels = mainInput.getNumChannels();
    auto isMainInputStereo = mainInputChannels > 1;
    const float* inputDataL = mainInput.getReadPointer(0);
    const float* inputDataR = mainInput.getReadPointer(isMainInputStereo ? 1 : 0);
    
    auto mainOutput = getBusBuffer(buffer, false, 0);
    auto mainOutputChannels = mainOutput.getNumChannels();
    auto isMainOutputStereo = mainOutputChannels > 1;
    float* outputDataL = mainOutput.getWritePointer(0);
    float* outputDataR = mainOutput.getWritePointer(isMainOutputStereo ? 1 : 0);

    float maxL = 0.0f;
    float maxR = 0.0f;
    
    if (isMainInputStereo)
    {
        for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            params.smoothen();

            float delayTimeL = params.tempoSync? syncedTimeL : params.delayTimeL;
            float delayTimeR = params.tempoSync? syncedTimeR : params.delayTimeR;
            
            float delayInSamplesL = delayTimeL / 1000.0f * sampleRate;
            float delayInSamplesR = delayTimeR / 1000.0f * sampleRate;

            if (params.lowCut != lastLowCut)
            {
                lowCutFilter.setCutoffFrequency(params.lowCut);
                lastLowCut = params.lowCut;
            }

            if (params.lowCutQ != lastLowCutQ)
            {
                lowCutFilter.setResonance(params.lowCutQ);
                lastLowCutQ = params.lowCutQ;
            }

            if (params.highCut != lastHighCut)
            {
                highCutFilter.setCutoffFrequency(params.highCut);
                lastHighCut = params.highCut;
            }

            if (params.highCutQ != lastHighCutQ)
            {
                highCutFilter.setResonance(params.highCutQ);
                lastHighCutQ = params.highCutQ;
            }
            
            float dryL = inputDataL[sample];
            float dryR = inputDataR[sample];

            float mono = (dryL + dryR) * 0.5f;

            delayLineL.write(mono*params.panL + feedbackR);
            delayLineR.write(mono*params.panR + feedbackL);

            float wetL = delayLineL.read(delayInSamplesL);
            float wetR = delayLineR.read(delayInSamplesR);

            feedbackL = wetL * params.feedback;
            feedbackL = lowCutFilter.processSample(0, feedbackL);
            feedbackL = distortionWaveShaper.processSample(params.drive * feedbackL) * params.postWSGain;
            feedbackL = highCutFilter.processSample(0, feedbackL);

            feedbackR = wetR * params.feedback;
            feedbackR = lowCutFilter.processSample(1, feedbackR);
            feedbackR = distortionWaveShaper.processSample(params.drive * feedbackR) * params.postWSGain;
            feedbackR = highCutFilter.processSample(1, feedbackR);

            float mixL = (1.0f - params.mix) * dryL + wetL * params.mix;
            float mixR = (1.0f - params.mix) * dryR + wetR * params.mix;

            float outL = mixL * params.gain;
            float outR = mixR * params.gain;
            
            outputDataL[sample] = outL;
            outputDataR[sample] = outR;

            maxL = std::max(maxL, std::abs(outL));
            maxL = std::max(maxR, std::abs(outR));
        }
    }
    else
    {
        for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            params.smoothen();

            float delayInSamples = params.delayTimeL / 1000.0f * sampleRate;
            
            float dry = inputDataL[sample];
            delayLineL.write(dry + feedbackL);

            float wet = delayLineL.read(delayInSamples);
            feedbackL = wet * params.feedback;

            float mix = (1.0f - params.mix) * dry + wet * params.mix;

            float outL = mix * params.gain;
            outputDataL[sample] = outL;
            maxL = std::max(maxL, std::abs(outL));
        }
    }

    levelL.store(maxL);
    levelR.store(maxR);

#if JUCE_DEBUG
    protectYourEars(buffer);
#endif
}

//==============================================================================
bool DelayAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DelayAudioProcessor::createEditor()
{
    return new DelayAudioProcessorEditor (*this);
}

//==============================================================================
void DelayAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    copyXmlToBinary(*apvts.copyState().createXml(), destData);
    //DBG(apvts.copyState().toXmlString());
}

void DelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DelayAudioProcessor();
}
