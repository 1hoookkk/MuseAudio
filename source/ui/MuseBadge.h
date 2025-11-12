#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "MuseLookAndFeel.h"

/**
 * MuseBadge - Small pill-shaped status indicator.
 *
 * Use for showing active state (e.g., "VOWEL", "NaN DETECTED", "AUTO ON").
 */
class MuseBadge : public juce::Component,
                  public juce::SettableTooltipClient
{
public:
    enum class Style { Default, Success, Warning, Error };

    MuseBadge(const juce::String& text = {}, Style style = Style::Default)
        : text_(text), style_(style)
    {
        setTooltip(text_); // Full text as tooltip (from SettableTooltipClient)
    }

    void setText(const juce::String& text)
    {
        text_ = text;
        setTooltip(text); // From SettableTooltipClient
        repaint();
    }

    void setStyle(Style style)
    {
        style_ = style;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Background color based on style
        juce::Colour bgColour;
        juce::Colour textColour = MuseTheme::Cream;

        switch (style_)
        {
            case Style::Success:
                bgColour = MuseTheme::Success.withAlpha(0.2f);
                break;
            case Style::Warning:
                bgColour = MuseTheme::Warning.withAlpha(0.2f);
                break;
            case Style::Error:
                bgColour = MuseTheme::Error.withAlpha(0.2f);
                break;
            case Style::Default:
            default:
                bgColour = MuseTheme::Grey700;
                break;
        }

        // Background (pill shape)
        g.setColour(bgColour);
        g.fillRoundedRectangle(bounds, MuseTheme::Radii::round);

        // Border
        juce::Colour borderColour = bgColour.brighter(0.3f);
        g.setColour(borderColour);
        g.drawRoundedRectangle(bounds, MuseTheme::Radii::round, 1.0f);

        // Text
        g.setFont(MuseTheme::Typography::tiny());
        g.setColour(textColour);
        g.drawText(text_, bounds, juce::Justification::centred, false);
    }

private:
    juce::String text_;
    Style style_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MuseBadge)
};
