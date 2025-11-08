#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * MuseColors - Design token constants for Muse UI
 *
 * Based on Option 3: Subtle Dark Texture Under Warm Overlay
 * See design/UI-SPECIFICATION.md for complete visual specification
 *
 * DO NOT modify these without updating design documentation.
 */
namespace Muse
{
    namespace Colors
    {
        // ============================================================================
        // BACKGROUND SYSTEM
        // ============================================================================

        /** Dark concrete texture base (applied at 15% opacity) */
        inline const juce::Colour TextureBase { 0xFF343A40 };

        /** Warm linen overlay (applied at 85% opacity over texture) */
        inline const juce::Colour WarmOverlay { 0xFFFAF0E6 };

        /** Final composited background (for reference - computed at runtime) */
        inline juce::Colour getCompositedBackground()
        {
            // This is a conceptual representation - actual implementation layers the texture
            return WarmOverlay; // Simplified; real version uses layered rendering
        }

        // ============================================================================
        // LOGO & BRANDING
        // ============================================================================

        /** Pale cream - logo silhouette color */
        inline const juce::Colour LogoSilhouette { 0xFFFAF9F6 };

        /** Slightly darker warm card for logo background */
        inline const juce::Colour LogoCard { 0xFFE8E3DB };

        // ============================================================================
        // TEXT & PRIMARY UI
        // ============================================================================

        /** Warm taupe - primary text, labels, knob outlines */
        inline const juce::Colour TextPrimary { 0xFF5C5552 };

        /** Lighter taupe - secondary text, hints */
        inline const juce::Colour TextSecondary { 0xFF8B8682 };

        /** Darker taupe - Muse's transmission text */
        inline const juce::Colour TextMuseVoice { 0xFF4A4745 };

        // ============================================================================
        // ACCENT COLORS (The Magic)
        // ============================================================================

        /** Soft lilac - gradient start */
        inline const juce::Colour AccentLilac { 0xFFC8B6D8 };

        /** Soft peach - gradient end */
        inline const juce::Colour AccentPeach { 0xFFFFD4C4 };

        /** Creates the signature lilac-to-peach gradient for given bounds */
        inline juce::ColourGradient createAccentGradient(juce::Rectangle<float> bounds)
        {
            return juce::ColourGradient(
                AccentLilac,
                bounds.getX(), bounds.getY(),
                AccentPeach,
                bounds.getRight(), bounds.getBottom(),
                false // Not radial
            );
        }

        /** Creates diagonal (135°) accent gradient for given bounds */
        inline juce::ColourGradient createAccentGradientDiagonal(juce::Rectangle<float> bounds)
        {
            return juce::ColourGradient(
                AccentLilac,
                bounds.getTopLeft(),
                AccentPeach,
                bounds.getBottomRight(),
                false
            );
        }

        // ============================================================================
        // TRANSMISSION AREA (Muse's Voice)
        // ============================================================================

        /** Soft lilac card background for transmission area */
        inline const juce::Colour TransmissionBackground { 0xFFE6D9F0 };

        /** Subtle lilac glow for breathing/pulsing effects (with alpha) */
        inline const juce::Colour TransmissionGlow { 0x4DC8B6D8 }; // 30% opacity

        // ============================================================================
        // KNOB COLORS
        // ============================================================================

        /** Warm taupe outline for knob circles */
        inline const juce::Colour KnobOutline { 0xFF5C5552 };

        /** Very light warm fill for inactive knobs */
        inline const juce::Colour KnobFillInactive { 0xFFF5EFE7 };

        /** Thin line showing knob position/indicator */
        inline const juce::Colour KnobIndicator { 0xFF5C5552 };

        /** Active knob uses the accent gradient - see createAccentGradient() */

        // ============================================================================
        // EFFECTS & SHADOWS
        // ============================================================================

        /** Shadow color for cards (logo, transmission) */
        inline const juce::Colour ShadowCard { 0x145C5552 }; // 8% opacity

        /** Shadow color for knobs (slightly stronger) */
        inline const juce::Colour ShadowKnob { 0x1F5C5552 }; // 12% opacity

        // ============================================================================
        // HELPER FUNCTIONS
        // ============================================================================

        /**
         * Creates a soft shadow drop-shadow effect for cards
         * Usage: g.setColour(Muse::Colors::createCardShadow()); g.fillRoundedRectangle(...)
         */
        inline juce::DropShadow createCardShadow()
        {
            return juce::DropShadow(ShadowCard, 8, juce::Point<int>(0, 2));
        }

        /**
         * Creates a subtle shadow for knobs
         */
        inline juce::DropShadow createKnobShadow()
        {
            return juce::DropShadow(ShadowKnob, 4, juce::Point<int>(0, 1));
        }

