# NORTH STAR: Muse

**Last Updated:** 2025-11-10
**Status:** Design Locked - Implementation in Progress

**What we're building:** A haunted morphing filter that conducts a séance with geometry.

**Not another studio plugin.** Not competing on features. Competing on **identity**.

---

## The Core Experience

You open Muse. Pure black void. A ghostly mint phosphor mouth stares back.

You turn MORPH. The mouth **snaps** between shapes—AA (wide open), AH (neutral), EE (narrow). Not smooth. **Stuttering at 10 FPS** like broken 1989 CRT hardware.

It's slightly unsettling. Mechanical. Otherworldly. You feel like you're communicating with something through an old terminal screen.

**That visceral discomfort is the product.**

---

## Why This Matters

### The Problem
Every modern plugin looks the same: gradients, RGB glows, smooth 60 FPS animations, "premium dark UI."

FabFilter, Serum, Ableton devices—they're **beautiful and forgettable**.

### The Solution
Muse doesn't try to look expensive. It looks **haunted**.

- **Mint phosphor (#d8f3dc) on pure black (#000000)** - Authentic P1 phosphor CRT displays
- **10 FPS mouth stutter** - Hardware snap creates visceral, mechanical feel
- **Minimal interface** - Just a mouth, 3 knobs, and a title
- **No explanations** - The mouth responds. That's all you need to know.

### The Rationale

**Mint phosphor is engineered, not aesthetic:**
- 19.8:1 contrast ratio (better than white on dark gray)
- Historical authenticity (real CRT oscilloscopes used P1 green phosphor)
- Eye fatigue reduction (green wavelengths easiest on eyes in dark studios)
- Market distinction (scan any plugin site—nothing looks like this)
- Emotional resonance (ghostly, otherworldly, refined)

**10 FPS stutter is intentional:**
- Creates visceral, slightly unsettling motion
- Feels mechanical and broken (in a good way)
- Reinforces the "1989 haunted hardware" narrative
- Makes the mouth feel **alive** but not human
- **This is NOT a bug** - it's the identity

---

## The Rules (Locked Forever)

**Source of Truth:** `design/muse-design-system.json`

1. **Two colors only**: #000000 (black) and #d8f3dc (mint phosphor)
2. **No gradients on background**: Pure black void
3. **Smooth vector mouth**: Ellipse that morphs AA → AH → EE
4. **10 FPS mouth animation**: Hardware snap, never smooth interpolation
5. **Minimal layout**: Mouth + 3 knobs + title only
6. **Authentic phosphor glow**: CRT bloom effect on mint elements

**These rules are non-negotiable.** They define the product.

---

## What We're NOT Building

❌ A "professional studio plugin" - Generic, safe, forgettable
❌ Feature-complete filter - We have 3 knobs, not 50
❌ Smooth, polished UX - The stutter is intentional
❌ Conventional design - We're not competing with FabFilter
❌ Something for everyone - This is intentionally polarizing

**If someone says "it looks broken" or "the animation stutters," that means it's working.**

---

## What We ARE Building

✅ A piece of haunted hardware from 1989
✅ A séance conducted through geometry
✅ An instrument with personality and character
✅ Something that makes you feel slightly uncomfortable
✅ The most distinctive filter plugin in any marketplace

**Identity over features. Character over polish. Vision over validation.**

---

## The Interface (800×400px Horizontal)

### Layout
```
┌──────────────────────────────────────────────────────────────────┐
│  MUSE                                                            │
│                                                                  │
│   ⚫          ⚫                  ┌──────────────────────────┐   │
│  MORPH    INTENSITY             │                          │   │
│                                  │                          │   │
│                                  │    MINT PHOSPHOR MOUTH   │   │
│   ⚫                              │    (Smooth vector)       │   │
│  MIX                             │    Snaps at 10 FPS       │   │
│                                  │    AA → AH → EE          │   │
│                                  │                          │   │
│                                  └──────────────────────────┘   │
└──────────────────────────────────────────────────────────────────┘
  0-380px: Controls              380-800px: Mouth Display
```

### Color Palette
```
Background:     #000000 (pure black void)
Phosphor:       #d8f3dc (mint - P1 phosphor green)
Glow:           #d8f3dc80 (semi-transparent for bloom)
Dim:            #d8f3dc40 (very dim for subtle elements)
```

**That's it.** Two colors. No gradients. No complexity. Pure contrast.

---

## The Mouth Algorithm (Hardware Snap at 10 FPS)

```cpp
// UI refreshes at 60 FPS, but mouth updates at 10 FPS
void timerCallback() override
{
    if (++frameCount % 6 == 0)  // 60 FPS / 6 = 10 FPS
    {
        // Get vowel shape from DSP
        VowelShape vowel = getCurrentVowelShape();

        // Snap to new shape (NO interpolation)
        currentMouthWidth = getWidthForVowel(vowel);
        currentMouthHeight = getHeightForVowel(vowel);

        repaint();  // Redraw with new shape
    }
}

void paint(Graphics& g) override
{
    // Pure black void
    g.fillAll(Colour(0xFF000000));

    // Draw mouth ellipse with phosphor glow
    Path mouth;
    mouth.addEllipse(bounds.withSizeKeepingCentre(
        currentMouthWidth, currentMouthHeight));

    // Glow layer (CRT bloom)
    g.setColour(Colour(0x80d8f3dc));
    g.fillPath(mouth);

    // Solid mint outline
    g.setColour(Colour(0xFFd8f3dc));
    g.strokePath(mouth, PathStrokeType(2.0f));
}

// Vowel shape dimensions
float getWidthForVowel(VowelShape v) {
    switch (v) {
        case AA: return 180.0f;  // Wide open
        case AH: return 150.0f;  // Neutral
        case EE: return 120.0f;  // Narrow smile
    }
}

float getHeightForVowel(VowelShape v) {
    switch (v) {
        case AA: return 120.0f;  // Tall opening
        case AH: return 100.0f;  // Neutral
        case EE: return 80.0f;   // Narrow horizontal
    }
}
```

**Key insight:** The UI runs at 60 FPS for smooth knob interaction, but the mouth **snaps** to new shapes every 100ms. This creates the haunted, mechanical feel while keeping the controls responsive.

---

## Current Implementation Status

### DSP ✅ Production Ready
- Authentic E-mu Z-plane filter (6-stage biquad cascade)
- Geodesic morphing between vowel shapes
- 3 parameters: MORPH (vowel journey), INTENSITY (resonance), MIX (dry/wet)
- 4 shape pairs: VOWEL, BELL, LOW, SUB
- Thread-safe (lock-free atomics)
- 5-8% CPU (scalar mode)

### UI 🏗️ In Progress
- ✅ Interactive knobs (MORPH, INTENSITY, MIX)
- ✅ All parameters at neutral state (0.5)
- ⏳ Mint phosphor on pure black aesthetic
- ⏳ Smooth vector mouth with 10 FPS snap
- ⏳ Phosphor glow effects (CRT bloom)

### Polish ⏳ Next
- Preset system (factory presets, save/load)
- SIMD optimization (2-3× DSP performance)
- Microinteractions (startup animation, haunted behavior)

---

## v1.0 Scope (Ship This)

**Must Have:**
1. Mint phosphor mouth that snaps at 10 FPS (AA → AH → EE)
2. 3 interactive knobs responding smoothly at 60 FPS
3. Pure black background with authentic phosphor glow
4. Preset system (5-10 factory presets)
5. VST3/AU/CLAP builds

**Nice to Have (v1.1+):**
- DRIFT parameter (autonomous movement)
- Advanced microinteractions
- SIMD DSP optimization

**Out of Scope (v2.0+):**
- AI features
- Voice control
- Neural DSP layers

---

## Design Principles (The Rules)

### 1. Hardware, Not Software
This plugin looks like **physical hardware from 1989**. A CRT display showing a mouth made of phosphorescent light. It's not a smooth modern app—it's vintage gear.

**Implications:**
- 10 FPS mouth animation (not 60fps smooth)
- Smooth vector ellipse (not chunky blocks)
- Pure black background (not gradients)
- Mint phosphor only (no color variety)

### 2. Direct, Visceral Control
You turn MORPH, the mouth morphs. You crank INTENSITY, it gets aggressive. You hear it, you see it, you feel it. **No cognitive load.**

**Implications:**
- Knobs respond immediately at 60 FPS
- Mouth snaps at 10 FPS for haunted feel
- No numbers unless you need them
- Everything important is visible

### 3. The Mouth Is the Interface
Users should watch the mouth, not the knobs. It's the **feedback mechanism**. When they adjust parameters, they're conducting the mouth like an instrument.

**Implications:**
- Mouth dominates the layout (right half of 800×400px)
- Knobs are secondary (left side, smaller)
- No waveform displays or spectrum analyzers (the mouth IS the visualizer)

### 4. Authentic Imperfection
The 10 FPS snap isn't smooth. It's **mechanical**. Slightly broken. Otherworldly. That's intentional.

**Implications:**
- Embrace the stutter (don't try to smooth it)
- The snap creates visceral discomfort
- This is the product, not a bug

---

## Success Metrics

**NOT measuring:**
- Feature parity with competitors
- Mass market appeal
- Positive sentiment from everyone

**ARE measuring:**
- Immediate visual recognition ("That's Muse")
- Polarized reactions (love it or hate it, not indifferent)
- People describing it as "haunted," "weird," "unsettling," "distinctive"
- Artists using it BECAUSE it's different, not despite it

**If everyone likes it, we've failed.**
**If it's divisive and memorable, we've succeeded.**

---

## The Vision Statement

> "Muse is not a plugin. It's a haunted terminal from 1989 that filters audio through geometry. When you turn the MORPH knob, a phosphorescent mouth snaps between vowel shapes at 10 FPS—mechanical, slightly broken, unnervingly responsive. It's not trying to be professional or polished. It's trying to feel **alive**."

**That's what we're building. Ship it.**

---

## Final Word

Every decision—mint phosphor, 10 FPS stutter, minimal layout, pure black void—serves the same goal: **Create an interface so distinctive that nobody mistakes it for anything else.**

The DSP is world-class E-mu Z-plane technology. The UI is a haunted CRT terminal. Together they create something that doesn't exist in the plugin market.

**Don't compromise the vision for validation. The vision IS the product.**

**Ship the séance.**

---

**Maintained by:** Claude Code + Human Visionary
**Last Updated:** 2025-11-10
**Status:** Design Locked - Mint Phosphor + 10 FPS Snap Final
