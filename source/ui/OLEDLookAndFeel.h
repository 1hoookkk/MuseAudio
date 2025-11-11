#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * OLEDLookAndFeel - Dark teal hardware aesthetic
 *
 * Color constants for OLED UI matching code.html
 */
class OLEDLookAndFeel : public juce::LookAndFeel_V4
{
public:
    // Color constants
    static constexpr juce::uint32 DarkTeal = 0xFF2F4F4F;
    static constexpr juce::uint32 MintGreen = 0xFFd8f3dc;
    static constexpr juce::uint32 Black = 0xFF000000;

    OLEDLookAndFeel()
    {
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(DarkTeal));
    }
};