        /**
         * Creates a breathing glow effect (for logo pulse, processing states)
         * Returns a DropShadow with varying radius - animate the radius for pulse effect
         */
        inline juce::DropShadow createBreathingGlow(int radius = 12)
        {
            return juce::DropShadow(TransmissionGlow, radius, juce::Point<int>(0, 0));
        }

        /**
         * Draws a card with shadow and rounded corners
         */
        inline void drawCardWithShadow(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour fillColour)
        {
            // Draw shadow first
            auto shadow = createCardShadow();
            shadow.drawForRectangle(g, bounds.toNearestInt());

            // Draw card
            g.setColour(fillColour);
            g.fillRoundedRectangle(bounds, 8.0f); // CardRadius = 8
        }

    } // namespace Colors

    // ============================================================================
    // TYPOGRAPHY CONSTANTS
    // ============================================================================

    namespace Typography
    {
        /** Font size for parameter labels (small caps) */
        constexpr float LabelSize = 11.0f;

        /** Font size for parameter values (when shown) */
        constexpr float ValueSize = 14.0f;

        /** Font size for Muse's transmission text */
        constexpr float TransmissionSize = 16.0f;

        /** Font size for section titles */
        constexpr float TitleSize = 18.0f;

        /** Letter spacing for labels (tracking) */
        constexpr float LabelTracking = 0.08f;

        /** Normal font weight */
        constexpr float WeightNormal = 400.0f;

        /** Medium font weight (labels, emphasis) */
        constexpr float WeightMedium = 500.0f;

        /** Bold font weight (rarely used) */
        constexpr float WeightBold = 600.0f;

        /**
         * Returns the primary UI font (clean geometric sans-serif)
         */
        inline juce::Font getPrimaryFont(float size = ValueSize, float weight = WeightNormal)
        {
            return juce::Font(size, juce::Font::plain).withExtraKerningFactor(0.0f);
        }

        /**
         * Returns the font for Muse's voice (serif, more personal)
         */
        inline juce::Font getMuseVoiceFont(float size = TransmissionSize)
        {
            return juce::Font("Georgia", size, juce::Font::plain);
        }

        /**
         * Returns font for parameter labels (uppercase, tracked)
         */
        inline juce::Font getLabelFont()
        {
            auto font = getPrimaryFont(LabelSize, WeightMedium);
            font = font.withExtraKerningFactor(LabelTracking);
            return font;
        }

    } // namespace Typography

    // ============================================================================
    // SPACING & LAYOUT CONSTANTS
    // ============================================================================

    namespace Layout
    {
        /** Base spacing unit (all spacing should be multiples of this) */
        constexpr int Unit = 8;

        /** Plugin default width */
        constexpr int PluginWidth = 640;

        /** Plugin default height */
        constexpr int PluginHeight = 480;

        /** Section spacing (between major UI sections) */
        constexpr int SectionSpacing = Unit * 6; // 48px

        /** Control spacing (between knobs, buttons) */
        constexpr int ControlSpacing = Unit * 4; // 32px

        /** Label spacing (between control and its label) */
        constexpr int LabelSpacing = Unit * 2; // 16px

        /** Small padding (cards, buttons) */
        constexpr int PaddingSmall = Unit * 2; // 16px

        /** Medium padding (sections) */
        constexpr int PaddingMedium = Unit * 3; // 24px

        /** Large padding (main container edges) */
        constexpr int PaddingLarge = Unit * 5; // 40px

        /** Knob diameter */
        constexpr int KnobDiameter = 80;

        /** Knob stroke width */
        constexpr int KnobStrokeWidth = 2;

        /** Logo height in UI */
        constexpr int LogoHeight = 64;

        /** Transmission area height */
        constexpr int TransmissionHeight = 80;

        /** Border radius for cards */
        constexpr int CardRadius = 8;

        /** Border radius for buttons (pill shape) */
        constexpr int ButtonRadius = 20;

        /** Shadow blur radius */
        constexpr int ShadowBlur = 8;

    } // namespace Layout

    // ============================================================================
    // ANIMATION CONSTANTS
    // ============================================================================

    namespace Animation
    {
        /** Stutter-frame text reveal FPS (10 fps = otherworldly but readable) */
        constexpr int StutterFPS = 10;

        /** Stutter-frame text reveal duration (milliseconds) */
        constexpr int StutterDuration = 400;

        /** Breathing pulse duration (milliseconds) */
        constexpr int BreathingDuration = 3000;

        /** Fast UI transitions (hover states) */
        constexpr int TransitionFast = 150;

        /** Normal transitions (parameter changes) */
        constexpr int TransitionNormal = 300;

    } // namespace Animation

} // namespace Muse
