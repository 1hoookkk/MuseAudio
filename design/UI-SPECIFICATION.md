# Muse UI Specification
## Option 3: Subtle Dark Texture Under Warm Overlay

**Last Updated**: 2025-11-03
**Status**: LOCKED - This is the definitive visual specification

---

## Visual Philosophy

**Core Concept**: "Haunted Ancient Instrument with Warm Sophistication"

The UI achieves depth and mystery through **layering**, not harshness:
- Dark concrete texture provides weight and age (15% opacity)
- Warm linen overlay provides sophistication and calm (85% opacity)
- Result: Subtle depth, like old plaster over ancient stone

**NOT**: Another dark plugin with cyan accents
**YES**: Anthropic-style warm palette with haunted depth

---

## Layer Structure (Bottom to Top)

```
Layer 5 (Top):    Floating UI elements (logo card, transmission card)
Layer 4:          Knobs and controls with subtle shadows
Layer 3:          Warm linen overlay (#FAF0E6 at 85% opacity)
Layer 2:          Optional 1px blur for texture softening
Layer 1 (Base):   Dark concrete texture (#343A40 at 15% opacity)
```

**Critical Implementation Note**: The texture and overlay must be composited at the root level, then all UI elements are painted on top. Do NOT apply opacity to individual components.

---

## Color Palette

### Background System
```
Base Texture:     #343A40 (dark charcoal) at 15% opacity
Overlay:          #FAF0E6 (warm linen) at 85% opacity
Result:           Warm background with subtle textural depth
```

**Texture Source**: `assets/images/Vliestapete-Putz-Optik-dunkelgrau-Imitations-10238-15_3.jpg.jpeg`

**JUCE Implementation**:
```cpp
// Pseudo-code for layered background
1. Load texture image, convert to grayscale
2. Paint at 15% opacity (or tint with #343A40)
3. Apply 1px blur (optional, for softness)
4. Fill with #FAF0E6 at 85% opacity
5. Result = composited background
```

### Primary Colors

| Token | Hex | Usage |
|-------|-----|-------|
| `text.primary` | #5C5552 | Warm taupe - labels, knob outlines |
| `text.secondary` | #8B8682 | Lighter taupe - hints, secondary text |
| `text.muse-voice` | #4A4745 | Darker taupe - transmission text |
| `logo.silhouette` | #FAF9F6 | Pale cream - logo color |
| `logo.card` | #E8E3DB | Slightly darker warm - logo background |

### Accent Colors (The Magic)

| Token | Hex | Usage |
|-------|-----|-------|
| `accent.lilac` | #C8B6D8 | Soft lilac - gradient start |
| `accent.peach` | #FFD4C4 | Soft peach - gradient end |
| `accent.gradient` | `linear-gradient(135deg, #C8B6D8 0%, #FFD4C4 100%)` | Active states, breathing effects |

**Gradient Usage Rules**:
- ✅ Active knob fills
- ✅ Breathing pulse around logo
- ✅ "Ask Muse" button when pressed
- ❌ NOT for backgrounds (too much)
- ❌ NOT for text (readability)

### Transmission Area

| Token | Hex | Usage |
|-------|-----|-------|
| `transmission.background` | #E6D9F0 | Soft lilac card background |
| `transmission.glow` | rgba(200, 182, 216, 0.3) | Subtle glow during stuttering |

---

## Typography

### Font Families

**UI Elements** (labels, values):
```
font-family: system-ui, -apple-system, 'Segoe UI', sans-serif;
```
Clean, geometric, no personality.

**Muse's Voice** (transmission text):
```
font-family: Georgia, 'Times New Roman', serif;
```
More personal, hand-written feel. She's not a robot.

### Font Sizes

| Element | Size | Weight | Letter Spacing |
|---------|------|--------|----------------|
| Parameter labels | 11px | 500 | 0.08em (slight tracking) |
| Parameter values | 14px | 400 | 0 |
| Transmission text | 16px | 400 | 0 |
| Section titles | 18px | 500 | 0 |

**All sizes in px for consistency across platforms.**

### Text Rendering

- Anti-aliasing: YES (smooth)
- Subpixel rendering: Platform default
- Transform: `text-transform: uppercase` for parameter labels only

---

## Component Specifications

### 1. Logo Card

