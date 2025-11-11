---
task: h-implement-hardware-ui-from-tokens
branch: feature/implement-hardware-ui-from-tokens
status: pending
created: 2025-11-09
modules: [ui]
---

# Implement Hardware UI from Figma Design Tokens

## Problem/Goal

Implement the complete hardware-inspired UI for Muse using the validated design tokens extracted from Figma. Replace the current placeholder UI with production-ready components that match the exact specifications from `design-tokens.json`.

**Design Vision**: Azure/slate hardware aesthetic with chartreuse LCD display, matching the "Brutalist Temple" philosophy.

## Success Criteria

- [ ] Create `source/ui/MuseDesignTokens.h` header from JSON tokens (colors, sizing, shadows, effects)
- [ ] Implement hardware LCD display panel component with:
  - Chartreuse (#D9F99D @ 50% opacity) background
  - Inner shadow effect (y=2.344, blur=3.516)
  - Grid pattern overlay (if needed for authentic LCD look)
  - Exact dimensions: 402×129px
- [ ] Implement LED indicator component with:
  - 18×18px circular shape
  - Red glow effect with drop shadow
  - Border radius: 9px
- [ ] Implement three hardware button components with:
  - Correct sizing (70×59, 70×59, 66×59)
  - Base color: #94A3B8, rim: #64748B
  - Drop shadow (y=1.625, blur=1.625) + inner shadow (y=4.0625, blur=4.0625)
- [ ] Update `PluginEditor` with exact layout from tokens:
  - Panel: 669×187px
  - LED position from tokens
  - LCD position from tokens
  - Button positions from tokens
- [ ] Apply all shadow effects correctly (panel drop shadow: blur=50, y=25)
- [ ] Build and verify in standalone - UI matches Figma pixel-perfect
- [ ] Clean up any old UI files (confirm with user first)

## Design Token Source

Primary reference: `design-tokens.json` (user-provided, extracted from `figma.svg`)

Key tokens structure:
```json
{
  "sizing": { "panel.width": 669, "lcd.width": 402, ... },
  "color": { "surface.panel": "#CBD5E1", "surface.lcd.base": "#D9F99D80", ... },
  "shadow": { "panel.drop": {...}, "lcd.inner": {...}, "knob.drop": {...} },
  "borderRadius": { "led": 9, "slider.track": 10 },
  "gradient": { "lcd.sheen.horizontal": {...}, "lcd.sheen.vertical": {...} }
}
```

## Context Manifest

### How the UI System Currently Works: Multiple Competing Aesthetics

The Muse codebase currently contains **FIVE different UI aesthetic implementations**, each representing a different design exploration. This is not a bug - it's the result of iterative design exploration where multiple themes were prototyped before settling on a final direction.

**The Active Implementation** (what users see when they build the plugin):

When you build and run Muse right now, `PluginProcessor::createEditor()` (line 410 in PluginProcessor.cpp) returns `new PluginEditor(*this)`, which instantiates the **Hardware Azure/Slate UI** defined in `source/PluginEditor.cpp`. This implementation:

1. **Size**: 669×239px (compact hardware panel, NOT the canonical 640×480 temple design)
2. **Color Scheme**: Azure/slate tones (#CBD5E1 background) with chartreuse LCD display (#D9F99D)
3. **Components**:
   - `HardwareLEDIndicator` (18×18px red LED at top-left)
   - `ChartreuseDisplayPanel` (402×129px green LCD panel, left side)
   - Three `HardwareButton` instances (70×59px each, right side)
4. **LookAndFeel**: `HardwareUILookAndFeel` (minimal, sets background and label fonts)
5. **Paint Order**: Shadow first (underneath) → Azure background with rounded corners → white border overlay → title text → decorative elements

The layout is defined in `PluginEditor::resized()` (lines 62-79) using **hardcoded pixel values** matching the Figma hardware design. The background is painted in `PluginEditor::paint()` (lines 27-60) with drop shadow effects, rounded rectangles, and typography.

**File Dependencies**:
- `source/PluginEditor.h` - Editor class declaration
- `source/PluginEditor.cpp` - Layout and rendering implementation
- `source/ui/HardwareUILookAndFeel.h` - Custom LookAndFeel (sets colors, label font)
- `source/ui/HardwareUIColors.h` - Color constants (Azure palette, chartreuse, LED red)
- `source/ui/HardwareLEDIndicator.h` - Red LED component with glow
- `source/ui/ChartreuseDisplayPanel.h` - LCD panel with inner shadow and grid pattern
- `source/ui/HardwareButton.h` - Round push button component

**CRITICAL ARCHITECTURAL NOTE**: These components are **stack-allocated members** (not pointers), following JUCE 8 best practices. The `HardwareUILookAndFeel` is owned by the editor and set/cleared in constructor/destructor using `setLookAndFeel(&hardwareLookAndFeel)` and `setLookAndFeel(nullptr)`.

---

### The Other Four UI Systems (Currently Dormant):

#### 1. **Warm Brutalist Temple** (The Canonical Muse Aesthetic)

This is the OFFICIAL brand direction defined in `design/UI-SPECIFICATION.md` (locked 2025-11-03) and codified in `design/design-tokens.json` (318 lines). This is what the brand documentation says Muse SHOULD look like.

**Design System Files**:
- `design/design-tokens.json` - Complete token system (colors, spacing, typography, effects, sizing, layout)
- `design/UI-SPECIFICATION.md` - Locked visual specification
- `design/FIGMA-DESIGN-PROMPT.md` - Original design brief
- `source/ui/MuseColors.h` - JUCE implementation of design tokens (11KB, comprehensive)
- `source/ui/MuseLookAndFeel.h` - LookAndFeel implementation (flat brutalist knobs, NO gradients)

**Key Characteristics**:
- **Size**: 640×480px (4:3 ratio, NOT the current 669×239)
- **Colors**: Warm linen background (#FAF0E6 at 85% over dark texture #343A40 at 15%), lilac/peach accent gradient (#C8B6D8 → #FFD4C4)
- **Philosophy**: "Dead simple mask over complex engine", generous negative space, floating cards with shadows
- **Typography**: System sans-serif for UI, Georgia serif for Muse's voice
- **Components**: 80px flat knobs, logo card, transmission area (bottom 20%), "mouth" visualizer

**Fully Implemented Components**:
- `source/ui/MuseColors.h` - Complete color system with helper functions (`createAccentGradient()`, `createCardShadow()`, `drawCardWithShadow()`)
- `source/ui/MuseLookAndFeel.h` - Full LookAndFeel with flat knob rendering (NO radial gradients, simple indicator line)
- `source/ui/MuseKnob.h` - Knob component
- `source/ui/MuseTransmission.h` - Transmission area component
- `source/ui/GenerativeMouth.h` - Procedural mouth visualizer
- `source/ui/StatusBar.h` - Status/info bar

**Why This Isn't Active**: The codebase has been exploring more compact, hardware-inspired layouts. The warm temple design is canonical but currently unused.

#### 2. **OLED Dark Theme**

Retro hardware synthesizer aesthetic with glowing indicators.

**Files**:
- `source/ui/OLEDLookAndFeel.h` - 3D skeuomorphic knobs with radial gradients
- `source/ui/OLEDScreen.h` - Black OLED display
- `source/ui/OLEDMouth.h` - Mouth visualizer for OLED
- `source/ui/SeanceColors.h` / `SeanceLookAndFeel.h` - Dark mystical variant

**Colors**: Dark teal (#2F4F4F), mint green (#d8f3dc), black OLED screen
**Knob Style**: Radial gradient (light top-left → dark bottom-right), rotating mint dot, glowing indicator line, inset shadow

#### 3. **EMU LCD Hardware**

Authentic E-mu Audity hardware lineage with lime green LCD.

**Files**:
- `source/ui/EMULCDPanel.h` - Lime green LCD panel with scanlines
- `source/ui/EMULCDMouth.h` - Black halftone dot mouth on lime background
- `source/ui/HalftoneDisplayPanel.h` - Halftone pattern implementation
- `source/ui/HalftoneMouth.h` - Halftone mouth visualizer

**Colors**: LCD lime green (#C3F73A), black mouth dots, dark warm brown bezel
**Philosophy**: Hardware authenticity, retro synth workstation feel

#### 4. **Modern Hardware (Currently Active)**

The compact hardware panel UI that's currently being used.

**Files**: Already listed above in "Active Implementation" section.

**Additional Components** (from Builder.io generation, may need refinement):
- `source/ui/HardwareColors.h` - Original hardware colors (different namespace from HardwareUIColors)
- `source/ui/HardwareKnob.h` - Hardware knob with inset shadow (95×95px)
- `source/ui/HardwareSlider.h` - Horizontal slider (169×20px)
- `source/ui/DisplayPanel.h` - Alternative display panel (with checkerboard)
- `source/ui/LEDIndicator.h` - Alternative LED implementation

**NOTE**: There are TWO sets of hardware components:
- `Hardware::Colors` namespace (HardwareColors.h) - Original implementation
- `HardwareUI::Colors` namespace (HardwareUIColors.h) - Builder.io generated

This duplication suggests the hardware UI was iterated multiple times.

---

### JUCE 8 Architecture Patterns Used Throughout

All UI implementations follow these JUCE 8 best practices:

**1. LookAndFeel Lifecycle Management**:
```cpp
// In constructor
setLookAndFeel(&customLookAndFeel);

// In destructor
setLookAndFeel(nullptr);  // CRITICAL: Prevents dangling pointer
```

Every editor owns its LookAndFeel as a member variable, not a pointer. This is visible in `PluginEditor.h` (line 32): `HardwareUILookAndFeel hardwareLookAndFeel;`

**2. Component Ownership**:
All UI components are stack-allocated members, NOT pointers:
```cpp
// In PluginEditor.h (lines 35-39)
HardwareLEDIndicator ledIndicator;
ChartreuseDisplayPanel displayPanel;
HardwareButton button1;
HardwareButton button2;
HardwareButton button3;
```

Added in constructor with `addAndMakeVisible()`, automatically cleaned up on destruction. No manual memory management.

**3. Paint Order**:
```cpp
// PluginEditor::paint() shows canonical order:
1. Draw shadow effects FIRST (underneath, using translated bounds)
2. Fill background shapes
3. Draw borders/overlays
4. Render text on top
```

**4. Sizing**:
Size is set ONCE in constructor (PluginEditor.cpp line 18): `setSize(669, 239);`

**5. Layout**:
Component bounds are set in `resized()` using explicit pixel values. No layout managers are used (simple enough for hardcoded positioning).

**6. Custom Rendering**:
LookAndFeel classes override methods like:
- `drawRotarySlider()` - Custom knob rendering
- `getLabelFont()` - Typography control
- `drawButtonBackground()` / `drawButtonText()` - Button styling

Each LookAndFeel sets default colors in its constructor using `setColour(ComponentId::colourId, colour)`.

---

### Design Token Architecture: JSON → C++ Constants

The `design/design-tokens.json` file defines the complete design system for the **Warm Brutalist Temple** aesthetic. It follows the W3C Design Tokens Community Group format (https://tr.designtokens.org/format/).

**Token Categories**:
```json
{
  "muse": {
    "color": { ... },           // Background, accent, text, transmission, knob colors
    "spacing": { ... },         // 8px base unit, section/control/label spacing, padding
    "typography": { ... },      // Font families, sizes, weights, letter-spacing
    "effects": { ... },         // Shadows, blur, animation timings
    "sizing": { ... },          // Plugin dimensions, knob size, logo size
    "layout": { ... }           // Grid columns, gutters
  }
}
```

**JUCE Implementation** in `source/ui/MuseColors.h`:

The token system is translated into C++ constants organized by namespace:
```cpp
namespace Muse {
    namespace Colors {
        inline const juce::Colour TextPrimary { 0xFF5C5552 };
        inline juce::ColourGradient createAccentGradient(juce::Rectangle<float> bounds) { ... }
        inline juce::DropShadow createCardShadow() { ... }
    }
    namespace Typography {
        constexpr float LabelSize = 11.0f;
        inline juce::Font getLabelFont() { ... }
    }
    namespace Layout {
        constexpr int Unit = 8;
        constexpr int KnobDiameter = 80;
    }
    namespace Animation {
        constexpr int StutterFPS = 10;
        constexpr int TransitionNormal = 300;
    }
}
```

This provides:
- **Type safety**: Colors are `juce::Colour`, not strings
- **Helper functions**: `createAccentGradient()`, `drawCardWithShadow()` encapsulate common patterns
- **Compile-time constants**: No runtime lookups, optimal performance
- **IntelliSense support**: Autocomplete for all design tokens

**Why This Matters**: The design-tokens.json is the SOURCE OF TRUTH for the canonical Muse aesthetic. Any UI implementation claiming to follow the "Muse design system" should use values from this file, translated through MuseColors.h.

---

### The Design System Confusion: Two Competing Visions

**Here's the critical tension this task needs to resolve**:

**Vision A: Warm Brutalist Temple** (design-tokens.json, MuseColors.h, UI-SPECIFICATION.md)
- 640×480px, 4:3 ratio
- Warm linen (#FAF0E6) with lilac/peach accents
- Generous negative space, floating cards
- "Dead simple mask over complex engine"
- Serif font for Muse's voice
- THIS IS DOCUMENTED AS THE CANONICAL BRAND

**Vision B: Hardware Azure/Slate Panel** (current PluginEditor, HardwareUIColors.h)
- 669×239px, compact panel
- Azure/slate (#CBD5E1) with chartreuse display
- Tight hardware layout, no floating cards
- Chakra Petch font
- THIS IS WHAT'S CURRENTLY IMPLEMENTED

**The Task Says**: "Implement hardware-inspired UI from Figma design tokens"

**The Problem**:
1. There is NO figma.svg file in the repo
2. The design-tokens.json contains Warm Temple specs, NOT hardware specs
3. The current PluginEditor already implements hardware UI (but with hardcoded values, not from tokens)

**Two Possible Interpretations**:

**Interpretation 1**: Refactor the current hardware UI to use a proper token system
- Create `HardwareDesignTokens.h` with the Azure/slate/chartreuse values
- Extract all hardcoded dimensions from PluginEditor.cpp into constants
- Make the existing hardware UI more maintainable

**Interpretation 2**: Replace the hardware UI with the canonical Warm Temple design
- Switch PluginEditor to use MuseColors.h and MuseLookAndFeel.h
- Implement the 640×480 floating card layout
- Align the code with the documented brand vision

**Recommendation**: Clarify with the user which direction they want. The task description mentions "chartreuse LCD display" and "hardware buttons", which matches the current implementation (Interpretation 1). But the reference to design-tokens.json suggests the canonical warm temple (Interpretation 2).

---

### Build System Integration

**CMake Configuration** (CMakeLists.txt lines 112-117):
```cmake
file(GLOB_RECURSE SourceFiles CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h")
target_sources(SharedCode INTERFACE ${SourceFiles})
```

**What This Means**:
- ALL `.h` and `.cpp` files in `source/` (including `source/ui/`) are automatically included
- `CONFIGURE_DEPENDS` triggers CMake reconfiguration when files are added/removed
- No need to manually edit CMakeLists.txt when creating new UI components
- Just create a `.h` file in `source/ui/` and it's automatically compiled

**Implications for This Task**:
- Creating `MuseDesignTokens.h` or `HardwareDesignTokens.h` will be picked up automatically
- Refactoring existing components won't require build system changes
- Can safely create new component files without breaking the build

---

### Shadow and Effect Implementation Patterns

All UI themes use JUCE's graphics primitives for shadows and effects. Here are the established patterns:

**Drop Shadow** (painted BEFORE the element):
```cpp
// From PluginEditor::paint() line 32
g.setColour(HardwareUI::Colors::ShadowMain);  // 40% opacity dark
g.fillRoundedRectangle(bounds.reduced(-12.5f).translated(0.0f, 12.5f), 8.0f);
// Then paint the actual element on top
```

**Inner Shadow** (painted AFTER background fill, BEFORE border):
```cpp
// From ChartreuseDisplayPanel::paint() line 18
g.setColour(juce::Colours::black.withAlpha(0.4f));
g.fillRoundedRectangle(bounds.reduced(1.0f).translated(0.0f, 2.0f), 7.0f);
g.setColour(HardwareUI::Colors::DisplayBackground);
g.fillRoundedRectangle(bounds, 7.0f);  // Actual fill
```

**Glow Effect** (expanded bounds with alpha):
```cpp
// From HardwareLEDIndicator::paint() line 18
g.setColour(juce::Colours::black.withAlpha(0.25f));
g.fillEllipse(bounds.translated(0.0f, 4.0f));  // Shadow
// OR for glow:
g.setColour(Hardware::Colors::LEDGlow);
g.fillEllipse(bounds.expanded(4.0f));  // Glow extends beyond bounds
```

**Using juce::DropShadow** (helper in MuseColors.h):
```cpp
// From MuseColors.h line 133-143
inline juce::DropShadow createCardShadow() {
    return juce::DropShadow(ShadowCard, 8, juce::Point<int>(0, 2));
}

// Usage in component:
auto shadow = Muse::Colors::createCardShadow();
shadow.drawForRectangle(g, bounds.toNearestInt());
```

**Design Token Specs** (from design-tokens.json):
- Panel drop shadow: `y=25px, blur=50px`
- LCD inner shadow: `y=2.344px, blur=3.516px`
- Knob drop shadow: `y=1.625px, blur=1.625px`
- Button inner shadow: `y=4.0625px, blur=4.0625px`

If implementing from tokens, these exact values should be used (currently approximated in existing code).

---

### Component Communication and Parameter Binding

The UI does NOT directly control DSP parameters. All parameter handling flows through JUCE's AudioProcessorValueTreeState (APVTS).

**Architecture** (from PluginProcessor.h lines 42, 98-106):
```cpp
// PluginProcessor owns the state
juce::AudioProcessorValueTreeState state_;

// Cached pointers for RT-safe audio thread access
std::atomic<float>* pairParam_ = nullptr;
std::atomic<float>* morphParam_ = nullptr;
// etc.
```

**Parameters Defined** (PluginProcessor.cpp, createParameterLayout):
- `pair` - Shape pair selector (0-3 integer)
- `morph` - A/B morph amount (0-1 float)
- `intensity` - Resonance strength (0-1 float)
- `mix` - Wet/dry blend (0-1 float)
- `autoMakeup` - Auto gain compensation (boolean)

**UI Binding Pattern** (NOT YET IMPLEMENTED IN CURRENT HARDWARE UI):

The current PluginEditor (hardware version) has NO parameter attachments. It's a static mockup.

For functional parameter control, components would need:
```cpp
// In editor header
std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> morphAttachment;

// In constructor
morphAttachment = std::make_unique<SliderAttachment>(
    processorRef.getState(), "morph", morphKnob);
```

This is visible in the warm temple components like MuseKnob, but NOT in the current active hardware UI.

**Implication**: If implementing hardware UI from tokens, you'll need to ADD parameter bindings to make the controls functional. The current implementation is visual only.

---

### File Organization and Naming Conventions

All UI components follow consistent patterns:

**Header-Only Components**:
- All UI components are header-only (`.h` files, no `.cpp`)
- Classes inherit from `juce::Component` or `juce::Slider`
- `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClassName)` at end
- Default constructors, custom `paint()` overrides

**Naming Patterns**:
- `[Theme]Colors.h` - Color constants (e.g., HardwareColors, MuseColors)
- `[Theme]LookAndFeel.h` - Custom rendering (e.g., OLEDLookAndFeel, MuseLookAndFeel)
- `[Component][Theme].h` - Theme-specific components (e.g., EMULCDPanel, HardwareKnob)
- Generic components: `[Component].h` (e.g., LEDIndicator, DisplayPanel)

**Color System Organization**:
```cpp
namespace ThemeName {
    namespace Colors {
        inline const juce::Colour ColorName { 0xFFRRGGBB };
    }
}
```

All colors use `inline const` for zero runtime cost.

**Current File Count**: 32 files in `source/ui/` directory (as of 2025-11-09)

---

### Next Steps for Implementation

Based on this context, here are the recommended paths forward:

**Path A: Refactor Hardware UI with Token System** (if task wants to keep current aesthetic)
1. Create `source/ui/HardwareDesignTokens.h` with all Azure/slate/chartreuse values
2. Extract dimensions from PluginEditor.cpp into constants
3. Refine shadow effects to match exact specifications
4. Add parameter attachments to make controls functional
5. Document the hardware aesthetic as an official theme

**Path B: Implement Canonical Warm Temple** (if task wants to align with brand docs)
1. Update `PluginProcessor::createEditor()` to return warm temple editor
2. Create new editor class using MuseColors.h and MuseLookAndFeel.h
3. Implement 640×480 layout with floating cards
4. Add transmission area and mouth visualizer
5. Remove or archive hardware UI components

**Path C: Support Multiple Themes** (maximum flexibility)
1. Create theme selection mechanism (compile-time or runtime)
2. Standardize token format for all themes
3. Make each theme complete and functional
4. Document when to use each aesthetic

**Questions to Resolve**:
1. Which aesthetic should be the default?
2. Should design-tokens.json remain warm temple only, or expand to cover all themes?
3. Are the hardware components from Builder.io temporary, or intended for refinement?
4. What does "Figma design tokens" refer to if there's no figma.svg?

## Context Files

## User Notes

- User has already validated the design tokens are accurate and complete
- Current branch: `ui/theme-system` (may need to reconcile or create new branch)
- Existing files in `source/ui/`:
  - `HardwareUIColors.h` (Builder.io generated - may be replaced)
  - `HardwareLEDIndicator.h` (Builder.io generated - may be refined)
  - `ChartreuseDisplayPanel.h` (Builder.io generated - may be refined)
  - `HardwareButton.h` (Builder.io generated - may be refined)
  - `HardwareUILookAndFeel.h` (Builder.io generated - may be refined)
- JUCE 8.0.10 best practices required (proper lifecycle, no manual memory management)
- NO emojis in code or UI unless explicitly requested
- Follow CLAUDE.md guidelines for UI architecture

## Technical Approach

1. **Convert tokens to C++ header** - Create type-safe constants from JSON
2. **Refine existing components** - Use Builder.io files as starting point, apply exact token values
3. **Implement shadow/effect system** - JUCE DropShadow and Graphics effects for all visual polish
4. **Layout precision** - Use token values directly in `resized()`, no magic numbers
5. **Verify visual match** - Build, compare to Figma side-by-side

## Work Log

- [2025-11-09] Task created, pending context gathering
- [2025-11-09] Context manifest completed - documented all five UI aesthetics, identified design system tension between Warm Temple (canonical) and Hardware Azure/Slate (current), recommended three implementation paths
