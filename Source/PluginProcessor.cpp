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

    levelL.reset();
    levelR.reset();

#if CROSSFADE
    delayInSamplesL = 0.0f;
    delayInSamplesR = 0.0f;
    targetDelayL = 0.0f;
    targetDelayR = 0.0f;
    xfadeL = 0.0f;
    xfadeR = 0.0f;
    xfadeInc = static_cast<float>(1.0 / (0.05 * sampleRate)); // 50 ms
#endif
#if DUCKING
    delayInSamplesL = 0.0f;
    delayInSamplesR = 0.0f;

    fadeL = 1.0f;
    fadeTargetL = 1.0f;
    waitL = 0.0f;

    fadeR = 1.0f;
    fadeTargetR = 1.0f;
    waitR = 0.0f;

    waitInc = 1.0f / (0.3f * static_cast<float>(sampleRate)); // 300 ms
    coeff = 1.0f - std::exp(-1.0f / (0.05f * static_cast<float>(sampleRate))); // 50 ms to 63.2%
#endif
    lastBypass = false;
    bypassXfade = 0.0f;
    bypassXfadeInc = static_cast<float>(1.0 / (0.05 * sampleRate)); // 50 ms
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

#if CROSSFADE
            if (xfadeL == 0.0f)
            {
                float delayTimeL = params.tempoSync ? syncedTimeL : params.delayTimeL;
                targetDelayL = delayTimeL / 1000.0f * sampleRate;

                if (delayInSamplesL == 0.0f)
                {
                    delayInSamplesL = targetDelayL;
                }
                else if (targetDelayL != delayInSamplesL)
                {
                    xfadeL = xfadeInc;
                }
            }

            if (xfadeR == 0.0f)
            {
                float delayTimeR = params.tempoSync ? syncedTimeR : params.delayTimeR;
                targetDelayR = delayTimeR / 1000.0f * sampleRate;

                if (delayInSamplesR == 0.0f)
                {
                    delayInSamplesR = targetDelayR;
                }
                else if (targetDelayR != delayInSamplesR)
                {
                    xfadeR = xfadeInc;
                }
            }
#elif DUCKING
            float delayTimeL = params.tempoSync ? syncedTimeL : params.delayTimeL;
            float newTargetDelayL = delayTimeL / 1000.0f * sampleRate;

            if (newTargetDelayL != targetDelayL)
            {
                targetDelayL = newTargetDelayL;

                if (delayInSamplesL == 0.0f)
                {
                    delayInSamplesL = targetDelayL;
                }
                else
                {
                    waitL = waitInc;
                    fadeTargetL = 0.0f;
                }
            }

            float delayTimeR = params.tempoSync ? syncedTimeR : params.delayTimeR;
            float newTargetDelayR = delayTimeR / 1000.0f * sampleRate;

            if (newTargetDelayR != targetDelayR)
            {
                targetDelayR = newTargetDelayR;

                if (delayInSamplesR == 0.0f)
                {
                    delayInSamplesR = targetDelayR;
                }
                else
                {
                    waitR = waitInc;
                    fadeTargetR = 0.0f;
                }
            }
#else
            float delayTimeL = params.tempoSync ? syncedTimeL : params.delayTimeL;
            float delayInSamplesL = delayTimeL / 1000.0f * sampleRate;

            float delayTimeR = params.tempoSync ? syncedTimeR : params.delayTimeR;
            float delayInSamplesR = delayTimeR / 1000.0f * sampleRate; 
#endif

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

#if CROSSFADE
            if (xfadeL > 0.0f)
            {
                float newL = delayLineL.read(targetDelayL);

                wetL = (1.0f - xfadeL) * wetL + xfadeL * newL;

                xfadeL += xfadeInc;
                if (xfadeL >= 1.0f)
                {
                    delayInSamplesL = targetDelayL;
                    xfadeL = 0.0f;
                }
            }

            if (xfadeR > 0.0f)
            {
                float newR = delayLineR.read(targetDelayR);

                wetR = (1.0f - xfadeR) * wetR + xfadeR * newR;

                xfadeR += xfadeInc;
                if (xfadeR >= 1.0f)
                {
                    delayInSamplesR = targetDelayR;
                    xfadeR = 0.0f;
                }
            }