**Position**: Top left corner
**Composition**:
```
┌─────────────────┐
│ [Warm card]     │  Card: #E8E3DB
│                 │  Shadow: 0 2px 8px rgba(92,85,82,0.08)
│      🎭         │  Silhouette: #FAF9F6
│                 │  Padding: 16px
│     MUSE        │  Text: 11px, uppercase, #5C5552
└─────────────────┘
```

**Sizing**:
- Card: Auto-width based on content + 32px horizontal padding
- Logo height: 64px
- Total card height: ~96px

**Optional Enhancement**: Subtle breathing pulse (lilac glow) when processing audio

### 2. Knobs (Primary Controls)

**Layout**: 2x2 grid, centered in UI

```
    Morph          Haunt

    Focus          Drift
```

**Visual Specification**:
```
Diameter: 80px
Stroke: 2px solid #5C5552 (warm taupe)
Fill (inactive): #F5EFE7 (very light warm)
Fill (active/touched): lilac-to-peach gradient
Indicator: 2px line from center to edge, #5C5552
```

**States**:
- **Default**: Taupe outline, light fill
- **Hover**: Subtle shadow increase
- **Active/Dragging**: Gradient fill, slightly larger shadow
- **Focused (keyboard)**: Lilac outline glow

**Label**:
- Position: Below knob, centered
- Text: Uppercase, 11px, #5C5552
- Spacing: 12px from knob bottom

**Value Display**:
- Hidden by default
- Right-click shows value as tooltip
- NO permanent value numbers (clean aesthetic)

### 3. Transmission Area (Muse's Voice)

**Position**: Bottom of UI, full width inset
**Composition**:
```
┌──────────────────────────────────────┐
│ [Lilac card #E6D9F0]                 │
│                                      │
│        "Tin...kering..."             │  ← Serif, 16px, #4A4745
│                                      │     Stutter-frame reveal
└──────────────────────────────────────┘
```

**Sizing**:
- Height: 80px fixed
- Width: Plugin width - 80px (40px margin each side)
- Border radius: 8px
- Shadow: Same as logo card

**Stutter-Frame Animation**:
```
FPS: 10 (one frame every 100ms)
Duration: 400ms total
Method: Reveal characters one at a time, stuttering
Effect: Low-fps transmission from another dimension

Example for "Tinkering...":
Frame 1 (0ms):    ""
Frame 2 (100ms):  "T"
Frame 3 (200ms):  "Tin"
Frame 4 (300ms):  "Tink"
Frame 5 (400ms):  "Tinkering..."
```

**Text should pulse slightly** during reveal (subtle scale or glow).

### 4. "Ask Muse" Button

**Position**: Between knobs and transmission area
**Visual**:
```
┌─────────────────┐
│   ASK MUSE      │  Normal: Outline button, taupe
│      🎭         │  Hover: Gradient background
└─────────────────┘  Pressed: Gradient + slight scale down
```

**Sizing**:
- Width: 160px
- Height: 40px
- Border radius: 20px (pill shape)
- Border: 2px solid #5C5552

**States**:
- **Default**: Outline only, no fill
- **Hover**: Gradient background, white text
- **Pressed**: Scale down 98%, gradient background
- **Processing**: Replace with "Pondering..." text + subtle pulse

---

## Layout Grid

**Plugin Dimensions**: 640px × 480px (4:3 aspect ratio)

**Layout Structure**:
```
┌─────────────────────────────────────────┐
│ [40px margin]                           │ ← Top
│   ┌─────────┐                           │
│   │ Logo    │                           │
│   └─────────┘                           │
│                                         │
│        [48px section spacing]           │
│                                         │
│         ○          ○                    │ ← Knobs
│       Morph      Haunt                  │
│                                         │
│         ○          ○                    │
│       Focus      Drift                  │
│                                         │
│        [32px spacing]                   │
│                                         │
│     ┌─────────────────┐                │
│     │   ASK MUSE      │                │
│     └─────────────────┘                │
│                                         │
│        [32px spacing]                   │
│                                         │
│   ┌─────────────────────────────────┐  │
│   │ [Transmission]                  │  │
│   │ "Tin...kering..."               │  │
│   └─────────────────────────────────┘  │
│ [40px margin]                           │ ← Bottom
└─────────────────────────────────────────┘
```

**Grid System**:
- Base unit: 8px
- All spacing in multiples of 8
- Margins: 40px (5 units)
- Section spacing: 48px (6 units)
- Control spacing: 32px (4 units)

---

## Effects & Animations

