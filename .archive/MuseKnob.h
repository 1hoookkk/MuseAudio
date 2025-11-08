#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "MuseColors.h"

/**
 * MuseKnob - Custom rotary control for Muse
 *
 * Design:
 * - 80px diameter circle
 * - Thin line position indicator (no traditional markings)
 * - Warm taupe outline
 * - Lilac-to-peach gradient on active state
 * - Smooth, weighted feel (high resolution)
 * - No value display unless right-clicked
 *
 * Interaction:
 * - Vertical drag: 200px for full range (0-1)
 * - Shift+drag: 10× slower for fine control (2000px for full range)
 * - Double-click: Reset to default value (0.5)
 * - Drag tooltip: Shows current value (2 decimal places)
 */
class MuseKnob : public juce::Slider
{
public:
    MuseKnob()
    {
        setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                           juce::MathConstants<float>::pi * 2.75f,
                           true);
        
        // 200 pixels for full range (0-1) - smooth, weighted feel
        setMouseDragSensitivity(200);
        
        // Enable popup tooltip during drag
        setPopupDisplayEnabled(true, false, this);
        
        // Double-click resets to center (0.5)
        setDoubleClickReturnValue(true, 0.5);
    }
    
    // Format tooltip text with 2 decimal places
    juce::String getTextFromValue(double value) override
    {
        return juce::String(value, 2);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        auto centre = bounds.getCentre();
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;

        // Calculate fill bounds (slightly inset for stroke)
        auto fillBounds = bounds.reduced(2.0f);

        // Fill circle - gradient when active/dragging, light warm when inactive
        if (isMouseOver() || isMouseButtonDown())
        {
            // Active state: Lilac-to-peach gradient
            g.setGradientFill(Muse::Colors::createAccentGradientDiagonal(fillBounds));
        }
        else
        {
            // Inactive state: Very light warm fill
            g.setColour(Muse::Colors::KnobFillInactive);
        }
        g.fillEllipse(fillBounds);

        // Outer circle stroke (warm taupe, 2px)
        g.setColour(Muse::Colors::KnobOutline);
        g.drawEllipse(fillBounds, 2.0f);

        // Position indicator line
        auto angle = getRotaryParameters().startAngleRadians
                   + (getValue() - getMinimum()) / (getMaximum() - getMinimum())
                   * (getRotaryParameters().endAngleRadians - getRotaryParameters().startAngleRadians);

        // Draw clean indicator line from center to edge
        auto lineLength = radius - 8.0f;
        juce::Point<float> lineStart(centre.x + std::sin(angle) * 4.0f,
                                      centre.y - std::cos(angle) * 4.0f);
        juce::Point<float> lineEnd(centre.x + std::sin(angle) * lineLength,
                                    centre.y - std::cos(angle) * lineLength);

        g.setColour(Muse::Colors::KnobIndicator);
        g.drawLine(lineStart.x, lineStart.y, lineEnd.x, lineEnd.y, 2.0f);
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        // Shift modifier: 10× slower for fine control
        if (event.mods.isShiftDown())
            setMouseDragSensitivity(2000);  // 2000px for full range (10× slower)
        else
            setMouseDragSensitivity(200);   // Normal: 200px for full range
        
        juce::Slider::mouseDown(event);
    }
    
    void mouseUp(const juce::MouseEvent& event) override
    {
        // Reset to normal sensitivity after drag
        setMouseDragSensitivity(200);
        juce::Slider::mouseUp(event);
    }

private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MuseKnob)
};
