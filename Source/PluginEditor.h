
#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Parameters.h"
#include "RotaryKnob.h"
#include "LookAndFeel.h"

class DelayAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    DelayAudioProcessorEditor (DelayAudioProcessor&);
    ~DelayAudioProcessorEditor() override;
    
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    DelayAudioProcessor& audioProcessor;
    RotaryKnob gainKnob {"Gain", *audioProcessor.getApvts(), gainParamID, true};
    RotaryKnob mixKnob {"Mix", *audioProcessor.getApvts(), mixParamID};
    RotaryKnob delayTimeLKnob {"Time (L, M)", *audioProcessor.getApvts(), delayTimeLParamID};
    RotaryKnob delayTimeRKnob {"Time (R)", *audioProcessor.getApvts(), delayTimeRParamID};
    RotaryKnob feedbackKnob {"Feedback", *audioProcessor.getApvts(), feedbackParamID, true};
    RotaryKnob stereoKnob {"Stereo", *audioProcessor.getApvts(), stereoParamID, true};
    RotaryKnob lowCutKnob {"Low Cut", *audioProcessor.getApvts(), lowCutParamID};
    RotaryKnob highCutKnob {"High Cut", *audioProcessor.getApvts(), highCutParamID};
    juce::GroupComponent delayGroup, feedbackGroup, outputGroup;
    MainLookAndFeel mainLF;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayAudioProcessorEditor)
};
