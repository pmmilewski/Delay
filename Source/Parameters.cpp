#include "Parameters.h"
#include "DSP.h"

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

static juce::String stringFromHz(float value, int)
{
  if (value < 1000.0f)
  {
    return juce::String(value) + " Hz";
  }
  
  if (value < 10000.0f)
  {
    return juce::String(value / 1000.0f, 2) + " kHz";
  }

  return juce::String(value / 1000.0f, 1) + " kHz";
}

static float hzFromString(const juce::String& str)
{
  float value = str.getFloatValue();
  if (value < 20.0f)
  {
    return value * 1000.0f;
  }
  
  return value;  
}

Parameters::Parameters(juce::AudioProcessorValueTreeState& apvts)
{
  castParameter(apvts, gainParamID, gainParam);
  castParameter(apvts, delayTimeLParamID, delayTimeLParam);
  castParameter(apvts, delayTimeRParamID, delayTimeRParam);
  castParameter(apvts, mixParamID, mixParam);
  castParameter(apvts, feedbackParamID, feedbackParam);
  castParameter(apvts, stereoParamID, stereoParam);
  castParameter(apvts, lowCutParamID, lowCutParam);
  castParameter(apvts, highCutParamID, highCutParam);
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

  parameterLayout.add(std::make_unique<juce::AudioParameterFloat>(delayTimeLParamID,
    "Delay Time (L)",
    juce::NormalisableRange<float> {minDelayTime, maxDelayTime, 0.001f, 0.25f},
    100.0f,
    juce::AudioParameterFloatAttributes()
    .withStringFromValueFunction(stringFromMilliseconds)
    .withValueFromStringFunction(millisecondsFromString)
    ));

  parameterLayout.add(std::make_unique<juce::AudioParameterFloat>(delayTimeRParamID,
  "Delay Time (R)",
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

  parameterLayout.add(std::make_unique<juce::AudioParameterFloat>(
    feedbackParamID,
    "Feedback",
    juce::NormalisableRange<float> {-100.0f, 100.0f, 1.0f},
    0.0f,
    juce::AudioParameterFloatAttributes().withStringFromValueFunction(stringFromPercent)
    ));

  parameterLayout.add(std::make_unique<juce::AudioParameterFloat>(
  stereoParamID,
  "Stereo",
  juce::NormalisableRange<float> {-100.0f, 100.0f, 1.0f},
  0.0f,
  juce::AudioParameterFloatAttributes().withStringFromValueFunction(stringFromPercent)
  ));

  parameterLayout.add(std::make_unique<juce::AudioParameterFloat>(
    lowCutParamID,
    "Low Cut",
    juce::NormalisableRange<float> {20.0f, 20000.0f, 1.0f, 0.3f},
    20.0f,
    juce::AudioParameterFloatAttributes()
    .withStringFromValueFunction(stringFromHz)
    .withValueFromStringFunction(hzFromString)
    ));

  parameterLayout.add(std::make_unique<juce::AudioParameterFloat>(
  highCutParamID,
  "High Cut",
  juce::NormalisableRange<float> {20.0f, 20000.0f, 1.0f, 0.3f},
  20000.0f,
  juce::AudioParameterFloatAttributes()
  .withStringFromValueFunction(stringFromHz)
  .withValueFromStringFunction(hzFromString)
  ));

  return parameterLayout;
}

void Parameters::update() noexcept
{
  gainSmoother.setTargetValue(juce::Decibels::decibelsToGain(gainParam->get()));

  targetDelayTimeL = delayTimeLParam->get();
  if (delayTimeL == 0.0f)
  {
    delayTimeL = targetDelayTimeL;
  }

  targetDelayTimeR = delayTimeRParam->get();
  if (delayTimeR == 0.0f)
  {
    delayTimeR = targetDelayTimeR;
  }
  
  mixSmoother.setTargetValue(mixParam->get() * 0.01f);
  feedbackSmoother.setTargetValue(feedbackParam->get() * 0.01f);
  stereoSmoother.setTargetValue(stereoParam->get() * 0.01f);
  lowCutSmoother.setTargetValue(lowCutParam->get());
  highCutSmoother.setTargetValue(highCutParam->get());
}

void Parameters::prepareToPlay(double sampleRate) noexcept
{
  double duration = 0.02;
  gainSmoother.reset(sampleRate, duration);
  coeffL = 1.0f - std::exp(-1.0f / (0.2f * static_cast<float>(sampleRate)));
  coeffR = 1.0f - std::exp(-1.0f / (0.2f * static_cast<float>(sampleRate)));
  mixSmoother.reset(sampleRate, duration);
  feedbackSmoother.reset(sampleRate, duration);
  stereoSmoother.reset(sampleRate, duration);
  lowCutSmoother.reset(sampleRate, duration);
  highCutSmoother.reset(sampleRate, duration);
}

void Parameters::reset() noexcept
{
  gain = 0.0f;
  gainSmoother.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(gainParam->get()));
  delayTimeL = 0.0f;
  delayTimeR = 0.0f;
  mix = 1.0f;
  mixSmoother.setCurrentAndTargetValue(mixParam->get() * 0.01f);
  feedbackSmoother.setCurrentAndTargetValue(feedbackParam->get() * 0.01f);
  stereoSmoother.setCurrentAndTargetValue(stereoParam->get() * 0.01f);
  panL = 0.0f;
  panR = 1.0f;
  lowCut = 20.0f;
  lowCutSmoother.setCurrentAndTargetValue(lowCutParam->get());
  highCut = 20000.0f;
  highCutSmoother.setCurrentAndTargetValue(highCutParam->get());  
}

void Parameters::smoothen() noexcept
{
  gain = gainSmoother.getNextValue();
  delayTimeL += (targetDelayTimeL - delayTimeL) * coeffL;
  delayTimeR += (targetDelayTimeR - delayTimeR) * coeffR;
  mix = mixSmoother.getNextValue();
  feedback = feedbackSmoother.getNextValue();
  panningEqualPower(stereoSmoother.getNextValue(), panL, panR);
  lowCut = lowCutSmoother.getNextValue();
  highCut = highCutSmoother.getNextValue();
}
