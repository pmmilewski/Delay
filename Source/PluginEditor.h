
#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Parameters.h"
#include "RotaryKnob.h"
#include "LookAndFeel.h"
#include "LevelMeter.h"

class DelayAudioProcessorEditor  : public juce::AudioProcessorEditor, private juce::AudioProcessorParameter::Listener
{
public:
    DelayAudioProcessorEditor (DelayAudioProcessor&);
    ~DelayAudioProcessorEditor() override;
    
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void parameterValueChanged(int, float value) override;
    void parameterGestureChanged(int, bool) override { }
    void updateDelayKnobs(bool tempoSyncActive);
    
    DelayAudioProcessor& audioProcessor;
    RotaryKnob gainKnob {"Gain", *audioProcessor.getApvts(), gainParamID, true};
    RotaryKnob mixKnob {"Mix", *audioProcessor.getApvts(), mixParamID};
    RotaryKnob delayTimeLKnob {"Time (L, M)", *audioProcessor.getApvts(), delayTimeLParamID};
    RotaryKnob delayTimeRKnob {"Time (R)", *audioProcessor.getApvts(), delayTimeRParamID};
    RotaryKnob feedbackKnob {"Feedback", *audioProcessor.getApvts(), feedbackParamID, true};
    RotaryKnob stereoKnob {"Stereo", *audioProcessor.getApvts(), stereoParamID, true};
    RotaryKnob lowCutKnob {"Low Cut", *audioProcessor.getApvts(), lowCutParamID};
    RotaryKnob highCutKnob {"High Cut", *audioProcessor.getApvts(), highCutParamID};
    RotaryKnob lowCutQKnob {"Low Cut Q", *audioProcessor.getApvts(), lowCutQParamID};
    RotaryKnob highCutQKnob {"High Cut Q", *audioProcessor.getApvts(), highCutQParamID};
    RotaryKnob driveKnob {"Dist Pre-Gain", *audioProcessor.getApvts(), driveParamID, true};
    RotaryKnob postWSGainKnob {"Dist Post-Gain", *audioProcessor.getApvts(), postWSGainParamID, true};
    RotaryKnob delayNoteLKnob {"Note (L, M)", *audioProcessor.getApvts(), delayNoteLParamID };
    RotaryKnob delayNoteRKnob {"Note (R)", *audioProcessor.getApvts(), delayNoteRParamID };
    LevelMeter meter;

    juce::TextButton tempoSyncButton;
    juce::AudioProcessorValueTreeState::ButtonAttachment tempoSyncAttachment {
       *audioProcessor.getApvts(), tempoSyncParamID.getParamID(), tempoSyncButton
    };

    juce::ImageButton bypassButton;
    juce::AudioProcessorValueTreeState::ButtonAttachment bypassAttachment {
        *audioProcessor.getApvts(), bypassParamID.getParamID(), bypassButton
    };
    
    juce::GroupComponent delayGroup, feedbackGroup, outputGroup;
    MainLookAndFeel mainLF;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayAudioProcessorEditor)
};
