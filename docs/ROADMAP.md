# Muse - North Star Vision

**Last Updated**: 2025-11-07  
**Status**: Live Implementation Guide

This document defines what Muse **is right now** and where it's going next.

---

## What Muse Is

**A haunted morphing filter** featuring authentic E-mu Z-plane technology, wrapped in an OLED séance aesthetic. It's a boutique instrument that feels alive—tactile, hypnotic, and beautiful.

**The User Experience**: You're not tweaking parameters. You're conducting a séance with a ghostly mouth made of light. Turn a knob, watch the mouth respond. It's direct, visceral, and strangely intimate.

---

## The Aesthetic: "OLED Séance"

### Core Identity
- **Dark, brutalist hardware aesthetic** from the 80s/90s
- **Retro OLED display** showing a procedurally-generated mouth
- **Mint green phosphor glow** (#d8f3dc) on pure black (#000000)
- **No gradients, no warmth**—just cold, precise, beautiful light

### The Mouth
This is the soul of the plugin. A **16×6 grid of glowing LED points** that form mouth shapes in real-time.

**What it looks like:**
- Individual circular LEDs with 15% gaps between them
- Each LED has a radial glow (mint green core fading to transparent)
- The pattern forms recognizable mouth shapes (open vowel, wide smile, tight circle, etc.)
- **10 FPS animation**—it snaps between states like old hardware, not smooth 60fps
- Organic imperfections: 3-5% of LEDs flicker randomly, asymmetry, breathing sparkles

**What controls it:**
- **Shape Pair** (VOWEL/BELL/LOW/SUB): Changes the mouth's character entirely
- **Morph** (0-1): Interpolates between A and B poles of the shape
- **Intensity** (0-1): How aggressive the filter resonates (mouth gets wider/more energetic at high values)
- **Audio Level**: Mouth responds to incoming signal (bigger when loud, subtle when quiet)

**Technical:** This is NOT vector art. It's a grid (`GenerativeMouth.h`, 16 columns × 6 rows). Each LED is positioned, tested against an ellipse equation, and rendered as a circle with glow. It's **deliberately retro hardware**, not smooth modern UI.

---

## The Interface (400×600px)

### Layout
```
┌─────────────────────────────────────┐
│  [Logo]                             │  ← Header (50px)
│                                     │
│  ┌─────────────────────────────┐   │
│  │                             │   │
│  │     THE GLOWING MOUTH       │   │  ← OLED Display (300px)
│  │     (16×6 LED grid)         │   │     Pure black background
│  │                             │   │     Mint green LEDs
│  └─────────────────────────────┘   │
│                                     │
│   [VOWEL] [BELL] [LOW] [SUB]       │  ← Shape Selector (40px)
│                                     │
│    ⚫MORPH  ⚫INTENSITY  ⚫MIX        │  ← Knobs (120px)
│                                     │
│  "Processing... fr...equencies"    │  ← Transmission (90px)
│  [STATUS: ████ 12% CPU]            │     Messages from Muse
└─────────────────────────────────────┘
```

### Color Palette
```
Background:     #000000 (pure black)
Accent:         #d8f3dc (mint green phosphor)
Structure:      #2F4F4F (dark teal for cards/knobs)
Inactive:       #1a1a1a (dark grey for off elements)
```

**That's it.** Two colors. No gradients, no lilac, no peach. Cold, precise, retro hardware.

### Components (What's Built ✅)

**1. GenerativeMouth (The LED Display)** ✅
- 16×6 grid of circular LEDs
- 10 FPS procedural generation
- Responds to: shape pair, morph, intensity, audio level
- Organic noise: random flicker, asymmetry, breathing
- **File:** `source/ui/GenerativeMouth.h`

**2. ShapePairSelector** ✅
- Four brutalist square buttons: VOWEL / BELL / LOW / SUB
- Active button glows mint green
- Changes the entire character of the mouth/filter
- **File:** `source/ui/ShapePairSelector.h`

**3. TransmissionArea** ✅
- Dark teal card with mint green text
- Context-aware messages (e.g., "Vowel morphing... fr...equencies")
- **10 FPS stutter-frame text** animation (broken radio transmission feel)
- **File:** `source/ui/TransmissionArea.h`

**4. StatusBar** ✅
- CPU meter
- Stability indicators
- Minimal, functional
- **File:** `source/ui/StatusBar.h`

**5. Custom Knobs** ✅
- Large (60-80px), charcoal circles
- Mint green indicator line (no numbers)
- Weighted feel (high resolution, smooth momentum)
- Labels: MORPH, INTENSITY, MIX
- **File:** `source/ui/MuseKnob.h`

**6. Logo** ✅
- Female silhouette (24×24px, top-left)
- Mint green on dark background
- Subtle breathing glow when audio passes
- **File:** Rendered in `PluginEditor.cpp`

---

## The Parameters (What Actually Exists)

### Current Parameters ✅
1. **Shape Pair** (int, 0-3): VOWEL / BELL / LOW / SUB
2. **Morph** (float, 0-1): A/B interpolation within selected pair
3. **Intensity** (float, 0-1): Resonance depth (filter feedback)
4. **Mix** (float, 0-1): Dry/wet blend
5. **Auto Makeup** (bool): RMS-based gain compensation

**These are locked in.** They work. The plugin sounds great with these five controls.

### Parameters NOT Implemented ❌
- ~~DRIFT~~ (autonomous LFO modulation)
- ~~HAUNT~~ (additional resonance layer)
- ~~FOCUS~~ (character-aware blend)

**Decision:** Do we add DRIFT/HAUNT later (v1.1+), or ship with current 3-knob layout?

---

## The DSP (What's Real)

### Core Engine ✅
- **6-stage biquad cascade** (12 poles total)
- **Authentic E-mu Z-plane shapes** (hardcoded poles from `EMUAuthenticTables_VERIFIED.h`)
- **Geodesic interpolation** between shape pairs (not linear)
- **Per-sample coefficient smoothing** (no clicks/zippers)
- **RMS-based auto makeup gain** (optional)
- **Safety constraints** (poles clamped to r < 0.995)

**Performance:**
- Scalar mode only (SIMD hooks exist but not enabled)
- ~5-8% CPU usage on mid-range hardware
- Rock-solid stability (no dropouts, no denormals)

**Files:**
- `modules/zplane-dsp/ZPlaneFilter_fast.h`
- `modules/zplane-dsp/EMUAuthenticTables.h`
- `source/PluginProcessor.cpp`

### Thread Safety ✅
- **Audio thread:** Pure DSP, no allocations, no locks
- **UI thread:** Rendering, parameter updates via APVTS
- **Communication:** 2 atomics (`audioLevel_`, `currentVowelShape_`) for UI feedback
- **Envelope smoothing:** Time-constant based (buffer-size independent)

**This is production-ready.** No race conditions, no undefined behavior.

---

## What's Next: The Roadmap

### Phase 1: Presets (1-2 weeks) 🎯 NEXT
**Goal:** Ship v1.0 with preset system

**Tasks:**
1. ComboBox or `< >` navigator for preset selection
2. 5-10 factory presets with simple names:
   - "Wide Open" (vowel A, high morph)
   - "Whisper" (vowel B, low intensity)
   - "Bell Tower" (bell shape, mid morph)
   - "Sub Rumble" (sub shape, high intensity)
   - etc.
3. Save/Load via JUCE PresetManager (or custom)
4. Current preset name shown in TransmissionArea

**No AI naming yet.** Just ship something usable.

### Phase 2: SIMD Optimization (2-3 days)
**Goal:** 1.5-3× DSP performance improvement

**Tasks:**
1. Enable SSE2 path in `ZPlaneFilter_fast.h`
2. Benchmark suite to prove gains
3. Fallback to scalar on old CPUs

**Low risk, high ROI.** The code is already written, just needs enabling.

### Phase 3: DRIFT Parameter (3-5 days) 🤔 OPTIONAL
**Goal:** Autonomous movement system

**What it does:**
- Ultra-slow LFO (30-120 second cycles)
- Modulates MORPH parameter subtly
- Knob indicator rotates on its own
- Depth controlled by DRIFT knob (0 = off, 1 = wild)

**Questions:**
- Do we need this for v1.0, or save for v1.1?
- Does it fit the "OLED hardware" aesthetic (or is it too "AI")?

### Phase 4: Microinteractions (1-2 weeks)
**Goal:** Bring the UI to life

**Examples:**
- **Startup:** Mouth draws itself (dot → full shape over 500ms)
- **Preset change:** LEDs scatter → reform in new pattern
- **Idle state:** Mouth thins to whisper, drifts slowly across screen
- **High intensity:** LEDs flicker violently, some blink off

**This is pure polish.** Makes it feel haunted and alive.

### Phase 5+: Advanced Features (Future)
- Voice control (whisper.cpp)
- AI preset naming (llama.cpp)
- Neural DSP layer (RTNeural)
- Style transfer (ONNX/Core ML)

**These are v2.0+.** Ship a solid v1.0 first.

---

## Design Principles (The Rules)

### 1. Hardware, Not Software
This plugin looks like **physical hardware from 1989**. An OLED display showing a mouth made of discrete LEDs. It's not a smooth iPad app—it's a piece of vintage gear.

**Implications:**
- 10 FPS animation (not 60fps)
- Discrete LED grid (not smooth vector paths)
- Pure black background (not gradients)
- Mint green only (no color variety)

### 2. Direct, Visceral Control
You turn MORPH, the mouth morphs. You crank INTENSITY, it gets aggressive. You hear it, you see it, you feel it. **No cognitive load.**

**Implications:**
- No numbers on knobs (unless you right-click)
- No preset browser dropdowns (just `< >` navigation)
- No hidden menus or settings panels
- Everything important is visible

### 3. The Mouth Is the Interface
Users should watch the mouth, not the knobs. It's the **feedback mechanism**. When they adjust parameters, they're conducting the mouth like an instrument.

**Implications:**
- Mouth dominates the layout (300px of 600px height)
- Knobs are secondary (below, smaller)
- No waveform displays or spectrum analyzers (the mouth IS the visualizer)

### 4. Retro Imperfection
The mouth has **organic flaws**: random LED flicker (3-5%), asymmetry, breathing sparkles. It's not perfect, it's alive.

**Implications:**
- Embrace the 10 FPS "snap" (don't try to smooth it)
- Add more noise/chaos at high intensity (not less)
- Avoid "clean" modern UI patterns

---

## What Muse Is NOT

❌ **Not a "Brutalist Temple"** with warm linen overlays and lilac-peach gradients (that was the old roadmap)  
❌ **Not AI-powered** (no llama.cpp, no "Ask Muse" oracle—yet)  
❌ **Not smooth/modern** (no 60fps, no vector art, no gradients)  
❌ **Not a traditional plugin UI** (no tabbed panels, no preset browsers, no numeric readouts)  

It's a **haunted OLED instrument**. Cold, precise, beautiful, and slightly broken.

---

## Technical Specs (For Reference)

### Stack
- **JUCE 8.0.4** (audio plugin framework)
- **CMake 3.25+** (build system)
- **MSVC 17.14** (C++17)
- **Windows/macOS/Linux** (cross-platform)

### Plugin Formats ✅
- VST3 (7.0 MB)
- CLAP (working)
- AU (macOS)
- Standalone (7.3 MB)

### Build ✅
All formats building successfully. No errors.

### Performance ✅
- 5-8% CPU (scalar mode)
- No dropouts, no clicks
- Stable under stress tests

---

## Success Criteria

### v1.0 (Ship in 1-2 Weeks)
- ✅ Core DSP works (Z-plane filter, 4 shape pairs, morphing)
- ✅ UI complete (OLED mouth, knobs, shape selector, transmission area)
- ✅ Thread-safe (no race conditions)
- ⏳ **Preset system** (5-10 factory presets, save/load)
- ⏳ **Polish pass** (fix any rough edges, test in real DAWs)

### v1.1 (Post-Launch)
- SIMD optimization (1.5-3× faster DSP)
- DRIFT parameter (autonomous movement)
- Microinteractions (startup animation, preset transitions)

### v2.0 (Future)
- AI features (llama.cpp for preset naming, parameter interpretation)
- Voice control (whisper.cpp)
- Neural DSP layer (RTNeural)

---

## The North Star

**"The Mouth, Drawn with Living Ink."**

A dark screen. A glowing grid of LEDs forming a mouth. You turn a knob, the mouth responds. It's **immediate, tactile, hypnotic, and beautiful**.

You're not using a plugin. You're conducting a séance with a piece of haunted hardware.

---

*This is what Muse is.*  
*Everything else is noise.*  
*Ship it.*

---

**Maintained by:** Claude Code + Human Visionary  
**Last Updated:** 2025-11-07  
**Status:** Live Implementation (v1.0 in progress)
