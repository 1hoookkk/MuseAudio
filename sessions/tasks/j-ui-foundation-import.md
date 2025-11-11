---
task: j-ui-foundation-import
branch: ui/theme-system
status: pending
created: 2025-11-08
modules: [ui, brand, design-system]
---

# UI Foundation: Import & Adapt Production Components

## Problem/Goal

Import battle-tested UI components from MuseAudio-UI-Reference repository and adapt them to Muse's brand identity ("Brutalist Temple" aesthetic with stutter-frame otherworldly transmission).

**Strategic Context:**
- Have production-quality UI code in external reference repo
- Need DSP-reactive mouth visualizer (core brand identity)
- Must maintain "10 FPS stutter-frame" aesthetic (not smooth animations)
- Color palette must match Muse design tokens (warm temple, not vibrant Field colors)

## Success Criteria
- [ ] GenerativeMouth.h adapted with MuseColors (OLEDLookAndFeel dependency removed)
- [ ] FieldLookAndFeel_v2 adapted to Muse aesthetic (renamed MuseLookAndFeel)
- [ ] All components use Muse design tokens from design/design-tokens.json
- [ ] Mouth visualizer integrated with PluginEditor
- [ ] Components compile with JUCE 8 and match brand specification
- [ ] Code-review agent approval on final adapted components

## Context Manifest

### How the Current UI System Works

**The OLED Aesthetic (Current Implementation):**

When you open the current Muse plugin, you're greeted with a retro hardware device aesthetic - a dark teal background (0x2F4F4F) with mint green indicators (0xd8f3dc), mimicking a vintage OLED display. The plugin is 400x600px and feels like a physical synthesizer module with glowing knobs and an LED screen.

The UI is built around `PluginEditor` (source/PluginEditor.h/cpp), which uses `OLEDLookAndFeel` (source/ui/OLEDLookAndFeel.h) to render all controls. This LookAndFeel provides:
- 3D skeuomorphic knobs with radial gradients (dark-to-light shading from bottom-right to top-left)
- Mint green dots on the knob perimeter that rotate with the value
- A central dark teal circle with mint green indicator line
- Glowing effects using layered alpha strokes

The crown jewel is `GenerativeMouth` (from MuseAudio-UI-Reference/muse-legacy-ui/GenerativeMouth.h), a procedurally animated mouth visualizer. This component is CRITICAL to Muse's identity - it's not just eye candy, it's Her presence.

**How GenerativeMouth Works (The Magic):**

GenerativeMouth is a 16x6 LED matrix that generates a UNIQUE mouth pattern every 100ms (10 FPS stutter aesthetic). This is not interpolated animation - each frame is procedurally computed from:
1. Vowel shape (AA/AH/EE/OH/OO) derived from the Z-plane filter's current shape pair and morph position
2. Audio activity (RMS level) that expands the mouth opening
3. Organic noise (3-5% pixel flicker) for living, breathing quality

The mouth updates via timer at 10 FPS (spec requirement). The algorithm:
- Generates elliptical base shape with width/open/smile/round parameters from vowel
- Audio level expands opening (0.6 + 0.8 * audioLevel scaling)
- Adds organic noise (random pixel flips)
- Adds edge softness (probabilistic boundary)
- Adds asymmetry (random row shifts for naturalism)
- Adds breathing jitter (sparkles when idle)
- Adds teeth hints (for AA/AH/EE when open)

Each pixel is either ON (mint green with glow) or OFF (3% alpha dim). The result: a mouth that feels alive, reacting to DSP state with intentional digital stutter.

**The Problem: OLEDLookAndFeel Dependency:**

GenerativeMouth currently imports `OLEDLookAndFeel.h` (line 2) to access color constants:
- `OLEDLookAndFeel::MintGreen` (0xFFd8f3dc) - used for lit LEDs and glow
- `OLEDLookAndFeel::DarkTeal` - used for background
- `OLEDLookAndFeel::Black` - used for OLED screen

This creates a hard dependency on the OLED aesthetic. When we want Muse to use the Brutalist Temple palette (warm taupe, linen, lilac, peach), the mouth still glows mint green.

**FieldLookAndFeel_v2: The Structure We Need (But Wrong Colors):**

