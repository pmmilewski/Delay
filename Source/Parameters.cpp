/*
  ==============================================================================

    Parameters.cpp
    Created: 20 Mar 2025 7:38:03pm
    Author:  PM

  ==============================================================================
*/

#include "Parameters.h"

template<typename T>
static void castParameter(juce::AudioProcessorValueTreeState& apvts,
                          const juce::ParameterID& id, T& destination)
{
  destination = dynamic_cast<T>(apvts.getParameter(id.getParamID()));
  jassert(destination != nullptr);
}

static juce::String stringFromMilliseconds(float value, int)
{
  if (value < 10.0f)
  {
    return juce::String(value, 2) + " ms";
  }

  if (value < 100.0f)
  {
    return juce::String(value, 1) + " ms";
  }

  if (value < 1000.0f)
  {
    return juce::String(static_cast<int>(value)) + " ms";
  }
  
  return juce::String(value * 0.001f, 2) + " s";
}

static juce::String stringFromDecibels(float value, int)
{
  return juce::String(value, 1) + " dB";
}

static juce::String stringFromPercent(float value, int)
{
  return juce::String(static_cast<int>(value)) + " %";
}

static float millisecondsFromString(const juce::String& text)
{
  float value = text.getFloatValue();

  if (!text.endsWithIgnoreCase("ms"))
  {
    if (text.endsWithIgnoreCase("s") || value < Parameters::minDelayTime)
    {
      return value * 1000.0f;
    }
  }

  return value;
}

Parameters::Parameters(juce::AudioProcessorValueTreeState& apvts)
{
  castParameter(apvts, gainParamID, gainParam);
  castParameter(apvts, delayTimeParamID, delayTimeParam);
  castParameter(apvts, mixParamID, mixParam);
}

juce::AudioProcessorValueTreeState::ParameterLayout Parameters::createParameterLayout()
{
  juce::AudioProcessorValueTreeState::ParameterLayout parameterLayout{};
  parameterLayout.add(std::make_unique<juce::AudioParameterFloat>(
      gainParamID,
      "Output Gain",
      juce::NormalisableRange<float> {-12.0f, 12.0f},
      0.0f,
      juce::AudioParameterFloatAttributes().withStringFromValueFunction(stringFromDecibels)
      ));

  parameterLayout.add(std::make_unique<juce::AudioParameterFloat>(delayTimeParamID,
    "Delay Time",
    juce::NormalisableRange<float> {minDelayTime, maxDelayTime, 0.001f, 0.25f},
    100.0f,
    juce::AudioParameterFloatAttributes()
    .withStringFromValueFunction(stringFromMilliseconds)
    .withValueFromStringFunction(millisecondsFromString)
    ));

  parameterLayout.add(std::make_unique<juce::AudioParameterFloat>(
    mixParamID,
    "Mix",
    juce::NormalisableRange<float> {0.0f, 100.0f, 1.0f},
    100.0f,
    juce::AudioParameterFloatAttributes().withStringFromValueFunction(stringFromPercent)
    ));

  return parameterLayout;
}

void Parameters::update() noexcept
{
  gainSmoother.setTargetValue(juce::Decibels::decibelsToGain(gainParam->get()));

  targetDelayTime = delayTimeParam->get();
  if (delayTime == 0.0f)
  {
    delayTime = targetDelayTime;
  }
  mixSmoother.setTargetValue(mixParam->get() * 0.01f);
}

void Parameters::prepareToPlay(double sampleRate) noexcept
{
  double duration = 0.02;
  gainSmoother.reset(sampleRate, duration);
  coeff = 1.0f - std::exp(-1.0f / (0.2f * static_cast<float>(sampleRate)));
  mixSmoother.reset(sampleRate, duration);
}

void Parameters::reset() noexcept
{
  gain = 0.0f;
  gainSmoother.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(gainParam->get()));
  delayTime = 0.0f;
  mix = 1.0f;
  mixSmoother.setCurrentAndTargetValue(mixParam->get() * 0.01f);
}

void Parameters::smoothen() noexcept
{
  gain = gainSmoother.getNextValue();
  delayTime += (targetDelayTime - delayTime) * coeff;
  mix = mixSmoother.getNextValue();
}
