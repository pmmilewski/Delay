/*
  ==============================================================================

    LevelMeter.h
    Created: 24 Apr 2025 6:15:44pm
    Author:  PM

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class LevelMeter  : public juce::Component
{
public:
    LevelMeter();
    ~LevelMeter() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelMeter)
};
