# Muse Codebase Setup - Complete ✓

**Date**: 2025-11-03
**Status**: Ready for UI Development

---

## What Was Accomplished

### 1. Design System Foundation ✓

Created complete visual specification to prevent "getting tripped up by UI":

```
design/
├── design-tokens.json          (318 lines) - W3C standard tokens
├── UI-SPECIFICATION.md         (454 lines) - Complete visual spec
└── README.md                   (75 lines)  - Navigation guide

source/ui/
└── MuseColors.h                (291 lines) - JUCE-ready constants
```

**Key Decision Locked**: Option 3 - Subtle dark texture (15% opacity) under warm linen overlay (85% opacity)

### 2. CLAUDE.md Optimized ✓

**Before**: 452 lines with verbose UI section
**After**: 377 lines, condensed, references design docs

**Sections**:
- ✅ Brand identity ("Dogmatic Genius" archetype)
- ✅ Build system (cmake commands)
- ✅ Architecture (DSP pipeline flow)
- ✅ Parameters (current + future)
- ✅ UI philosophy (brief, points to design/)
- ✅ Code guidelines
- ✅ Branding rules (never mention E-mu/Z-plane)

### 3. Design Token System ✓

**Color Palette** (warm, not "dark plugin with cyan"):
- `#FAF0E6` - Warm linen background overlay
- `#343A40` - Dark texture base (15% opacity)
- `#5C5552` - Warm taupe text/knobs
- `#C8B6D8 → #FFD4C4` - Lilac-to-peach gradient (the magic)
- `#E6D9F0` - Soft lilac transmission area

**Typography**:
- UI: System sans-serif, clean, geometric
- Muse's voice: Georgia serif (personal)
- Sizes: 11px labels, 16px transmission, 14px values

**Layout**:
- 640×480px (4:3 ratio)
- 8px base unit (all spacing in multiples)
- 80px knobs, 32px spacing

**Animation**:
- Stutter-frame: 10 fps, 400ms reveal
- Breathing pulse: 3s cycle
- Transitions: 150ms fast, 300ms normal

---

## File Structure

```
C:\Muse\MuseAudio\
├── CLAUDE.md                   ← Optimized context (377 lines)
├── CMakeLists.txt              ← Build config (Pamplejuce)
├── design/                     ← NEW: Complete design system
│   ├── README.md
│   ├── design-tokens.json
│   └── UI-SPECIFICATION.md
├── dsp/                        ← Z-plane filter (complete, working)
│   ├── EMUAuthenticTables.h    ← Authentic E-mu pole data
│   ├── ZPlaneEngineV2.hpp/cpp  ← 6-stage biquad cascade
│   ├── ZPlaneShapes.hpp/cpp    ← JSON loader + fallback
│   └── ZPlaneFilter.h          ← Core biquad implementation
├── source/
│   ├── PluginProcessor.h/cpp   ← JUCE 8, APVTS, working
│   ├── PluginEditor.h/cpp      ← Minimal (needs Muse UI)
│   └── ui/                     ← NEW: UI components folder
│       └── MuseColors.h        ← JUCE color constants
├── shapes/                     ← JSON shape data (48kHz)
├── assets/images/              ← Logo + texture assets
├── tests/                      ← Catch2 tests
└── benchmarks/                 ← Performance benchmarks
```

---

## Next Steps for UI Development

### Phase 1: Foundation (2-3 days)
1. Implement layered background (texture + overlay)
2. Create custom knob component using `MuseColors.h`
3. Add logo card (floating, top-left)
4. Test the aesthetic - does it feel "haunted"?

### Phase 2: Controls (1-2 days)
5. Wire 4 knobs to APVTS (Morph, Haunt, Focus, Drift)
6. Add parameter labels (uppercase, proper spacing)
7. Create "Ask Muse" button with gradient states
8. Implement right-click value display (hidden by default)

### Phase 3: Personality (2-3 days)
9. Build transmission area card
10. Implement stutter-frame text component (10 fps)
11. Add loading mutterings (random selection)
12. Add success/error verdicts
13. Implement breathing pulse on logo

### Phase 4: Polish (1-2 days)
14. Add all shadows and blur effects
15. Fine-tune animation timings
16. Test all interaction states
17. Ensure keyboard navigation works

---

## Build Commands

```bash
# Initial setup (first time only)
git submodule update --init --recursive

# Configure
cmake -B build -G "Visual Studio 17 2022" -A x64

# Build
cmake --build build --config Release

# Test
cd build && ctest -C Release
```

---

## Key Philosophy Reminders

1. **User is petitioner, Muse is Oracle** - Not helpful software, powerful artifact
2. **Prime Directive**: Absence of cognitive load - user should FEEL, not think
3. **Personality through timing, not blocking** - Never break functionality for whimsy
4. **The Law of Masterclass Output** - Results must be flawless, even if personality is chaotic
5. **Never mention E-mu/Z-plane in marketing** - It's just "Muse, The Dimensional Filter"

---

## Current Status

✅ **DSP**: Complete, working, authentic E-mu Z-plane implementation
✅ **Build System**: Pamplejuce, JUCE 8, CMake configured
✅ **Parameters**: 5 params defined (pair, morph, intensity, mix, autoMakeup)
✅ **Design System**: Complete, locked, documented
⏳ **UI**: Minimal (needs implementation using design system)
⏳ **Presets**: Need eccentric naming ("Trying Too Hard", etc.)
⏳ **Testing**: Basic tests exist, need UI validation tests

---

## Design System Usage

**For Color**:
```cpp
#include "ui/MuseColors.h"

// Use constants
g.setColour(Muse::Colors::TextPrimary);

// Use gradients
auto gradient = Muse::Colors::createAccentGradient(bounds);
g.setGradientFill(gradient);

// Use helpers
auto shadow = Muse::Colors::createCardShadow();
```

**For Layout**:
```cpp
using namespace Muse::Layout;

int knobSize = KnobDiameter;      // 80px
int spacing = ControlSpacing;     // 32px
int margin = PaddingLarge;        // 40px
```

**For Typography**:
```cpp
auto labelFont = Muse::Typography::getLabelFont();  // 11px, tracked
auto voiceFont = Muse::Typography::getMuseVoiceFont();  // 16px serif
```

---

## Documentation Map

- **Build/Architecture**: `CLAUDE.md`
- **UI Visual Spec**: `design/UI-SPECIFICATION.md`
- **Design Tokens**: `design/design-tokens.json`
- **JUCE Colors**: `source/ui/MuseColors.h`
- **Design Navigation**: `design/README.md`
- **Project Overview**: `README.md` (Pamplejuce)

---

## This Setup Prevents

❌ Inconsistent colors hardcoded in components
❌ UI drift from brand vision
❌ "Another dark plugin with cyan accents"
❌ Forgetting the personality (petitioner/oracle)
❌ Breaking functionality for whimsy

✅ Single source of truth for all visual decisions
✅ JUCE-ready constants, no guesswork
✅ Complete specs with implementation examples
✅ Warm, unique aesthetic (not generic dark)
✅ Clear brand identity preserved

---

**Ready to start UI implementation using the locked design system.**
