#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace MuseTheme {
    // === E-MU Planet Phatt Palette (1997 Digital Rompler) ===
    const juce::Colour CHASSIS_BLACK { 0xFF4E3159 }; // Deep plum panel (Planet Phatt purple)
    const juce::Colour LCD_BG        { 0xFF424143 }; // Graphite grey (dark chassis/trim)
    const juce::Colour LCD_TEXT      { 0xFFF4F3F8 }; // Warm off-white text
    const juce::Colour KNOB_LIGHT    { 0xFF7A7A7A }; // Warm slate grey (lighter)
    const juce::Colour KNOB_DARK     { 0xFF5A5A5A }; // Warm slate grey
    const juce::Colour MINT_GLOW     { 0xFFC04BCB }; // Vivid magenta accent (hover/active)
    const juce::Colour LED_GREEN     { 0xFF8FCF5A }; // Chartreuse green (LED matrix/LCD)

    // === Legacy Compatibility (for UI elements not yet ported) ===
    const juce::Colour Moss       { 0xFF4E3159 }; // Maps to chassis (deep plum)
    const juce::Colour MossDark   { 0xFF3A2343 }; // Darker plum
    const juce::Colour MossLight  { 0xFF6B4877 }; // Lighter plum
    const juce::Colour Obsidian   { 0xFF424143 }; // Graphite grey (dark trim)
    const juce::Colour Cream      { 0xFFF4F3F8 }; // Warm off-white text
    const juce::Colour OffWhite   { 0xFFF4F3F8 }; // Warm off-white
    const juce::Colour Grey700    { 0xFF5A5A5A }; // Warm slate grey borders
    const juce::Colour Grey600    { 0xFF5A5A5A }; // Disabled text
    const juce::Colour Grey400    { 0xFF8A8A8A }; // Secondary text

    // === Feedback Colors ===
    const juce::Colour Success  { 0xFF4CAF50 }; // Green (safe state)
    const juce::Colour Warning  { 0xFFFFA726 }; // Orange (caution)
    const juce::Colour Error    { 0xFFEF5350 }; // Red (NaN/clip)
    const juce::Colour Info     { 0xFF42A5F5 }; // Blue (info)

    // === Typography Scale (1980s LCD Style) ===
    struct Typography {
        static juce::Font display() {
            return juce::Font(juce::FontOptions("Courier New", 16.0f, juce::Font::bold));
        }

        static juce::Font heading() {
            return juce::Font(juce::FontOptions("Courier New", 16.0f, juce::Font::bold));
        }

        static juce::Font subheading() {
            return juce::Font(juce::FontOptions("Courier New", 11.0f, juce::Font::bold));
        }

        static juce::Font body() {
            return juce::Font(juce::FontOptions("Courier New", 11.0f, juce::Font::plain));
        }

        static juce::Font small() {
            return juce::Font(juce::FontOptions("Courier New", 10.0f, juce::Font::plain));
        }

        static juce::Font tiny() {
            return juce::Font(juce::FontOptions("Courier New", 9.0f, juce::Font::plain));
        }

        static juce::Font mono(float height = 10.0f) {
            return juce::Font(juce::FontOptions("Consolas", height, juce::Font::plain));
        }
    };

    // === Spacing Grid (8pt Base) ===
    struct Spacing {
        static constexpr int unit = 8;  // Base unit
        static constexpr int xs   = unit;      // 8px
        static constexpr int sm   = unit * 2;  // 16px
        static constexpr int md   = unit * 3;  // 24px
        static constexpr int lg   = unit * 4;  // 32px
        static constexpr int xl   = unit * 6;  // 48px
        static constexpr int xxl  = unit * 8;  // 64px
    };

    // === Border Radii ===
    struct Radii {
        static constexpr float none   = 0.0f;
        static constexpr float sm     = 3.0f;
        static constexpr float md     = 6.0f;
        static constexpr float lg     = 12.0f;
        static constexpr float round  = 999.0f;
    };

    // === Shadows ===
    struct Shadows {
        static juce::DropShadow subtle() {
            return juce::DropShadow(
                juce::Colours::black.withAlpha(0.15f),
                8,
                juce::Point<int>(0, 2)
            );
        }

        static juce::DropShadow medium() {
            return juce::DropShadow(
                juce::Colours::black.withAlpha(0.25f),
                12,
                juce::Point<int>(0, 4)
            );
        }

        static juce::DropShadow strong() {
            return juce::DropShadow(
                juce::Colours::black.withAlpha(0.35f),
                20,
                juce::Point<int>(0, 8)
            );
        }
    };

    // === Focus Ring ===
    struct Focus {
        static constexpr float ringWidth = 2.0f;
        static constexpr float ringOffset = 2.0f;

        static juce::Colour ringColour() {
            return MINT_GLOW;
        }

        static void drawFocusRing(juce::Graphics& g, juce::Rectangle<int> bounds) {
            g.setColour(ringColour());
            g.drawRoundedRectangle(
                bounds.toFloat().expanded(ringOffset),
                Radii::md,
                ringWidth
            );
        }
    };

    // Legacy color aliases (to be removed)
    const juce::Colour Grey800 = CHASSIS_BLACK;  // Deep plum
    const juce::Colour Grey900 = LCD_BG;          // Graphite grey
    const juce::Colour Grey950 = LCD_BG;          // Graphite grey
    const juce::Colour Grey200 = LCD_TEXT;        // Off-white
    const juce::Colour Amber = MINT_GLOW;         // Vivid magenta
}

/**
 * MuseLookAndFeel - Custom JUCE LookAndFeel_V4 for Muse plugin
 *
 * Implements premium boutique instrument aesthetic:
 * - Surgical precision (restrained palette, generous whitespace)
 * - Tactile immediacy (hardware-inspired knobs, instant feedback)
 * - Accessible by default (WCAG AA contrast, keyboard nav, screen readers)
 */
class MuseLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MuseLookAndFeel();
    ~MuseLookAndFeel() override = default;

    // === Slider (Rotary Knobs) ===
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override;

    juce::Label* createSliderTextBox(juce::Slider& slider) override;

    // === Linear Slider ===
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle style, juce::Slider& slider) override;

    // === Button ===
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;

    // === ToggleButton ===
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;

    // === ComboBox ===
    void drawComboBox(juce::Graphics& g, int width, int height,
                      bool isButtonDown, int buttonX, int buttonY,
                      int buttonW, int buttonH, juce::ComboBox& box) override;

    void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override;

    // === PopupMenu ===
    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;

    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                           bool isSeparator, bool isActive, bool isHighlighted,
                           bool isTicked, bool hasSubMenu, const juce::String& text,
                           const juce::String& shortcutKeyText,
                           const juce::Drawable* icon, const juce::Colour* textColour) override;

    // === Tooltip ===
    void drawTooltip(juce::Graphics& g, const juce::String& text, int width, int height) override;

    // === CallOutBox ===
    void drawCallOutBoxBackground(juce::CallOutBox& box, juce::Graphics& g,
                                   const juce::Path& path, juce::Image& cachedImage) override;

    // === Focus Ring (Note: drawFocusRect removed in JUCE 8, focus handled per-component) ===

private:
    void applyTheme();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MuseLookAndFeel)
};
