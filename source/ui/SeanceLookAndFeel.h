#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "SeanceColors.h"

/**
 * SeanceLookAndFeel - Warm Brutalist Temple Aesthetic
 * 
 * NOT:
 * - Teal/mint OLED hardware aesthetic
 * - Dark mode plugin
 * - Skeuomorphic 3D gradients
 * 
 * YES:
 * - Warm sophisticated palette (taupe, linen, lilac, peach)
 * - Flat, brutalist minimalism
 * - Generous negative space
 * - She IS the UI, not decorations
 */
class SeanceLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SeanceLookAndFeel()
    {
        using namespace Muse::Colors;
        
        // Set minimal palette
        setColour(juce::ResizableWindow::backgroundColourId, TextureBase);
        setColour(juce::Label::textColourId, FloatingText);
        setColour(juce::ComboBox::backgroundColourId, ShapeSelector);
        setColour(juce::ComboBox::textColourId, FloatingText);
        setColour(juce::ComboBox::outlineColourId, Taupe.withAlpha(0.3f));
    }
    
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPosProportional, float rotaryStartAngle,
                         float rotaryEndAngle, juce::Slider& slider) override
    {
        using namespace Muse::Colors;
        
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
        auto centre = bounds.getCentre();
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        
        // === Simple Flat Knob Body (Warm Taupe) ===
        g.setColour(KnobBody);
        g.fillEllipse(bounds);
        
        // === Subtle Ring (Darker) ===
        g.setColour(KnobBody.darker(0.3f));
        g.drawEllipse(bounds.reduced(2.0f), 2.0f);
        
        // === Lilac Indicator Line ===
        auto lineLength = radius * 0.65f;
        auto lineStart = centre;
        auto lineEnd = centre.translated(std::sin(angle) * lineLength, -std::cos(angle) * lineLength);
        
        g.setColour(KnobIndicator);
        g.drawLine(lineStart.x, lineStart.y, lineEnd.x, lineEnd.y, 3.0f);
        
        // === Center Dot ===
        g.setColour(KnobIndicator.brighter(0.2f));
        g.fillEllipse(centre.x - 4.0f, centre.y - 4.0f, 8.0f, 8.0f);
    }
    
    juce::Font getLabelFont(juce::Label&) override
    {
        return juce::Font(juce::Font::getDefaultSansSerifFontName(), 12.0f, juce::Font::plain);
    }
};
