#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * OLEDLookAndFeel - Custom rendering for OLED-style UI
 *
 * Design specs from prototype:
 * - Dark teal (#2F4F4F) base
 * - Mint green (#d8f3dc) indicators with glow
 * - 3D skeuomorphic knobs with gradient shading
 * - Clean, retro hardware aesthetic
 */
class OLEDLookAndFeel : public juce::LookAndFeel_V4
{
public:
    // OLED color palette
    static constexpr uint32_t DarkTeal = 0xFF2F4F4F;
    static constexpr uint32_t MintGreen = 0xFFd8f3dc;
    static constexpr uint32_t Black = 0xFF000000;
    static constexpr uint32_t KnobLight = 0xFF385f5f;
    static constexpr uint32_t KnobDark = 0xFF263e3e;
    static constexpr uint32_t KnobMid = 0xFF325555;

    OLEDLookAndFeel()
    {
        // Set default colors for OLED theme
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(DarkTeal));
        setColour(juce::Label::textColourId, juce::Colour(MintGreen));
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(Black));
        setColour(juce::ComboBox::textColourId, juce::Colour(MintGreen));
        setColour(juce::ComboBox::outlineColourId, juce::Colour(MintGreen).withAlpha(0.5f));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPosProportional, float rotaryStartAngle,
                         float rotaryEndAngle, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
        auto centre = bounds.getCentre();
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;

        // Calculate angle for current value
        auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // === 3D Knob Background (skeuomorphic gradient) ===
        // Simulate lighting from top-left
        {
            juce::ColourGradient gradient(
                juce::Colour(KnobLight), // Highlight (top-left)
                centre.x - radius * 0.3f, centre.y - radius * 0.3f,
                juce::Colour(KnobDark),   // Shadow (bottom-right)
                centre.x + radius * 0.3f, centre.y + radius * 0.3f,
                false
            );
            gradient.addColour(0.5, juce::Colour(KnobMid));
            g.setGradientFill(gradient);
            g.fillEllipse(bounds);
        }

        // === Inset Shadow (inner depth) ===
        {
            auto insetBounds = bounds.reduced(2.0f);
            g.setColour(juce::Colour(KnobDark).withAlpha(0.6f));
            g.drawEllipse(insetBounds, 2.0f);
        }

        // === Rotating Indicator Dot (on outer edge) ===
        {
            auto dotRadius = 2.0f;
            auto dotDistance = radius - 6.0f;
            auto dotX = centre.x + std::sin(angle) * dotDistance;
            auto dotY = centre.y - std::cos(angle) * dotDistance;

            // Glow effect (outer ring)
            g.setColour(juce::Colour(MintGreen).withAlpha(0.3f));
            g.fillEllipse(dotX - 4.0f, dotY - 4.0f, 8.0f, 8.0f);

            // Dot itself
            g.setColour(juce::Colour(MintGreen));
            g.fillEllipse(dotX - dotRadius, dotY - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
        }

        // === Center Circle (darker, recessed) ===
        {
            auto centerRadius = radius * 0.8f;
            auto centerBounds = juce::Rectangle<float>(
                centre.x - centerRadius, centre.y - centerRadius,
                centerRadius * 2.0f, centerRadius * 2.0f
            );

            g.setColour(juce::Colour(DarkTeal));
            g.fillEllipse(centerBounds);
        }

        // === Line Indicator (in center, points to current position) ===
        {
            auto lineLength = radius * 0.6f;
            auto lineStart = centre.translated(std::sin(angle) * 6.0f, -std::cos(angle) * 6.0f);
            auto lineEnd = centre.translated(std::sin(angle) * lineLength, -std::cos(angle) * lineLength);

            // Glow effect
            g.setColour(juce::Colour(MintGreen).withAlpha(0.3f));
            g.drawLine(lineStart.x, lineStart.y, lineEnd.x, lineEnd.y, 4.0f);

            // Line itself
            g.setColour(juce::Colour(MintGreen));
            g.drawLine(lineStart.x, lineStart.y, lineEnd.x, lineEnd.y, 2.0f);
        }
    }

    juce::Font getLabelFont(juce::Label&) override
    {
        // Monospace font for retro hardware feel
        return juce::Font(juce::Font::getDefaultMonospacedFontName(), 10.0f, juce::Font::plain);
    }
};
