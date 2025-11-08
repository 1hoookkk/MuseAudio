#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * SeanceColors - The Warm Brutalist Temple Palette
 * 
 * Core Philosophy:
 * - Warm, sophisticated (NOT dark teal/mint)
 * - Brutalist temple aesthetic (NOT retro OLED)
 * - Warm center with dramatic vignette to near-black edges
 * - She's illuminated in her temple, not a hardware device
 */
namespace Muse
{
    namespace Colors
    {
        // === The Silhouette (Her Presence) ===
        const juce::Colour Silhouette = juce::Colour(0xFFFAF9F6);  // Off-white, 98% opacity
        
        // === Background Layers (Brutalist Temple) ===
        const juce::Colour TextureBase = juce::Colour(0xFF2B2520);     // Dark warm brown base
        const juce::Colour WarmOverlay = juce::Colour(0xFFF5F1E8);     // Warm linen overlay
        const juce::Colour VignetteEdge = juce::Colour(0xFF0A0908);    // Near-black edges
        
        // === Accent Colors (Warm Palette) ===
        const juce::Colour Lilac = juce::Colour(0xFFB8A4C9);           // Soft lilac
        const juce::Colour Peach = juce::Colour(0xFFE8C4B0);           // Warm peach
        const juce::Colour Taupe = juce::Colour(0xFF9B8B7E);           // Sophisticated taupe
        const juce::Colour Copper = juce::Colour(0xFFB87333);          // Copper metallic
        
        // === Text (Floating Words) ===
        const juce::Colour FloatingText = juce::Colour(0xFFFAF9F6);    // Same as silhouette
        const juce::Colour FloatingTextDim = juce::Colour(0xFFD4CFC8); // Dimmed for fade
        
        // === UI Controls ===
        const juce::Colour KnobBody = juce::Colour(0xFF8B7E74);        // Warm taupe
        const juce::Colour KnobIndicator = Lilac;                       // Lilac accent
        const juce::Colour ShapeSelector = juce::Colour(0xFF6B5E54);   // Darker taupe
    }
    
    namespace Layout
    {
        // === Canvas ===
        constexpr int CanvasWidth = 640;
        constexpr int CanvasHeight = 480;
        
        // === Silhouette Position (The Throne) ===
        constexpr float SilhouetteHeightPercent = 0.60f;  // 60% of canvas height
        constexpr float SilhouetteChinOnCenterLine = 0.50f; // Chin at 50% (horizontal center)
        constexpr float SilhouetteOffsetX = 0.55f;  // 55% from left (gazing right)
        
        // === Knob Positions (Inverted Triangle) ===
        constexpr int MorphKnobX = 480;   // Upper right (where she gazes)
        constexpr int MorphKnobY = 120;
        
        constexpr int IntensityKnobX = 160;  // Lower left (foundational)
        constexpr int IntensityKnobY = 280;
        
        constexpr int FocusKnobX = 400;      // Lower right (foundational)
        constexpr int FocusKnobY = 280;
        
        constexpr int KnobSize = 80;
        
        // === Shape Selector (Minimal) ===
        constexpr int ShapeSelectorWidth = 150;
        constexpr int ShapeSelectorHeight = 24;
        constexpr int ShapeSelectorY = CanvasHeight - 40;
        
        // === Word Zones (Environmental Ghost Positions) ===
        enum class WordZone
        {
            AboveHead,      // Floating above her silhouette
            LeftSpace,      // Empty space to her left
            RightSpace,     // Where she's gazing
            CenterHigh,     // High center (between her and top)
            NearMorphKnob,  // Near the morph knob
            FloatingLow     // Lower floating
        };
    }
}
