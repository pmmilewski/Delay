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
const juce::ParameterID delayTimeParamID {"delayTime", 1};

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
    static constexpr float minDelayTime = 5.0f;
    static constexpr float maxDelayTime = 5000.0f;
    float delayTime = 0.0f;

private:
    juce::AudioParameterFloat* gainParam = { nullptr };
    juce::LinearSmoothedValue<float> gainSmoother = { 0.0f };

    juce::AudioParameterFloat* delayTimeParam = { nullptr };
    float targetDelayTime = 0.0f;
    float coeff = 0.0f; // one-pole smoothing
};