### Shadows
```css
/* Logo/Transmission Cards */
box-shadow: 0 2px 8px rgba(92, 85, 82, 0.08);

/* Knobs */
box-shadow: 0 1px 4px rgba(92, 85, 82, 0.12);
```

### Animation Timings
| Effect | Duration | Easing |
|--------|----------|--------|
| Hover states | 150ms | ease-out |
| Parameter changes | 300ms | ease-in-out |
| Stutter reveal | 400ms | steps(4) |
| Breathing pulse | 3000ms | ease-in-out, infinite |

### Breathing Pulse (Logo/Processing States)
```
Animation: Subtle lilac glow that pulses
Duration: 3 seconds per cycle
Effect: Glow from 0% to 30% opacity and back
Color: rgba(200, 182, 216, 0.3)
Blur: 12px
```

---

## States & Feedback

### Loading States (Muse's Mutterings)

Display in transmission area during processing:
- "Doodling..."
- "Tinkering..."
- "Pondering..."
- "Rummaging..."

**Rotation**: Random selection, never same twice in a row

### Success Verdicts

After "Ask Muse" completes:
- "Voila."
- "So."
- "Ta-da."

**Display**: 2 seconds, then fade out

### Error Verdict

On any failure:
- "Fiddlesticks."

**Display**: 3 seconds, then fade out

---

## Implementation Checklist

### Phase 1: Foundation
- [ ] Load and composite texture + overlay background
- [ ] Implement warm color palette (all tokens)
- [ ] Create basic layout grid (640×480)
- [ ] Add logo card with silhouette

### Phase 2: Controls
- [ ] Implement 4 custom knobs with gradient states
- [ ] Add parameter labels (uppercase, proper spacing)
- [ ] Create "Ask Muse" button with all states
- [ ] Wire APVTS to knobs (parameter binding)

### Phase 3: Personality
- [ ] Build transmission area card
- [ ] Implement stutter-frame text reveal
- [ ] Add loading state mutterings (random selection)
- [ ] Add success/error verdicts
- [ ] Implement breathing pulse on logo

### Phase 4: Polish
- [ ] Add all shadows and blur effects
- [ ] Fine-tune animation timings
- [ ] Test all interaction states
- [ ] Ensure accessibility (keyboard navigation)

---

## JUCE-Specific Implementation Notes

### Background Rendering
```cpp
void paint(juce::Graphics& g) override
{
    // Layer 1: Dark texture at 15% opacity
    g.setOpacity(0.15f);
    g.drawImage(concreteTexture, getLocalBounds().toFloat());

    // Layer 2: Warm overlay at 85% opacity
    g.setOpacity(0.85f);
    g.setColour(juce::Colour(0xFAF0E6));
    g.fillRect(getLocalBounds());

    // Reset opacity for UI elements
    g.setOpacity(1.0f);
}
```

### Gradient Helper
```cpp
juce::ColourGradient createMuseGradient(juce::Rectangle<float> bounds)
{
    juce::ColourGradient gradient(
        juce::Colour(0xFFC8B6D8), // Lilac
        bounds.getX(), bounds.getY(),
        juce::Colour(0xFFFFD4C4), // Peach
        bounds.getRight(), bounds.getBottom(),
        false
    );
    return gradient;
}
```

### Stutter-Frame Text Component
```cpp
class StutterText : public juce::Component, private juce::Timer
{
    // Reveal text at 10 fps over 400ms
    void startReveal(juce::String text);
    void timerCallback() override;
    // ... implementation
};
```

---

## Don't Get Tripped Up: Common Pitfalls

❌ **Don't**: Apply texture at 100% opacity (too dark)
✅ **Do**: Layer texture at 15% under warm overlay

❌ **Don't**: Use pure black for text
✅ **Do**: Use warm taupe #5C5552

❌ **Don't**: Overuse the gradient (looks tacky)
✅ **Do**: Only for active states and magic moments

❌ **Don't**: Make stutter-frame text too slow (>500ms)
✅ **Do**: Keep it snappy at 400ms total

❌ **Don't**: Add numbers to every knob permanently
✅ **Do**: Hide by default, show only on right-click

❌ **Don't**: Make "impatient" behaviors block functionality
✅ **Do**: Add personality through timing, not blocking

---

## This Is Locked

This specification is FINAL. Any deviations must be discussed and documented.

**The Goal**: When someone opens Muse, they should feel like they've discovered an ancient, warm, haunted instrument - not "just another plugin."
