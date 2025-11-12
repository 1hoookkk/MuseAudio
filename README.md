# Muse

**Formant Z-Plane Resonator** • Retro Hardware LCD Aesthetic • Content-Aware Intelligence

A VST3/Standalone audio plugin that combines authentic EMU Z-Plane filter emulation with procedural halftone visualization, broken hardware aesthetics, and psychoacoustic content analysis.

[![Build Status](https://github.com/sudara/pamplejuce/actions/workflows/build_and_test.yml/badge.svg)](https://github.com/sudara/pamplejuce/actions)

---

## Features

### DSP Engine
- **Unified Z-Plane Architecture**: Fast (emu::ZPlaneFilter_fast) and Authentic (AuthenticEMUZPlane) modes
- **Four Shape Pairs**: VOWEL (AA→AH→EE), BELL (OH→OO), LOW (AA→OO), SUB (AH static)
- **Continuous Morphing**: Smooth transitions between pole formations (no discrete quantization)
- **Adaptive Gain**: Per-block RMS analysis compensates for resonance boost
- **Danger Mode**: +3dB boost with gain compensation bypass
- **RT-Safe**: Zero allocations/locks on audio thread, NaN/Inf sanitization, denormal suppression

### UI & Visualization
- **Lime Green LCD**: Retro 1980s hardware aesthetic (0xFF9FFF9F)
- **Dense Almond Lip**: 140×58 halftone grid with superellipse SDF (n=2.6)
  - Edge-weighted dot scaling (bigger at rim, smaller in center)
  - Vertical taper + horizontal pinch for organic contour
  - Steel grey tint (0xFF3B4A52) for dark-on-light contrast
- **Direct Pole Visualization**: Live Z-plane pole positions → 16×6 dot matrix @ 10 FPS
- **Phosphor Decay**: Per-dot CRT persistence with exponential fade
- **Chassis Corruption**: Procedural burn marks, scratches, knob wear (seed 1993)
- **Serial Badge**: "EMU-Z-1993-MUSE" faded hardware ID
- **30-Second Glitches**: Random 20-40s timing for broken hardware feel

### Intelligence
- **AUTO Mode**: Content-aware pair selection via psychoacoustic analysis
  - 10 Hz RMS-based heuristics (VOWEL/BELL/LOW/SUB)
  - Atomic state updates for UI feedback
- **Status LED**: 6px indicator showing DSP stability (Flow/Struggle/Meltdown)

---

## Build Instructions

### Requirements
- **CMake** 3.25+
- **C++23** compiler (MSVC 2022, Clang 15+, GCC 13+)
- **JUCE** 8.x (included as submodule)

### Windows (Visual Studio 2022)
```bash
# Configure
cmake -S . -B build-vs2022 -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build-vs2022 --config Release -j

# Test (optional - tests currently have build issues)
ctest --test-dir build-vs2022 -C Release --output-on-failure
```

### macOS (Xcode)
```bash
# Configure
cmake -S . -B build -GXcode

# Build
cmake --build build --config Release -j

# Test (optional)
ctest --test-dir build -C Release --output-on-failure
```

### Outputs
- **Standalone**: `build-vs2022/Muse_artefacts/Release/Standalone/Muse.exe`
- **VST3**: `build-vs2022/Muse_artefacts/Release/VST3/Muse.vst3`

---

## Parameters

| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| `pair` | Shape Pair | 0-3 | 0 | 0=VOWEL, 1=BELL, 2=LOW, 3=SUB |
| `morph` | Morph | 0-1 | 0.25 | Continuous interpolation between pair shapes |
| `intensity` | Intensity | 0-1 | 0.33 | Formant resonance strength (r ≈ 0.87 @ default) |
| `mix` | Mix | 0-1 | 1.0 | Wet/dry blend (full wet by default) |
| `auto` | AUTO | bool | false | Enable content-aware pair selection |
| `danger` | Danger Mode | bool | false | +3dB boost, bypass adaptive gain |

---

## Architecture

### RT-Safety Contract
- **No allocations** on audio thread (pre-allocated buffers)
- **No locks** (atomics + lock-free structures for UI↔DSP)
- **No UI calls** from audio thread (AsyncUpdater for utterances)
- **NaN/Inf sanitization** on input and output
- **Denormal suppression** (JUCE ScopedNoDenormals)

### Parameter Smoothing
- `ParameterState` with `LinearSmoothedValue` (20ms ramps)
- Consumed once per block to prevent zipper noise
- Pair changes handled by engine's internal coefficient smoothing

### Pole Frame Sharing
- Audio thread: `engine_.getLastPoles()` → `cachedPoleFrame_` (SpinLock)
- UI thread: `getLastPoles()` reads cached frame @ 10 FPS
- No direct engine access from UI (thread-safe by design)

---

## Design Philosophy

> **"Broken hardware that somehow still works"**

Muse embraces the aesthetic of 1980s hardware pushed to its limits:
- **Corruption as beauty**: Burn marks, scratches, phosphor decay
- **10 FPS stutter**: Haunted hardware refresh (never smooth 60 FPS)
- **Occasional glitches**: 30-second random timing for authenticity
- **Lime LCD + steel grey dots**: Retro display technology

The mouth visualization is **not decorative** — it shows actual DSP pole positions mapped to a halftone grid, giving you real-time Z-plane feedback.

---

## Project Structure

```
source/
├── PluginProcessor.{h,cpp}   # APVTS, processBlock, psychoacoustic analysis
├── PluginEditor.{h,cpp}      # UI layout, corruption layers, knob rendering
├── PresetManager.{h,cpp}     # JUCE 8 preset persistence
├── dsp/
│   ├── MuseZPlaneEngine.{h,cpp}     # Unified Fast/Authentic wrapper
│   ├── AuthenticShapeLoader.h       # EMU pole formation loader
│   └── ZPlaneShapes.hpp             # Hardcoded shape definitions
└── ui/
    ├── HalftoneMouth.h              # Superellipse lip + pole viz
    ├── MuseLookAndFeel.{h,cpp}      # Custom rendering styles
    └── MuseThemeManager.h           # Color palette + tokens
```

---

## Development

### Adding a New Shape Pair
1. Define pole formations in `ZPlaneShapes.hpp` (6 conjugate pairs, r/theta)
2. Add entry to `shapes_` array in `MuseZPlaneEngine`
3. Update `pair` parameter range in `createParameterLayout()`
4. Add vowel mapping in `AuthenticShapeLoader::getShapeIndex()`

### Modifying the Mouth Visualization
- **Superellipse**: Tune `nExp` in `HalftoneMouth.h` (2.2-3.0, default 2.6)
- **Edge weighting**: Adjust `smoothstep(0.70f, 1.00f, f)` thresholds
- **Dot density**: Change `lipCols`/`lipRows` (140×58 default)
- **Colors**: `LCD_LIME` (0xFF9FFF9F), tint (0xFF3B4A52) in PluginEditor.h

### Testing
```bash
# Build tests (currently have missing header issues)
cmake --build build-vs2022 --target Tests -j

# Run tests
./build-vs2022/Release/Tests.exe
```

---

## Contributing

This plugin prioritizes:
1. **RT-safety** (no audio thread violations)
2. **Parameter stability** (no ID/range changes across releases)
3. **Minimal diffs** (surgical patches, local fixes)
4. **Actionable commits** (what/why, files touched, validation)

See `CLAUDE.md` for full engineering guidelines.

---

## Credits

**DSP**: EMU Z-Plane filter architecture (emu::ZPlaneFilter_fast, AuthenticEMUZPlane)
**UI Framework**: JUCE 8.x
**Build System**: Pamplejuce (CMake + GitHub Actions)
**Visualization**: Procedural halftone rendering with superellipse SDF

Built with [Claude Code](https://claude.com/claude-code) 🤖

---

## License

See `LICENSE` file for details.
