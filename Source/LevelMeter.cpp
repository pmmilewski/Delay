#include <JuceHeader.h>
#include "LevelMeter.h"
#include "LookAndFeel.h"

LevelMeter::LevelMeter(std::atomic<float>& measurementL_, std::atomic<float>& measurementR_)
    : measurementL(measurementL_), measurementR(measurementR_),
      dbLevelL(clampdB), dbLevelR(clampdB)
{
    setOpaque(true);
    startTimerHz(refreshRate);
    decay = 1.0f - std::exp(-1.0f / (static_cast<float>(refreshRate) * 0.2f)); // 0.2f -> decay time 200 ms
}

//==============================================================================
LevelMeter::~LevelMeter()
{
}

void LevelMeter::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds();

    g.fillAll(Colors::LevelMeter::background);

    drawLevel(g, dbLevelL, 0, 7);
    drawLevel(g, dbLevelR, 9, 7);

    g.setFont(Fonts::getFont(10.0f));
    for (float db = maxdB; db >= mindB; db -= stepdB)
    {
        int y = positionForLevel(db);

        g.setColour(Colors::LevelMeter::tickLine);
        g.fillRect(0, y, 16, 1);

        g.setColour(Colors::LevelMeter::tickLabel);
        g.drawSingleLineText(juce::String(static_cast<int>(db)),
            bounds.getWidth(), y + 3, juce::Justification::right);
    }
}

void LevelMeter::resized()
{
    maxPos = 4.0f;
    minPos = static_cast<float>(getHeight()) - 4.0f;
}

void LevelMeter::timerCallback()
{
    updateLevel(measurementL.load(), levelL, dbLevelL);
    updateLevel(measurementR.load(), levelR, dbLevelR);

    repaint();
}

void LevelMeter::drawLevel(juce::Graphics& g, float level, int x, int width)
{
    int y = positionForLevel(level);
    if (level > 0.0f)
    {
        const int y0 = positionForLevel(0.0f);
        g.setColour(Colors::LevelMeter::tooLoud);
        g.fillRect(x, y, width, y0 - y);
        g.setColour(Colors::LevelMeter::levelOK);
        g.fillRect(x, y0, width, getHeight() - y0);
    }
    else if (y < getHeight())
    {
        g.setColour(Colors::LevelMeter::levelOK);
        g.fillRect(x, y, width, getHeight() - y);
    }
}

void LevelMeter::updateLevel(float newLevel, float& smoothedLevel, float& leveldB) const
{
    if (newLevel > smoothedLevel)
    {
        smoothedLevel = newLevel; // instantaneous attack
    }
    else
    {
        smoothedLevel += (newLevel - smoothedLevel) * decay;
    }

    if (smoothedLevel > clampLevel)
    {
        leveldB = juce::Decibels::gainToDecibels(smoothedLevel);
    }
    else
    {
        leveldB = clampdB;
    }
}