#endif
#if DUCKING
            fadeL += (fadeTargetL - fadeL) * coeff;

            wetL *= fadeL;

            if (waitL > 0.0f)
            {
                waitL += waitInc;
                if (waitL >= 1.0f)
                {
                    delayInSamplesL = targetDelayL;
                    waitL = 0.0f;
                    fadeTargetL = 1.0f; // fade in
                }
            }

            fadeR += (fadeTargetR - fadeR) * coeff;

            wetR *= fadeR;

            if (waitR > 0.0f)
            {
                waitR += waitInc;
                if (waitR >= 1.0f)
                {
                    delayInSamplesR = targetDelayR;
                    waitR = 0.0f;
                    fadeTargetR = 1.0f; // fade in
                }
            }
#endif

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

            float postGainL = mixL * params.gain;
            float postGainR = mixR * params.gain;

            float outL = postGainL;
            float outR = postGainR;

            if (params.bypassed != lastBypass)
            {
                lastBypass = params.bypassed;
                bypassXfade = bypassXfadeInc;
            }

            if (params.bypassed)
            {
                if (bypassXfade > 0.0f)
                {
                    outL = (1.0f - bypassXfade) * postGainL + dryL * bypassXfade;
                    outR = (1.0f - bypassXfade) * postGainR + dryR * bypassXfade;

                    bypassXfade += bypassXfadeInc;
                    if (bypassXfade >= 1.0f)
                    {
                        bypassXfade = 0.0f;
                    }
                }
                else
                {
                    outL = dryL;
                    outR = dryR;
                }
            }
            else
            {
                if (bypassXfade > 0.0f)
                {
                    outL = (1.0f - bypassXfade) * dryL + postGainL * bypassXfade;
                    outR = (1.0f - bypassXfade) * dryR + postGainR * bypassXfade;

                    bypassXfade += bypassXfadeInc;
                    if (bypassXfade >= 1.0f)
                    {
                        bypassXfade = 0.0f;
                    }
                }
            }

            outputDataL[sample] = outL;
            outputDataR[sample] = outR;

            maxL = std::max(maxL, std::abs(outL));
            maxR = std::max(maxR, std::abs(outR));
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

            float postGain = mix * params.gain;
            float out = postGain;

            if (params.bypassed != lastBypass)
            {
                lastBypass = params.bypassed;
                bypassXfade = bypassXfadeInc;
            }

            if (params.bypassed)
            {
                if (bypassXfade > 0.0f)
                {
                    out = (1.0f - bypassXfade) * postGain + dry * bypassXfade;

                    bypassXfade += bypassXfadeInc;
                    if (bypassXfade >= 1.0f)
                    {
                        bypassXfade = 0.0f;
                    }
                }
                else
                {
                    out = dry;
                }
            }
            else
            {
                if (bypassXfade > 0.0f)
                {
                    out = (1.0f - bypassXfade) * dry + postGain * bypassXfade;

                    bypassXfade += bypassXfadeInc;
                    if (bypassXfade >= 1.0f)
                    {
                        bypassXfade = 0.0f;
                    }
                }
            }

            outputDataL[sample] = out;
            maxL = std::max(maxL, std::abs(out));
            maxR = std::max(maxL, std::abs(out));
        }
    }

    levelL.updateIfGreater(maxL);
    levelR.updateIfGreater(maxR);

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

juce::AudioProcessorParameter* DelayAudioProcessor::getBypassParameter() const
{
    return params.bypassParam;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DelayAudioProcessor();
}
