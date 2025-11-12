#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "MuseLookAndFeel.h"

/**
 * MuseCard - Styled panel container with optional title and padding.
 *
 * Use for grouping related controls (e.g., display area, knob section).
 */
class MuseCard : public juce::Component
{
public:
    MuseCard(const juce::String& title = {})
        : title_(title)
    {
        if (title_.isNotEmpty())
        {
            titleLabel_.setText(title_, juce::dontSendNotification);
            titleLabel_.setFont(MuseTheme::Typography::subheading());
            titleLabel_.setColour(juce::Label::textColourId, MuseTheme::Cream);
            titleLabel_.setJustificationType(juce::Justification::centredLeft);
            addAndMakeVisible(titleLabel_);
        }
    }

    // === Layout ===
    void resized() override
    {
        auto bounds = getLocalBounds();

        if (title_.isNotEmpty())
        {
            titleLabel_.setBounds(bounds.removeFromTop(32).reduced(MuseTheme::Spacing::sm, 0));
        }

        // Content area (with padding)
        contentBounds_ = bounds.reduced(MuseTheme::Spacing::sm);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Shadow
        MuseTheme::Shadows::subtle().drawForRectangle(g, bounds.toNearestInt());

        // Background
        g.setColour(MuseTheme::Grey800);
        g.fillRoundedRectangle(bounds, MuseTheme::Radii::md);

        // Border
        g.setColour(MuseTheme::Grey700);
        g.drawRoundedRectangle(bounds, MuseTheme::Radii::md, 1.0f);
    }

    // === Content Area ===
    juce::Rectangle<int> getContentBounds() const { return contentBounds_; }

private:
    juce::String title_;
    juce::Label titleLabel_;
    juce::Rectangle<int> contentBounds_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MuseCard)
};
