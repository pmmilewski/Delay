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

private:
    juce::AudioParameterFloat* gainParam = { nullptr };
    juce::LinearSmoothedValue<float> gainSmoother = { 0.0f };
};
