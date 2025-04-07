/*
  ==============================================================================

    Parameters.h
    Created: 20 Mar 2025 7:38:03pm
    Author:  PM

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

const juce::ParameterID gainParamID {"gain", 1};
const juce::ParameterID delayTimeLParamID {"delayTimeL", 1};
const juce::ParameterID delayTimeRParamID {"delayTimeR", 1};
const juce::ParameterID mixParamID {"mix", 1};
const juce::ParameterID feedbackParamID {"feedback", 1};
const juce::ParameterID stereoParamID {"stereo", 1};
const juce::ParameterID lowCutParamID {"lowCut", 1};
const juce::ParameterID highCutParamID {"highCut", 1};
const juce::ParameterID lowCutQParamID {"lowCutQ", 1};
const juce::ParameterID highCutQParamID {"highCutQ", 1};
const juce::ParameterID driveParamID {"drive", 1};

class Parameters
{
public:
    Parameters(juce::AudioProcessorValueTreeState& apvts);
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void update() noexcept;
    void prepareToPlay(double sampleRate) noexcept;
    void reset() noexcept;
    void smoothen() noexcept;
    
    float gain = { 0.0f };
    static constexpr float minDelayTime = {5.0f};
    static constexpr float maxDelayTime = {5000.0f};
    float delayTimeL = {0.0f};
    float delayTimeR = {0.0f};
    float mix = {1.0f};
    float feedback = {0.0f};
    float stereo = {0.0f};
    float panL = {0.0f};
    float panR = {1.0f};
    float lowCut = {20.0f};
    float highCut = {20000.0f};
    float lowCutQ = {0.707f};
    float highCutQ = {0.707f};
    float drive = {0.0f};

private:
    juce::AudioParameterFloat* gainParam = { nullptr };
    juce::LinearSmoothedValue<float> gainSmoother = { 0.0f };

    juce::AudioParameterFloat* delayTimeLParam = { nullptr };
    float targetDelayTimeL = {0.0f};
    float coeffL = {0.0f}; // one-pole smoothing

    juce::AudioParameterFloat* delayTimeRParam = { nullptr };
    float targetDelayTimeR = {0.0f};
    float coeffR = {0.0f}; // one-pole smoothing

    juce::AudioParameterFloat* mixParam = { nullptr };
    juce::LinearSmoothedValue<float> mixSmoother = { 0.0f };

    juce::AudioParameterFloat* feedbackParam = { nullptr };
    juce::LinearSmoothedValue<float> feedbackSmoother = { 0.0f };

    juce::AudioParameterFloat* stereoParam = { nullptr };
    juce::LinearSmoothedValue<float> stereoSmoother = { 0.0f };

    juce::AudioParameterFloat* lowCutParam = { nullptr };
    juce::LinearSmoothedValue<float> lowCutSmoother = { 0.0f };

    juce::AudioParameterFloat* highCutParam = { nullptr };
    juce::LinearSmoothedValue<float> highCutSmoother = { 0.0f };

    juce::AudioParameterFloat* lowCutQParam = { nullptr };
    juce::LinearSmoothedValue<float> lowCutQSmoother = { 0.0f };

    juce::AudioParameterFloat* highCutQParam = { nullptr };
    juce::LinearSmoothedValue<float> highCutQSmoother = { 0.0f };

    juce::AudioParameterFloat* driveParam = { nullptr };
    juce::LinearSmoothedValue<float> driveSmoother = { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Parameters)
};