The reference repository also has `FieldLookAndFeel_v2` (from MuseAudio-UI-Reference/enginefield-ui/), which demonstrates excellent JUCE LookAndFeel patterns:
- Clean namespace for colors (`FieldColors::` with const juce::Colour definitions)
- Custom `drawLinearSlider()` for horizontal strip controls
- Custom `drawButtonBackground()` for pill-shaped toggle buttons
- Custom `drawButtonText()` with state-based color switching
- Typography helper (`getTextButtonFont()`)

However, FieldLookAndFeel_v2 uses the vibrant Field color palette:
- Chassis: 0x2D6DA9 (vibrant blue)
- Tracer: 0xC3FF00 (lime green)
- Delta: 0xF9F034 (bright yellow)
- Baseline: 0x59B850 (alive green)

This is the OPPOSITE of Muse's warm, sophisticated palette. We need the structural patterns but with Muse's colors.

**Muse's Design System (Already Implemented):**

Muse already has a complete design token system in `source/ui/MuseColors.h` and `design/design-tokens.json`. This defines:

**Color Palette:**
- Background: Dark texture (0x343A40) at 15% opacity under warm linen (0xFAF0E6) at 85%
- Logo silhouette: Pale cream (0xFAF9F6)
- Text primary: Warm taupe (0x5C5552)
- Accent gradient: Lilac (0xC8B6D8) to Peach (0xFFD4C4)
- Transmission background: Soft lilac (0xE6D9F0)

**Typography:**
- Labels: 11px, weight 500, uppercase, 0.08em tracking
- Values: 14px, weight 400
- Muse's voice: Georgia serif, 16px (personal, hand-written feel)

**Layout:**
- Plugin: 640x480px (4:3 ratio, NOT 400x600 OLED size)
- Knobs: 80px diameter, 2px stroke
- Base unit: 8px grid
- Logo card, transmission card with 8px radius, subtle shadows

**Animation:**
- Stutter-frame: 10 FPS (100ms per frame) - MATCHES GenerativeMouth!
- Breathing pulse: 3000ms cycle
- Transitions: 150ms fast, 300ms normal

**The Architectural Insight:**

MuseColors.h is ALREADY the correct structure - it's a namespace with inline const juce::Colour definitions and helper functions. It's the EXACT pattern we need. We don't need to reinvent anything, we just need to:

1. Remove OLEDLookAndFeel dependency from GenerativeMouth
2. Replace `OLEDLookAndFeel::MintGreen` with `Muse::Colors::AccentLilac` (or create a new "MouthGlow" color)
3. Create MuseLookAndFeel that uses Muse::Colors instead of FieldColors
4. Update PluginEditor to use MuseLookAndFeel instead of OLEDLookAndFeel

### What Needs to Connect: UI Foundation Import

**The Goal:** Get Muse's OLED mouth visualizer working with the Brutalist Temple aesthetic while preserving the 10 FPS generative magic.

**Step 1: GenerativeMouth Color Migration**

Currently (line 122, 129, 135):
```cpp
const auto mint = juce::Colour (OLEDLookAndFeel::MintGreen);
```

Needs to become:
```cpp
const auto museGlow = juce::Colour (Muse::Colors::AccentLilac); // or custom MouthGlow color
```

The challenge: Should the mouth glow be lilac (matches accent gradient), peach (warmer), or a new custom color? The design spec (design/UI-SPECIFICATION.md) doesn't explicitly define a mouth color because the current prototype doesn't have a mouth visualizer yet.

**Decision point:** We need to determine if the mouth should:
- Use the lilac-to-peach gradient (breathing effect)
- Be a single warm color (consistency)
- Remain mint green (contrast with warm palette)

**Step 2: Create MuseLookAndFeel (Adapt FieldLookAndFeel_v2)**

