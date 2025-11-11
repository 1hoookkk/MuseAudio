#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "MuseSeanceTokens.h"
#include "../PluginProcessor.h"

/**
 * Muse Mouth - SIMPLE VERSION THAT WORKS
 */
class MuseMouth : public juce::Component,
                  private juce::Timer
{
public:
    MuseMouth(PluginProcessor& proc)
        : processorRef(proc)
    {
        startTimer(100);  // 10 FPS
        setOpaque(false);
    }

    ~MuseMouth() override
    {
        stopTimer();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Calculate mouth bounds based on current vowel shape
        auto mouthBounds = calculateMouthBounds(bounds);

        // Simple mint ellipse outline
        g.setColour(juce::Colour(0xFFd8f3dc));  // Bright mint
        g.drawEllipse(mouthBounds, 3.0f);

        // Subtle fill
        g.setColour(juce::Colour(0x20d8f3dc));
        g.fillEllipse(mouthBounds);
    }

private:
    void timerCallback() override
    {
        auto newVowel = processorRef.getCurrentVowelShape();
        if (newVowel != currentVowel_)
        {
            currentVowel_ = newVowel;
            repaint();
        }
    }

    juce::Rectangle<float> calculateMouthBounds(juce::Rectangle<float> bounds)
    {
        MuseSeance::MouthMapping::VowelDimensions dims = getDimensionsForVowel(currentVowel_);

        float centerX = bounds.getCentreX();
        float centerY = bounds.getCentreY();

        float maxWidth = bounds.getWidth() * 0.7f;
        float maxHeight = bounds.getHeight() * 0.7f;

        float width = maxWidth * dims.widthRatio;
        float height = maxHeight * dims.heightRatio;

        return juce::Rectangle<float>(
            centerX - width * 0.5f,
            centerY - height * 0.5f,
            width,
            height
        );
    }

    MuseSeance::MouthMapping::VowelDimensions getDimensionsForVowel(PluginProcessor::VowelShape vowel)
    {
        using namespace MuseSeance::MouthMapping;

        switch (vowel)
        {
            case PluginProcessor::VowelShape::AA:      return AA;
            case PluginProcessor::VowelShape::AH:      return AH;
            case PluginProcessor::VowelShape::EE:      return EE;
            case PluginProcessor::VowelShape::OH:      return OH;
            case PluginProcessor::VowelShape::OO:      return OO;
            case PluginProcessor::VowelShape::Wide:    return Wide;
            case PluginProcessor::VowelShape::Narrow:  return Narrow;
            case PluginProcessor::VowelShape::Neutral: return Neutral;
            default:                                   return AH;
        }
    }

    PluginProcessor& processorRef;
    PluginProcessor::VowelShape currentVowel_ = PluginProcessor::VowelShape::AH;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MuseMouth)
};
