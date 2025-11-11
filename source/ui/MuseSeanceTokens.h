#pragma once

#include <juce_graphics/juce_graphics.h>

/**
 * OLED Séance Design System - LOCKED
 *
 * Source: design/muse-design-system.json
 * Philosophy: Haunted hardware from 1989. Mint phosphor on void-black background.
 *
 * IMMUTABLE RULES:
 * - Two colors only: #000000 (black) and #d8f3dc (mint phosphor)
 * - No gradients on background
 * - Smooth vector mouth, not pixel grid
 * - 10 FPS mouth animation (hardware snap)
 * - Clean and minimal
 */
namespace MuseSeance
{
    // ===== COLOR SYSTEM =====
    namespace Colors
    {
        // Background: Pure black void (no gradients)
        inline constexpr juce::uint32 BG_VOID = 0xFF000000;

        // Mint phosphor (P1 CRT green - 19.8:1 contrast ratio)
        inline constexpr juce::uint32 MINT = 0xFFd8f3dc;
        inline constexpr juce::uint32 MINT_GLOW = 0x80d8f3dc;    // 50% alpha for glow
        inline constexpr juce::uint32 MINT_DIM = 0x40d8f3dc;     // 25% alpha for subtle elements

        // Helper functions
        inline juce::Colour getVoid() { return juce::Colour(BG_VOID); }
        inline juce::Colour getMint() { return juce::Colour(MINT); }
        inline juce::Colour getMintGlow() { return juce::Colour(MINT_GLOW); }
        inline juce::Colour getMintDim() { return juce::Colour(MINT_DIM); }
    }

    // ===== LAYOUT SYSTEM =====
    namespace Layout
    {
        // Canvas dimensions (horizontal layout)
        inline constexpr int CANVAS_WIDTH = 800;
        inline constexpr int CANVAS_HEIGHT = 400;

        // Region boundaries
        inline constexpr int CONTROLS_WIDTH = 380;
        inline constexpr int DISPLAY_X = 380;
        inline constexpr int DISPLAY_WIDTH = 420;

        // Knob specifications
        inline constexpr int KNOB_DIAMETER = 120;

        // Knob positions (x, y from design system)
        inline constexpr int MORPH_X = 120;
        inline constexpr int MORPH_Y = 90;

        inline constexpr int INTENSITY_X = 120;
        inline constexpr int INTENSITY_Y = 260;

        inline constexpr int MIX_X = 280;
        inline constexpr int MIX_Y = 175;

        // Header
        inline constexpr int HEADER_X = 30;
        inline constexpr int HEADER_Y = 15;
    }

    // ===== VISUAL EFFECTS =====
    namespace Effects
    {
        // Phosphor glow (authentic CRT bloom)
        inline constexpr float GLOW_BLUR_RADIUS = 8.0f;
        inline constexpr float GLOW_OPACITY = 0.6f;

        // Mouth animation timing
        inline constexpr int MOUTH_FPS = 10;                    // Hardware snap rate
        inline constexpr int MOUTH_UPDATE_MS = 100;             // 1000/10 = 100ms

        inline constexpr int UI_REFRESH_HZ = 60;                // Smooth UI interactions

        // Helper: Create phosphor glow effect
        inline juce::DropShadow createPhosphorGlow()
        {
            return juce::DropShadow(
                Colors::getMintGlow(),
                static_cast<int>(GLOW_BLUR_RADIUS),
                juce::Point<int>(0, 0)
            );
        }
    }

    // ===== TYPOGRAPHY =====
    namespace Typography
    {
        inline constexpr float HEADER_SIZE = 18.0f;
        inline constexpr float LABEL_SIZE = 12.0f;

        inline juce::Font getHeaderFont()
        {
            return juce::Font(juce::FontOptions(HEADER_SIZE, juce::Font::bold));
        }

        inline juce::Font getLabelFont()
        {
            return juce::Font(juce::FontOptions(LABEL_SIZE, juce::Font::plain));
        }
    }

    // ===== MOUTH VOWEL MAPPING =====
    namespace MouthMapping
    {
        // Ellipse dimensions for each vowel shape
        // Format: {width_ratio, height_ratio} relative to bounds

        struct VowelDimensions
        {
            float widthRatio;
            float heightRatio;
        };

        // AA: Wide mouth (dark back vowel)
        inline constexpr VowelDimensions AA = {0.85f, 0.6f};

        // AH: Neutral mouth (center transition)
        inline constexpr VowelDimensions AH = {0.65f, 0.5f};

        // EE: Narrow smile (bright front vowel)
        inline constexpr VowelDimensions EE = {0.9f, 0.35f};

        // OH: Rounded (medium round)
        inline constexpr VowelDimensions OH = {0.55f, 0.7f};

        // OO: More rounded (tight round)
        inline constexpr VowelDimensions OO = {0.45f, 0.65f};

        // Wide: Wide aperture
        inline constexpr VowelDimensions Wide = {0.9f, 0.55f};

        // Narrow: Narrow aperture
        inline constexpr VowelDimensions Narrow = {0.5f, 0.45f};

        // Neutral: Minimal movement
        inline constexpr VowelDimensions Neutral = {0.6f, 0.5f};
    }
}
