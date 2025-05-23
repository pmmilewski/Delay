#pragma once

#include <JuceHeader.h>
#include "DelayLine.h"
#include "Parameters.h"
#include "Tempo.h"
#include "Measurement.h"
#include "Defines.h"

//==============================================================================
/**
*/
class DelayAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    DelayAudioProcessor();
    ~DelayAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    auto getApvts() {return &apvts;}
    auto getParams() {return &params;}
    juce::AudioProcessorParameter* getBypassParameter() const override;

    Measurement levelL, levelR;

private:
    juce::AudioProcessorValueTreeState apvts { *this, nullptr, "Parameters", Parameters::createParameterLayout() };
    Parameters params;
    DelayLine delayLineL, delayLineR;
    float feedbackL = 0.0f;
    float feedbackR = 0.0f;
    juce::dsp::StateVariableTPTFilter<float> lowCutFilter;
    juce::dsp::StateVariableTPTFilter<float> highCutFilter;
    juce::dsp::WaveShaper<float> distortionWaveShaper;
    float lastLowCut = -1.0f;
    float lastHighCut = -1.0f;
    float lastLowCutQ = -1.0f;
    float lastHighCutQ = -1.0f;
    Tempo tempo;

#if CROSSFADE
    float delayInSamplesL = 0.0f;
    float delayInSamplesR = 0.0f;
    float targetDelayL = 0.0f;
    float targetDelayR = 0.0f;
    float xfadeL = 0.0f;
    float xfadeR = 0.0f;
    float xfadeInc = 0.0f;
#endif
#if DUCKING
    float delayInSamplesL = 0.0f;
    float delayInSamplesR = 0.0f;
    float targetDelayL = 0.0f;
    float targetDelayR = 0.0f;

    float fadeL = 0.0f;
    float fadeTargetL = 0.0f;
    float waitL = 0.0f;

    float fadeR = 0.0f;
    float fadeTargetR = 0.0f;
    float waitR = 0.0f;

    float waitInc = 0.0f;
    float coeff = 0.0f;

    bool lastBypass = false;
    float bypassXfade = 0.0f;
    float bypassXfadeInc = 0.0f;
#endif

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayAudioProcessor)
};
