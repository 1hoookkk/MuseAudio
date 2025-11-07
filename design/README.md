# Muse Design System

This directory contains the complete visual design specification for the Muse audio plugin.

## Files

### `UI-SPECIFICATION.md` (454 lines)
Complete visual specification including:
- Color palette (warm linen + subtle dark texture)
- Typography system
- Component specifications (knobs, transmission area, logo card)
- Layout grid and spacing
- Animation timings and effects
- Implementation checklist with JUCE code examples

**This is the source of truth for all UI decisions.**

### `design-tokens.json` (318 lines)
Machine-readable design tokens in W3C Design Tokens Community Group format.
- Colors (background system, text, accents, shadows)
- Spacing (base unit: 8px, all multiples)
- Typography (sizes, weights, tracking)
- Effects (shadows, animations, blur)
- Sizing (plugin dimensions, component sizes)

Use this for token-based design systems or tooling.

### `../source/ui/MuseColors.h` (291 lines)
JUCE-ready C++ constants generated from design tokens.
- `Muse::Colors` namespace with all color constants
- `Muse::Typography` namespace with font helpers
- `Muse::Layout` namespace with spacing/sizing constants
- `Muse::Animation` namespace with timing constants
- Helper functions for gradients, shadows, and effects

**Include this in all UI components for consistent styling.**

## Quick Reference

### The Locked Visual Choice: Option 3

**Subtle Dark Texture Under Warm Overlay**

```
Layer 1 (Base):    Dark concrete texture (#343A40 at 15% opacity)
Layer 2:           Warm linen overlay (#FAF0E6 at 85% opacity)
Result:            Warm, sophisticated background with haunted depth
```

This is NOT "another dark plugin with cyan accents."
This is Anthropic-style warm palette with mysterious depth.

### Core Color Palette

| Token | Hex | Usage |
|-------|-----|-------|
| Warm Linen | #FAF0E6 | Background overlay |
| Warm Taupe | #5C5552 | Primary text, knobs |
| Lilac | #C8B6D8 | Accent gradient start |
| Peach | #FFD4C4 | Accent gradient end |
| Soft Lilac | #E6D9F0 | Transmission background |

### Typography

- **UI Labels**: 11px, system sans-serif, 0.08em tracking, uppercase
- **Muse's Voice**: 16px, Georgia serif (more personal)
- **Values**: 14px (hidden by default, right-click to show)

### Key Measurements

- Plugin: 640×480px (4:3 ratio)
- Knobs: 80px diameter, 32px spacing
- Base unit: 8px (all spacing in multiples)
- Margins: 40px
- Section spacing: 48px

### Stutter-Frame Text

- FPS: 10 (otherworldly but readable)
- Duration: 400ms total
- Effect: Low-fps transmission reveal

## Implementation Notes

1. **Always use MuseColors.h** - Never hardcode colors in components
2. **Reference UI-SPECIFICATION.md** - For detailed component specs
3. **Follow the grid** - All spacing in 8px multiples
4. **Test the feel** - Does it feel like a "haunted ancient instrument"?

## Philosophy

**Prime Directive**: Absence of Cognitive Load
- User should FEEL, not think
- No dividers, generous negative space
- Personality through timing/feedback, not blocking
- User is petitioner, Muse is Oracle

## Status

✅ **LOCKED** - This design is finalized.

Any changes require updating:
1. This README
2. UI-SPECIFICATION.md
3. design-tokens.json
4. source/ui/MuseColors.h
5. CLAUDE.md (brief summary)
