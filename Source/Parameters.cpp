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
  castParameter(apvts, lowCutQParamID, lowCutQParam);
  castParameter(apvts, highCutQParamID, highCutQParam);
  castParameter(apvts, driveParamID, driveParam);
  castParameter(apvts, postWSGainParamID, postWSGainParam);
  castParameter(apvts, delayNoteParamID, delayNoteParam);
  castParameter(apvts, tempoSyncParamID, tempoSyncParam);
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
    50.0f,
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

  parameterLayout.add(std::make_unique<juce::AudioParameterFloat>(
  lowCutQParamID,
  "Low Cut Q",
  juce::NormalisableRange<float> {0.5f, 10.0f, 0.1f},
  0.707f
  ));

  parameterLayout.add(std::make_unique<juce::AudioParameterFloat>(
  highCutQParamID,
  "High Cut Q",
  juce::NormalisableRange<float> {0.5f, 10.0f, 0.1f},
  0.707f
  ));
  
  parameterLayout.add(std::make_unique<juce::AudioParameterFloat>(
  driveParamID,
  "Dist Pre-gain",
  juce::NormalisableRange<float> {-24.0f, 24.0f},
  0.0f,
  juce::AudioParameterFloatAttributes().withStringFromValueFunction(stringFromDecibels)
  ));

  parameterLayout.add(std::make_unique<juce::AudioParameterFloat>(
  postWSGainParamID,
  "Dist Post-gain",
  juce::NormalisableRange<float> {-24.0f, 24.0f},
  0.0f,
  juce::AudioParameterFloatAttributes().withStringFromValueFunction(stringFromDecibels)
  ));

  parameterLayout.add(std::make_unique<juce::AudioParameterBool>(
    tempoSyncParamID, "Tempo Sync", false));

  juce::StringArray noteLengths = {
    "1/32",
    "1/16 trip",
    "1/32 dot",
    "1/16",
    "1/8 trip",
    "1/16 dot",
    "1/8",
    "1/4 trip",
    "1/8 dot",
    "1/4",
    "1/2 trip",
    "1/4 dot",
    "1/2",
    "1/1 trip",
    "1/2 dot",
    "1/1"
  };

  parameterLayout.add(std::make_unique<juce::AudioParameterChoice>(
    delayNoteParamID, "Delay Note", noteLengths, 9));

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
  lowCutQSmoother.setTargetValue(lowCutQParam->get());
  highCutQSmoother.setTargetValue(highCutQParam->get());
  driveSmoother.setTargetValue(juce::Decibels::decibelsToGain(driveParam->get()));
  postWSGainSmoother.setTargetValue(juce::Decibels::decibelsToGain(postWSGainParam->get()));
  delayNote = delayNoteParam->getIndex();
  tempoSync = tempoSyncParam->get();
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
  lowCutQSmoother.reset(sampleRate, duration);
  highCutQSmoother.reset(sampleRate, duration);
  driveSmoother.reset(sampleRate, duration);
  postWSGainSmoother.reset(sampleRate, duration);
}

void Parameters::reset() noexcept
{
  gain = 0.0f;
  gainSmoother.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(gainParam->get()));
  delayTimeL = 0.0f;
  delayTimeR = 0.0f;
  mix = 0.5f;
  mixSmoother.setCurrentAndTargetValue(mixParam->get() * 0.01f);
  feedbackSmoother.setCurrentAndTargetValue(feedbackParam->get() * 0.01f);
  stereoSmoother.setCurrentAndTargetValue(stereoParam->get() * 0.01f);
  panL = 0.0f;
  panR = 1.0f;
  lowCut = 20.0f;
  lowCutSmoother.setCurrentAndTargetValue(lowCutParam->get());
  highCut = 20000.0f;
  highCutSmoother.setCurrentAndTargetValue(highCutParam->get());
  lowCutQ = 0.707f;
  lowCutQSmoother.setCurrentAndTargetValue(lowCutQParam->get());
  highCutQ = 0.707f;
  highCutQSmoother.setCurrentAndTargetValue(highCutQParam->get());
  drive = 0.0f;
  driveSmoother.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(driveParam->get()));
  postWSGainSmoother.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(postWSGainParam->get()));
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
  lowCutQ = lowCutQSmoother.getNextValue();
  highCutQ = highCutQSmoother.getNextValue();
  drive = driveSmoother.getNextValue();
  postWSGain = postWSGainSmoother.getNextValue();
}
