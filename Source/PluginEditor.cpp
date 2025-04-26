#include "PluginProcessor.h"
#include "PluginEditor.h"


DelayAudioProcessorEditor::DelayAudioProcessorEditor (DelayAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), meter(p.levelL, p.levelR)
{
    delayGroup.setText("Delay");
    delayGroup.setTextLabelPosition(juce::Justification::horizontallyCentred);
    delayGroup.addAndMakeVisible(delayTimeLKnob);
    delayGroup.addAndMakeVisible(delayTimeRKnob);
    delayGroup.addChildComponent(delayNoteLKnob);
    delayGroup.addChildComponent(delayNoteRKnob);
    addAndMakeVisible(delayGroup);

    feedbackGroup.setText("Feedback");
    feedbackGroup.setTextLabelPosition(juce::Justification::horizontallyCentred);
    feedbackGroup.addAndMakeVisible(feedbackKnob);
    feedbackGroup.addAndMakeVisible(stereoKnob);
    feedbackGroup.addAndMakeVisible(lowCutKnob);
    feedbackGroup.addAndMakeVisible(highCutKnob);
    feedbackGroup.addAndMakeVisible(lowCutQKnob);
    feedbackGroup.addAndMakeVisible(highCutQKnob);
    feedbackGroup.addAndMakeVisible(driveKnob);
    feedbackGroup.addAndMakeVisible(postWSGainKnob);
    addAndMakeVisible(feedbackGroup);
    

    outputGroup.setText("Output");
    outputGroup.setTextLabelPosition(juce::Justification::horizontallyCentred);
    outputGroup.addAndMakeVisible(gainKnob);
    outputGroup.addAndMakeVisible(mixKnob);
    outputGroup.addAndMakeVisible(meter);
    addAndMakeVisible(outputGroup);

    // way to set separate colors to individual knobs
    //gainKnob.slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::green);
    tempoSyncButton.setButtonText("Sync");
    tempoSyncButton.setClickingTogglesState(true);
    tempoSyncButton.setBounds(0, 0, 70, 27);
    tempoSyncButton.setLookAndFeel(ButtonLookAndFeel::get());
    delayGroup.addAndMakeVisible(tempoSyncButton);

    setLookAndFeel(&mainLF);
    
    setSize (500, 560);

    updateDelayKnobs(audioProcessor.getParams()->getTempoSyncParam()->get());
    audioProcessor.getParams()->getTempoSyncParam()->addListener(this);
}

DelayAudioProcessorEditor::~DelayAudioProcessorEditor()
{
    audioProcessor.getParams()->getTempoSyncParam()->removeListener(this);
    setLookAndFeel(nullptr);
}


void DelayAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto noise = juce::ImageCache::getFromMemory(BinaryData::Noise_png, BinaryData::Noise_pngSize);
    auto fillType = juce::FillType(noise, juce::AffineTransform::scale(0.5f));
    g.setFillType(fillType);
    g.fillRect(getLocalBounds());

    auto rect = getLocalBounds().withHeight(40);
    g.setColour (Colors::header);
    g.fillRect(rect);

    auto image = juce::ImageCache::getFromMemory(BinaryData::Logo_png, BinaryData::Logo_pngSize);

    int destWidth = image.getWidth() / 2;
    int destHeight = image.getHeight() / 2;
    g.drawImage(image,
        getWidth() / 2 - destWidth / 2, 0, destWidth, destHeight,
        0, 0, image.getWidth(), image.getHeight());
}

void DelayAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    int y = 50;
    int height = bounds.getHeight() - 60;

    // positioning groups
    delayGroup.setBounds(10, y, 110, height);
    outputGroup.setBounds(bounds.getWidth() - 160, y, 150, height);
    feedbackGroup.setBounds(delayGroup.getRight() + 10, y,
        outputGroup.getX() - delayGroup.getRight() - 20, height);

    // positioning knobs in groups
    delayTimeLKnob.setTopLeftPosition(20, 20);
    delayTimeRKnob.setTopLeftPosition(delayTimeLKnob.getX(), delayTimeLKnob.getBottom() + 10);
    tempoSyncButton.setTopLeftPosition(20, delayTimeRKnob.getBottom() + 10);
    delayNoteLKnob.setTopLeftPosition(delayTimeLKnob.getX(), delayTimeLKnob.getY());
    delayNoteRKnob.setTopLeftPosition(delayTimeRKnob.getX(), delayTimeRKnob.getY());
    
    mixKnob.setTopLeftPosition(20, 20);
    gainKnob.setTopLeftPosition(mixKnob.getX(), mixKnob.getBottom() + 10);
    feedbackKnob.setTopLeftPosition(20, 20);
    stereoKnob.setTopLeftPosition(feedbackKnob.getRight() + 20, feedbackKnob.getY());
    lowCutKnob.setTopLeftPosition(feedbackKnob.getX(), feedbackKnob.getBottom() + 10);
    highCutKnob.setTopLeftPosition(lowCutKnob.getRight() + 20, lowCutKnob.getY());
    lowCutQKnob.setTopLeftPosition(lowCutKnob.getX(), lowCutKnob.getBottom() + 10);
    highCutQKnob.setTopLeftPosition(lowCutQKnob.getRight() + 20, lowCutQKnob.getY());
    driveKnob.setTopLeftPosition(lowCutKnob.getX(), highCutQKnob.getBottom() + 10);
    postWSGainKnob.setTopLeftPosition(driveKnob.getRight() + 20, driveKnob.getY());
    meter.setBounds(outputGroup.getWidth() - 45, 30, 30, gainKnob.getBottom() - 30);
}

void DelayAudioProcessorEditor::parameterValueChanged(int, float value)
{
    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
    {
        updateDelayKnobs(value != 0.0f);
    }
    else
    {
        juce::MessageManager::callAsync(
            [this,value]
            {
                updateDelayKnobs(value != 0.0f);
            });
    }
}

void DelayAudioProcessorEditor::updateDelayKnobs(bool tempoSyncActive)
{
    delayTimeLKnob.setVisible(!tempoSyncActive);
    delayTimeRKnob.setVisible(!tempoSyncActive);

    delayNoteLKnob.setVisible(tempoSyncActive);
    delayNoteRKnob.setVisible(tempoSyncActive);
}