FieldLookAndFeel_v2 structure (what we're adapting):
```cpp
class FieldLookAndFeel_v2 : public juce::LookAndFeel_V4 {
    // Constructor sets default colors
    // drawLinearSlider() - horizontal strip thumb rendering
    // drawButtonBackground() - pill button with state colors
    // drawButtonText() - state-based text color
    // getTextButtonFont() - typography
};
```

Becomes MuseLookAndFeel:
```cpp
class MuseLookAndFeel : public juce::LookAndFeel_V4 {
    // Constructor uses Muse::Colors::*
    // drawRotarySlider() - flat warm knobs with lilac indicator (NOT 3D gradients)
    // drawButtonBackground() - warm pill buttons
    // drawButtonText() - taupe/lilac text
    // getTextButtonFont() - Muse typography (11px medium weight)
};
```

Key differences from FieldLookAndFeel_v2:
- Replace all FieldColors:: with Muse::Colors::
- Rotary knobs are FLAT (not gradient), warm taupe body with lilac indicator line
- No 3D effects (brutalist minimal)
- Typography uses Muse::Typography helpers

Key differences from OLEDLookAndFeel:
- Remove radial gradients on knobs (lines 46-56 in OLEDLookAndFeel.h)
- Remove inset shadow effects (lines 60-64)
- Flat fill with single color, not gradient
- Indicator is simple line, not dot on perimeter

**Step 3: Mouth Image Assets Integration**

The task mentions mouth image assets in:
```
C:\Users\hooki\Downloads\stitch_muse_default_shape\stitch_muse_default_shape\muse_default_shape\screen.png
```

However, GenerativeMouth is PROCEDURAL - it doesn't use image assets. It generates pixel patterns algorithmically. The image assets are likely:
- Reference images for visual design
- Fallback static mouth for loading states
- Prototype mockup comparisons

We should NOT replace the procedural mouth with static images. The 10 FPS generative algorithm is core to Muse's identity. The images may be used for:
- Initial state before DSP is running
- Loading placeholder
- Design reference for color/shape

**Step 4: Integration with PluginEditor**

Current PluginEditor.cpp (lines 6-7):
```cpp
setLookAndFeel (&oledLookAndFeel);
```

Needs to become:
```cpp
setLookAndFeel (&museLookAndFeel);
```

The generativeMouth component (line 303 in PluginEditor.cpp) is already positioned and wired correctly. It receives:
- Vowel shape from processor (line 388-389)
- Audio level from processor (line 386)
- Morph value for subtle influence (line 392)

These connections don't change. Only the visual rendering (colors) changes.

**Step 5: CRITICAL - Preserve the 10 FPS Aesthetic**

The design spec (design/UI-SPECIFICATION.md, lines 226-235) defines stutter-frame animation:
```
FPS: 10 (one frame every 100ms)
Duration: 400ms total
Method: Reveal characters one at a time, stuttering
Effect: Low-fps transmission from another dimension
```

GenerativeMouth already implements this PERFECTLY (line 50):
```cpp
startTimerHz (10);
```

DO NOT change this to 30 FPS or add smoothing. The stutter is intentional. The mouth updates at 10 FPS, painting happens at component repaint rate (which is fine).

### Technical Reference Details

#### GenerativeMouth Component Interface

**File:** C:\Muse\MuseAudio-UI-Reference\muse-legacy-ui\GenerativeMouth.h

**Public API:**
```cpp
class GenerativeMouth : public juce::Component, private juce::Timer {
public:
    enum class Vowel {
        AA, AH, EE, OH, OO,  // Standard vowels
        Wide, Narrow, Neutral // Special shapes
    };

    void setVowel(Vowel v);           // Update vowel shape
    void setMorph(float m);            // 0-1 morph parameter
    void setAudioLevel(float level);   // 0-1 RMS level

private:
    void generateNextFrame();          // Core procedural algorithm
    void getVowelParameters(...);      // Vowel → shape parameters
    void addAsymmetry();               // Organic variation
    void addBreathingJitter();         // Idle sparkles
    void addTeethHint();               // AA/AH/EE teeth
};
```

**Grid Specification:**
- `COLS = 16`, `ROWS = 6` (96 total pixels)
- `currentFrame` is `std::array<bool, 96>` (on/off per pixel)
- Timer callback at 10 FPS calls `generateNextFrame()` then `repaint()`

**Color References (Lines 122, 129, 135, 136):**
```cpp
const auto mint = juce::Colour (OLEDLookAndFeel::MintGreen);
g.setColour (mint.withAlpha (0.15f * brightness));  // Outer glow
g.setColour (mint.withAlpha (brightness));           // Core LED
g.setColour (juce::Colour (OLEDLookAndFeel::MintGreen).withAlpha (0.03f)); // Dim LED
```

**Required Changes:**
1. Remove `#include "OLEDLookAndFeel.h"` (line 2)
2. Replace color references with Muse::Colors equivalents
3. PRESERVE all algorithmic logic (generateNextFrame, asymmetry, jitter, teeth)

#### FieldLookAndFeel_v2 Structure (Adaptation Template)

**File:** C:\Muse\MuseAudio-UI-Reference\enginefield-ui\FieldLookAndFeel_v2.h/cpp

**Color Namespace Pattern (Lines 19-31):**
```cpp
namespace FieldColors {
    const juce::Colour chassis       {0x2D, 0x6D, 0xA9};  // Vibrant blue
    const juce::Colour viewport      {0x00, 0x00, 0x00};  // Black
    const juce::Colour tracerLime    {0xC3, 0xFF, 0x00};  // Lime
    // ... etc
}
```

**Adaptation Pattern:**
Use existing Muse::Colors namespace (already defined in source/ui/MuseColors.h), don't create a new one. Just reference Muse::Colors::KnobOutline, etc.

**Constructor Pattern (Lines 13-18 in .cpp):**
```cpp
FieldLookAndFeel_v2::FieldLookAndFeel_v2() {
    setColour(juce::Slider::thumbColourId, FieldColors::deltaYellow);
    setColour(juce::Slider::trackColourId, FieldColors::outlineGrey);
}
```

**MuseLookAndFeel Constructor Should Use:**
```cpp
MuseLookAndFeel::MuseLookAndFeel() {
    using namespace Muse::Colors;
    setColour(juce::ResizableWindow::backgroundColourId, WarmOverlay);
    setColour(juce::Label::textColourId, TextPrimary);
    setColour(juce::Slider::thumbColourId, AccentLilac);
    setColour(juce::Slider::trackColourId, KnobOutline);
    // ... etc
}
```

**Rotary Slider Implementation (for MuseLookAndFeel):**

DO NOT copy OLEDLookAndFeel's gradient approach (lines 34-106). Instead, flat rendering:
```cpp
void MuseLookAndFeel::drawRotarySlider(...) {
    using namespace Muse::Colors;

    // Flat fill (no gradient)
    g.setColour(KnobFillInactive);
    g.fillEllipse(bounds);

    // Outline
    g.setColour(KnobOutline);
    g.drawEllipse(bounds.reduced(1.0f), 2.0f);

    // Indicator line (lilac)
    g.setColour(KnobIndicator);
    g.drawLine(center, endPoint, 3.0f);
}
```

Reference SeanceLookAndFeel.h (lines 34-62) for flat knob pattern - it's already correct for Muse aesthetic.

#### Muse Design Token Reference

**File:** C:\Muse\MuseAudio\source\ui\MuseColors.h

**Background System (Lines 16-31):**
```cpp
inline const juce::Colour TextureBase { 0xFF343A40 };       // Dark concrete (15%)
inline const juce::Colour WarmOverlay { 0xFFFAF0E6 };       // Warm linen (85%)
```

**Accent Colors (Lines 55-89):**
```cpp
inline const juce::Colour AccentLilac { 0xFFC8B6D8 };       // Soft lilac
inline const juce::Colour AccentPeach { 0xFFFFD4C4 };       // Soft peach

inline juce::ColourGradient createAccentGradient(juce::Rectangle<float> bounds) {
    return juce::ColourGradient(AccentLilac, bounds.getX(), bounds.getY(),
                                 AccentPeach, bounds.getRight(), bounds.getBottom(), false);
}
```

**Knob Colors (Lines 102-114):**
```cpp
inline const juce::Colour KnobOutline { 0xFF5C5552 };       // Warm taupe
inline const juce::Colour KnobFillInactive { 0xFFF5EFE7 };  // Very light warm
inline const juce::Colour KnobIndicator { 0xFF5C5552 };     // Thin line
```

**Typography Helpers (Lines 172-228):**
```cpp
namespace Typography {
    constexpr float LabelSize = 11.0f;
    constexpr float ValueSize = 14.0f;
    constexpr float TransmissionSize = 16.0f;

    inline juce::Font getLabelFont() {
        auto font = getPrimaryFont(LabelSize, WeightMedium);
        font = font.withExtraKerningFactor(LabelTracking);
        return font;
    }
}
```

**Animation Constants (Lines 287-307):**
```cpp
namespace Animation {
    constexpr int StutterFPS = 10;            // MATCHES GenerativeMouth!
    constexpr int StutterDuration = 400;      // ms
    constexpr int BreathingDuration = 3000;   // ms
}
```

#### Current PluginEditor Integration Points

**File:** C:\Muse\MuseAudio\source\PluginEditor.cpp

**LookAndFeel Setup (Line 7):**
```cpp
setLookAndFeel (&oledLookAndFeel);
```

**GenerativeMouth Declaration (PluginEditor.h, Line 67):**
```cpp
GenerativeMouth generativeMouth;
```

**GenerativeMouth Positioning (PluginEditor.cpp, Line 303):**
```cpp
generativeMouth.setBounds (screenBounds.reduced (6));
```

**GenerativeMouth Data Updates (Lines 386-392):**
```cpp
generativeMouth.setAudioLevel (audioLevel);
auto vowel = processorRef.getCurrentVowelShape();
generativeMouth.setVowel (static_cast<GenerativeMouth::Vowel> (vowel));
generativeMouth.setMorph ((float) morphKnob.getValue());
```

**Timer Polling (Line 189):**
```cpp
startTimerHz (30);  // Editor polls DSP state at 30 FPS
```

Note: Editor runs at 30 FPS for smooth UI responsiveness, but GenerativeMouth internally throttles to 10 FPS for stutter aesthetic. This is correct architecture.

#### File Locations for Implementation

**Source Files to Create:**
- `source/ui/MuseLookAndFeel.h` - Adapted from FieldLookAndFeel_v2.h (header-only or with .cpp)
- `source/ui/MuseLookAndFeel.cpp` - Implementation (if needed, can be header-only)
- `source/ui/GenerativeMouth.h` - Copied from reference, modified colors

**Files to Modify:**
- `source/PluginEditor.h` - Replace `OLEDLookAndFeel oledLookAndFeel;` with `MuseLookAndFeel museLookAndFeel;`
- `source/PluginEditor.cpp` - Update setLookAndFeel call

**Files Already Complete (No Changes Needed):**
- `source/ui/MuseColors.h` - Complete design tokens
- `design/design-tokens.json` - Source of truth
- `design/UI-SPECIFICATION.md` - Complete visual spec

**Assets to Import:**
- Mouth image assets (if used for loading placeholder)
- Location in project: `assets/images/mouth/` (create if needed)
- Note: Procedural mouth is primary, images are reference/fallback only

#### CMake Configuration (No Changes Required)

The project already uses JUCE 8 with proper asset embedding via BinaryData (CMakeLists.txt, line 127):
```cmake
include(Assets)
```

If mouth images are added to `assets/images/mouth/`, they'll be automatically embedded. No CMakeLists.txt changes needed.

#### JUCE 8 Best Practices Applied

**LookAndFeel Inheritance:**
- Inherit from `juce::LookAndFeel_V4` (modern JUCE 8 base)
- Override specific draw methods (drawRotarySlider, drawButtonBackground, etc.)
- Use setColour() in constructor for default colors

**Component Architecture:**
- GenerativeMouth inherits from `juce::Component` and `juce::Timer`
- Timer runs on message thread (safe for UI updates)
- No direct audio thread access (all data via atomics in PluginProcessor)

**Thread Safety:**
- PluginEditor timer polls atomics at 30 FPS (line 189)
- Atomics declared in PluginProcessor.h (lines 64, 79-81)
- No locks, no blocking, pure read operations

**Color Constants:**
- Use `inline const juce::Colour` (C++17, no ODR violations)
- Namespace organization (Muse::Colors::)
- Helper functions for gradients, shadows

#### Build and Test Strategy

**Compile Check:**
```bash
cmake --build build --config Release --target Muse
```

**Visual Verification:**
1. Launch standalone: `build/Muse_artefacts/Release/Standalone/Muse.exe`
2. Verify knobs are flat warm taupe (not 3D gradients)
3. Verify mouth glows with Muse colors (not mint green)
4. Verify 10 FPS stutter aesthetic (should feel intentional, not smooth)
5. Verify mouth responds to audio (louder = wider opening)

**Regression Test:**
- Ensure parameter changes still work (knobs attached to APVTS)
- Ensure preset dropdown still functions
- Ensure vowel shape updates when shape pair changes
- Ensure no performance degradation (GenerativeMouth is lightweight)

#### Potential Gotchas

**Don't:**
- Change GenerativeMouth algorithm (preserve asymmetry, jitter, teeth logic)
- Smooth the 10 FPS updates (stutter is intentional)
- Add gradients to knobs (Muse is flat brutalist, not skeuomorphic)
- Use OLEDLookAndFeel colors in new code
- Modify MuseColors.h constants without updating design-tokens.json

**Do:**
- Preserve all GenerativeMouth::generate*() functions exactly
- Keep timer at 10 FPS (line 50: `startTimerHz(10)`)
- Use Muse::Colors namespace consistently
- Follow flat rendering patterns from SeanceLookAndFeel (already in codebase)
- Test mouth at different audio levels and vowel shapes

#### Open Design Questions

**Mouth Glow Color:**

Options:
1. Lilac (0xFFC8B6D8) - matches accent gradient start, cooler
2. Peach (0xFFFFD4C4) - matches accent gradient end, warmer
3. Gradient (breathing pulse) - use createAccentGradient() per pixel
4. Custom warm glow (0xFFE8D9C4?) - between peach and linen

Recommendation: Start with Lilac (option 1) for consistency with active knob states. If it feels too cool, add a custom "MouthGlow" color to MuseColors.h.

**Mouth Size:**

Current: 16x6 grid inside 300x150 black OLED screen (PluginEditor.cpp line 301)
Muse spec: 640x480 plugin (not 400x600)

Should mouth be:
- Larger grid (20x8?) for higher resolution at 640px width
- Keep 16x6 but scale up cell size
- Procedural vector rendering instead of grid

Recommendation: Keep 16x6 grid initially (proven aesthetic), but render at larger cell size for 640px canvas.

**Image Assets Usage:**

The screen.png assets in Downloads/ - are they:
- Static fallback for loading states
- Design references only
- Prototype mockups

Recommendation: Treat as reference only. Keep procedural mouth primary. May add static loading placeholder later.

## Context Files

## User Notes

**Code Review Results (completed):**
- ✅ GenerativeMouth.h - IMPORT WITH CHANGES (perfect 10 FPS aesthetic, needs color fix)
- ❌ SpringValue.h - REJECT (smooth motion contradicts stutter aesthetic)
- ⚠️ FieldLookAndFeel_v2 - IMPORT WITH CHANGES (good structure, wrong colors)

**Source Files:**
- `C:\Muse\MuseAudio-UI-Reference\muse-legacy-ui\GenerativeMouth.h`
- `C:\Muse\MuseAudio-UI-Reference\enginefield-ui\FieldLookAndFeel_v2.h/cpp`
- `C:\Users\hooki\Downloads\stitch_muse_default_shape\stitch_muse_default_shape\muse_default_shape\` (mouth assets)

**Existing:**
- `source/ui/MuseColors.h` - Already has complete Muse design tokens
- `design/design-tokens.json` - Source of truth
- `design/UI-SPECIFICATION.md` - Complete visual spec

## Work Log
- [2025-11-08] Task created after code-review subagent analysis
- [2025-11-08] MuseColors.h already exists with complete design tokens
- [2025-11-08] Context-gathering agent created comprehensive 530-line manifest
- [2025-11-08] GenerativeMouth.h adapted - OLEDLookAndFeel removed, lilac glow added, teeth disabled (commit d15cfda)
- [2025-11-08] Created MuseLookAndFeel.h (flat brutalist knobs, warm Muse palette)
- [2025-11-08] Updated PluginEditor to use MuseLookAndFeel, changed size to 640×480
- [2025-11-08] Fixed GenerativeMouth rendering: squares→circles, removed chaos, 70% dot size
- [2025-11-08] Created EMULCDMouth.h (lime LCD #C3F73A + black halftone dots)
- [2025-11-08] **PIVOT TO DESIGN-FIRST**: Created comprehensive Figma design prompt (500+ lines, 4 theme variants)
- [2025-11-08] User analyzed reference UIs (UAD/FabFilter, iZotope Ozone, Valhalla DSP)
- [2025-11-08] All builds successful (Release), components ready for design finalization
- [2025-11-08] **SESSION PAUSED**: Awaiting Figma mockups to choose final aesthetic direction
