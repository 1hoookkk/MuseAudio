# Muse Audio Plugin - Complete Architecture Documentation

**Version:** 1.0  
**Date:** November 7, 2025  
**Repository:** github.com/1hoookkk/MuseAudio  
**Branch:** feature/seance-ui

---

## Table of Contents

1. [System Overview](#system-overview)
2. [DSP Architecture](#dsp-architecture)
3. [Parameter System](#parameter-system)
4. [Thread Safety Model](#thread-safety-model)
5. [UI Architecture](#ui-architecture)
6. [Data Flow](#data-flow)
7. [Module Structure](#module-structure)
8. [Coding Patterns](#coding-patterns)
9. [Build System](#build-system)

---

## System Overview

**Muse** is a Z-plane morphing filter audio plugin built with JUCE 8, providing authentic EMU-style resonant filtering with a unique generative OLED-style interface.

### Tech Stack
- **Framework:** JUCE 8.0.4
- **Build System:** CMake 3.25+
- **Compiler:** MSVC 17.14 (Visual Studio 2022) on Windows
- **Language:** C++17
- **Plugin Formats:** VST3, CLAP, AU, Standalone

### Design Principles
1. **Thread Safety First:** Lock-free atomic communication between audio and UI threads
2. **RT-Safety:** Audio thread never allocates, never blocks, never touches UI
3. **Zero-Copy DSP:** SIMD-optimized biquad cascade with in-place processing
4. **Generative Aesthetics:** 10 FPS procedural mouth animation, intentional stutter
5. **OLED Hardware Aesthetic:** Dark teal + mint green, 400×600px retro display

---

## DSP Architecture

### Core Engine: `emu::ZPlaneFilter_fast`

Located: `modules/zplane-dsp/include/zplane/ZPlaneFilter_fast.h`

```cpp
namespace emu {
    class ZPlaneFilter_fast {
        // 6-stage biquad cascade (12 poles, 12 zeros)
        // SIMD-optimized, per-sample coefficient interpolation
        // Gated saturation for performance
    };
}
```

**Key Features:**
- **6-Stage Biquad Cascade:** 12 poles + 12 zeros for authentic Z-plane topology
- **Performance Modes:**
  - `Authentic`: Geodesic radius interpolation, exact tanh, full saturation (highest quality)
  - `Efficient`: Linear interpolation, fast tanh, gated saturation (2-5× faster)
- **SIMD Support:** SSE2 (x64) and ARM NEON optimization hooks
- **Zipper-Free:** Per-sample coefficient smoothing via `juce::LinearSmoothedValue`

### Shape Pairs (Authentic EMU Tables)

Located: `modules/zplane-dsp/include/zplane/EMUAuthenticTables.h`

```cpp
namespace emu {
    extern const PolePair VOWEL_A[6];  // Formant A
    extern const PolePair VOWEL_B[6];  // Formant B
    extern const PolePair BELL_A[6];   // Resonance A
    extern const PolePair BELL_B[6];   // Resonance B
    extern const PolePair LOW_A[6];    // Low-pass A
    extern const PolePair LOW_B[6];    // Low-pass B
    extern const PolePair SUB_A[6];    // Sub-bass A
    extern const PolePair SUB_B[6];    // Sub-bass B
}
```

**Shape Pair Semantics:**
- **Pair 0 (VOWEL):** Formant transitions (AA → AH → EE vowels)
- **Pair 1 (BELL):** Resonant peaks (OH → OO shaping)
- **Pair 2 (LOW):** Width modulation (Wide → Narrow filtering)
- **Pair 3 (SUB):** Sub-bass focus (Neutral, minimal movement)

### DSP Parameters

```cpp
filter_.setShapePair(shapeA, shapeB);  // Load pole/zero tables
filter_.setMorph(0.0f - 1.0f);         // Interpolate A→B
filter_.setIntensity(0.0f - 1.0f);     // Resonance depth (0% = transparent)
filter_.setMix(0.0f - 1.0f);           // Wet/dry blend
filter_.setDrive(0.0f - 1.0f);         // Pre-gain (0.0f = unity)
filter_.setSectionSaturation(0.0f - 1.0f); // Per-stage tanh amount
```

---

## Parameter System

### JUCE 8 Best Practices: APVTS + Cached Pointers

**Pattern:**
```cpp
// In PluginProcessor.h
private:
    juce::AudioProcessorValueTreeState state_;
    std::atomic<float>* morphParam_ = nullptr;  // Cached pointer
    
// In PluginProcessor.cpp constructor
PluginProcessor::PluginProcessor()
    : state_(*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache pointers ONCE (RT-safe read in processBlock)
    morphParam_ = state_.getRawParameterValue("morph");
}

// In processBlock (audio thread)
void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    // Direct atomic read (lock-free, no APVTS lookup)
    float morph = morphParam_ ? *morphParam_ : 0.5f;
    filter_.setMorph(morph);
}
```

### Parameter Definitions

Located: `source/PluginProcessor.cpp::createParameterLayout()`

| ID | Type | Range | Default | Description |
|----|------|-------|---------|-------------|
| `pair` | Int | 0-3 | 0 | Shape pair selector (VOWEL/BELL/LOW/SUB) |
| `morph` | Float | 0.0-1.0 | 0.5 | Morph position (A→B interpolation) |
| `intensity` | Float | 0.0-1.0 | 0.0 | Resonance intensity (0% = transparent bypass) |
| `mix` | Float | 0.0-1.0 | 1.0 | Wet/dry mix (1.0 = 100% wet) |
| `autoMakeup` | Bool | - | true | Auto makeup gain (always on in filter) |

---

## Thread Safety Model

### Lock-Free Atomic Communication

**Pattern: Audio Thread → UI Thread**

```cpp
// In PluginProcessor.h (audio thread writes, UI thread reads)
std::atomic<int> currentVowelShape_ {0};    // Vowel enum for mouth
std::atomic<float> audioLevel_ {0.0f};      // RMS level (0-1)

// Audio thread (processBlock)
void PluginProcessor::processBlock(...)
{
    // Calculate RMS
    float rms = /* ... */;
    audioLevel_.store(rms, std::memory_order_relaxed);
    
    // Map filter state to vowel shape
    VowelShape vowel = /* ... */;
    currentVowelShape_.store(static_cast<int>(vowel), std::memory_order_relaxed);
}

// UI thread (PluginEditor::timerCallback - 30 FPS)
void PluginEditor::timerCallback()
{
    // Lock-free read (relaxed ordering safe for non-critical UI updates)
    float level = processorRef.getAudioLevel();
    auto vowel = processorRef.getCurrentVowelShape();
    
    // Update UI components
    generativeMouth.setAudioLevel(level);
    generativeMouth.setVowel(static_cast<GenerativeMouth::Vowel>(vowel));
}
```

### Critical Rules

1. **Audio Thread NEVER:**
   - Allocates memory
   - Calls `MessageManager::callAsync()`
   - Touches UI components directly
   - Uses locks/mutexes
   - Calls `getActiveEditor()`

2. **UI Thread NEVER:**
   - Writes to DSP state (except via APVTS parameters)
   - Blocks audio thread
   - Assumes atomics are instantly updated

3. **Parameter Callbacks:**
   - `Slider::onValueChange` is ALWAYS called on message thread
   - No need for `MessageManager::callAsync()` wrapper
   - `SliderAttachment` handles thread marshalling automatically

---

## UI Architecture

### Component Hierarchy

```
PluginEditor (400×600px OLED aesthetic)
├── OLEDLookAndFeel (custom styling)
├── headerLabel ("MUSE")
├── ShapePairSelector (4-way shape pair selector)
├── GenerativeMouth (16×6 LED matrix, 10 FPS procedural)
├── TransmissionArea (context-aware message display)
├── morphKnob (MORPH parameter)
├── intensityKnob (INTENSITY parameter)
├── mixKnob (MIX parameter)
├── StatusBar (CPU/stability indicators)
└── footerLabel ("AUDIOFABRICA V 1.0")
```

### Custom Components

#### 1. GenerativeMouth (source/ui/GenerativeMouth.h)

**Purpose:** Procedurally generated LED mouth visualization

```cpp
class GenerativeMouth : public juce::Component, private juce::Timer
{
public:
    enum class Vowel { AA, AH, EE, OH, OO, Wide, Narrow, Neutral };
    
    void setVowel(Vowel v);           // Vowel shape from DSP
    void setMorph(float m);            // Morph parameter (0-1)
    void setAudioLevel(float level);   // RMS audio level (0-1)
    
private:
    void timerCallback() override;     // 10 FPS generation
    void generateNextFrame();          // Procedural algorithm
    
    std::array<bool, 96> currentFrame; // 16×6 pixel grid
    juce::Random random;               // Organic noise
};
```

**Visual Algorithm:**
1. **Base Shape:** Ellipse test with vowel-specific width/height/smile/roundness
2. **Audio Reactivity:** Opening scales with RMS level (0.6 + 0.8 × level)
3. **Organic Noise:** 3-5% per-pixel flicker (higher at loud volumes)
4. **Edge Softness:** 85-100% radius has probabilistic dropout
5. **Asymmetry:** Random row shifts (25% chance, ±1 pixel horizontal)
6. **Breathing Sparkles:** 1-3 random pixels when idle (level < 0.1)
7. **Teeth Hint:** 3-5 pixel horizontal line on AA/AH/EE when level > 0.2

**Rendering:**
- **Background:** `#0a0a0a` (deep black OLED)
- **LED Pixels:** Circular (`fillEllipse`), 15% gap between pixels
- **Glow Effect:** Radial gradient 2.5× pixel radius, mint green → transparent
- **Hotspot:** 40% pixel radius bright center (0.6× brighter)
- **Off LEDs:** 2% opacity mint green (barely visible dark pixels)

#### 2. ShapePairSelector (source/ui/ShapePairSelector.h)

**Purpose:** 4-way selector for shape pairs

```cpp
class ShapePairSelector : public juce::Component
{
public:
    void attachToParameter(juce::AudioProcessorValueTreeState& apvts,
                          const juce::String& paramID);
    void setShapeChangeCallback(std::function<void(const juce::String&)> cb);
    
private:
    std::array<ShapeButton, 4> buttons_;
    // VOWEL, BELL, LOW, SUB
};
```

#### 3. TransmissionArea (source/ui/TransmissionArea.h)

**Purpose:** Context-aware message display with stutter rendering

```cpp
class TransmissionArea : public juce::Component, private juce::Timer
{
public:
    enum class MessageType { None, ShapeChange, MorphHint, Warning };
    enum class RenderMode { Instant, Stutter, Smooth };
    enum class Zone { TopLeft, TopRight, Center, BottomLeft, BottomRight };
    
    void setMessage(const juce::String& msg, MessageType type, 
                   Zone zone, RenderMode mode);
    void setShapePair(const juce::String& shape);
    void setMorphPosition(float morph);
    void setAudioLevel(float level);
    
private:
    juce::String currentMessage_;
    float morphValue_;
    float audioLevel_;
    // Context-aware display logic
};
```

**Message Semantics:**
- **Shape Change:** "VOWEL LOADED", "BELL ACTIVE", etc.
- **Morph Position:** "A ← MORPH → B" visual indicator
- **Audio Activity:** Brightness modulation based on RMS level

#### 4. StatusBar (source/ui/StatusBar.h)

**Purpose:** CPU and stability indicators

```cpp
class StatusBar : public juce::Component
{
public:
    void setCpuUsage(float usage);      // 0-1
    void setStability(bool stable);     // RT-safe indicator
    
private:
    float cpuUsage_ = 0.0f;
    bool isStable_ = true;
};
```

---

## Data Flow

### Parameter Change Flow

```
User Interaction
    ↓
JUCE Slider Component
    ↓
SliderAttachment (thread-safe marshalling)
    ↓
APVTS (juce::AudioProcessorValueTreeState)
    ↓
std::atomic<float>* cached pointer
    ↓
Audio Thread (processBlock)
    ↓
filter_.setParameter(value)
    ↓
juce::LinearSmoothedValue (per-sample smoothing)
    ↓
Biquad Coefficients
    ↓
DSP Processing
```

### Audio Level Visualization Flow

```
Audio Input Buffer
    ↓
processBlock() RMS Calculation
    ↓
Time-Constant Envelope Follower (attack 10ms, release 200ms, buffer-size independent)
    ↓
audioLevel_.store() [atomic write]
    ↓
PluginEditor::timerCallback() [30 FPS]
    ↓
getAudioLevel() [atomic read]
    ↓
GenerativeMouth::setAudioLevel()
    ↓
TransmissionArea::setAudioLevel()
    ↓
StatusBar::setCpuUsage()
    ↓
UI Repaint (mouth opening, glow brightness, message opacity)
```

### Vowel Shape Mapping Flow

```
processBlock() reads: pair, morph parameters
    ↓
Map to VowelShape enum:
    VOWEL pair: morph < 0.33 → AA, < 0.67 → AH, else → EE
    BELL pair:  morph < 0.5  → OH, else → OO
    LOW pair:   morph < 0.5  → Wide, else → Narrow
    SUB pair:   always → Neutral
    ↓
currentVowelShape_.store() [atomic write]
    ↓
PluginEditor::timerCallback() [30 FPS]
    ↓
getCurrentVowelShape() [atomic read]
    ↓
GenerativeMouth::setVowel()
    ↓
generateNextFrame() [procedural algorithm]
    ↓
UI Repaint [10 FPS intentional stutter]
```

---

## Module Structure

### Project Layout

```
MuseAudio/
├── CMakeLists.txt              # Root build config (Pamplejuce-based)
├── source/
│   ├── PluginProcessor.h       # Audio processor (DSP engine wrapper)
│   ├── PluginProcessor.cpp
│   ├── PluginEditor.h          # UI controller
│   ├── PluginEditor.cpp
│   └── ui/                     # Custom UI components
│       ├── GenerativeMouth.h
│       ├── OLEDLookAndFeel.h
│       ├── ShapePairSelector.h
│       ├── TransmissionArea.h
│       ├── StatusBar.h
│       └── MuseColors.h
├── modules/                    # External/custom JUCE modules
│   ├── zplane-dsp/             # Custom DSP module
│   │   ├── CMakeLists.txt      # INTERFACE library
│   │   └── include/zplane/
│   │       ├── ZPlaneFilter_fast.h
│   │       └── EMUAuthenticTables.h
│   ├── melatonin_inspector/    # UI debugger (Cmd+I)
│   └── clap-juce-extensions/   # CLAP format support
├── JUCE/                       # JUCE 8.0.4 submodule
├── build/                      # CMake build artifacts (gitignored)
└── docs/                       # Additional documentation
```

### Module: zplane-dsp

**Type:** INTERFACE library (header-only)

```cmake
# modules/zplane-dsp/CMakeLists.txt
add_library(zplane-dsp INTERFACE)
target_include_directories(zplane-dsp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(zplane-dsp INTERFACE juce::juce_dsp)
target_compile_features(zplane-dsp INTERFACE cxx_std_17)
```

**Purpose:** Modular, reusable Z-plane filter DSP engine

**Dependencies:** `juce::juce_dsp` (for `LinearSmoothedValue`, `MathConstants`)

---

## Coding Patterns

### 1. Thread-Safe Atomic Communication

**Pattern:**
```cpp
// Processor.h
std::atomic<float> sharedState_ {0.0f};

// Audio thread (write)
void processBlock(...) {
    float computed = /* ... */;
    sharedState_.store(computed, std::memory_order_relaxed);
}

// UI thread (read)
void timerCallback() {
    float value = sharedState_.load(std::memory_order_relaxed);
    updateUI(value);
}
```

**Rationale:**
- `std::memory_order_relaxed` is safe for non-critical UI updates
- Avoid `seq_cst` overhead (4-10× slower on ARM)
- Only primitive types (`float`, `int`, `bool`) can be atomic
- `juce::String` and complex types CANNOT be atomic

### 2. JUCE 8 Parameter Access

**Pattern:**
```cpp
// ✅ CORRECT: Cached pointer (RT-safe)
std::atomic<float>* param_ = state_.getRawParameterValue("id");
float value = *param_;  // Direct atomic read

// ❌ WRONG: APVTS lookup (mutex lock, slow)
float value = *state_.getRawParameterValue("id");  // Repeated lookup
```

### 3. UI Update from Parameters

**Pattern:**
```cpp
// ✅ CORRECT: Direct UI update in callback (already on message thread)
slider.onValueChange = [this]() {
    label.setText(juce::String(slider.getValue(), 2), juce::dontSendNotification);
};

// ❌ WRONG: Unnecessary MessageManager::callAsync wrapper
slider.onValueChange = [this]() {
    juce::MessageManager::callAsync([this]() {  // Redundant!
        label.setText(juce::String(slider.getValue(), 2), juce::dontSendNotification);
    });
};
```

**Rationale:** `Slider::onValueChange` is ALWAYS called on message thread, even during automation.

### 4. DSP Parameter Smoothing

**Pattern:**
```cpp
// In filter class
juce::LinearSmoothedValue<float> morphSmoothed_;

// In prepare
morphSmoothed_.reset(sampleRate, 0.05);  // 50ms smoothing

// In updateCoeffs (called once per block)
morphSmoothed_.setTargetValue(newMorph);

// In process (per-sample)
float currentMorph = morphSmoothed_.getNextValue();
```

**Rationale:** Eliminates zipper noise, spreads coefficient updates across samples.

### 5. Generative UI Component Pattern

**Pattern:**
```cpp
class GenerativeComponent : public juce::Component, private juce::Timer
{
public:
    GenerativeComponent() { startTimerHz(10); }  // 10 FPS
    
    void setExternalState(float value) { /* Update state */ }
    
private:
    void timerCallback() override {
        generateNextFrame();  // Procedural generation
        repaint();
    }
    
    void generateNextFrame() {
        // Algorithm that creates new frame based on:
        // - External state (DSP parameters, audio level)
        // - Internal randomness (juce::Random)
        // - Time-based evolution
    }
    
    void paint(juce::Graphics& g) override {
        // Render currentFrame_
    }
};
```

### 6. OLED Visual Rendering

**Pattern:**
```cpp
void paint(juce::Graphics& g) override {
    // Deep black background
    g.fillAll(juce::Colour(0xff0a0a0a));
    
    const auto mint = juce::Colour(0xffd8f3dc);
    
    // Individual circular LED pixels with gaps
    for (int row = 0; row < ROWS; ++row) {
        for (int col = 0; col < COLS; ++col) {
            float centerX = col * cellWidth + cellWidth * 0.5f;
            float centerY = row * cellHeight + cellHeight * 0.5f;
            float radius = std::min(cellWidth, cellHeight) * 0.425f;  // 15% gap
            
            if (isLit[row][col]) {
                // Radial glow
                juce::ColourGradient glow(
                    mint.withAlpha(0.3f), centerX, centerY,
                    mint.withAlpha(0.0f), centerX, centerY + radius * 2.5f,
                    true
                );
                g.setGradientFill(glow);
                g.fillEllipse(centerX - radius * 2.5f, centerY - radius * 2.5f, 
                             radius * 5.0f, radius * 5.0f);
                
                // Core LED
                g.setColour(mint);
                g.fillEllipse(centerX - radius, centerY - radius, 
                             radius * 2.0f, radius * 2.0f);
                
                // Hotspot
                float hotRadius = radius * 0.4f;
                g.setColour(mint.brighter(0.6f));
                g.fillEllipse(centerX - hotRadius, centerY - hotRadius,
                             hotRadius * 2.0f, hotRadius * 2.0f);
            } else {
                // Off LED (barely visible)
                g.setColour(mint.withAlpha(0.02f));
                g.fillEllipse(centerX - radius, centerY - radius,
                             radius * 2.0f, radius * 2.0f);
            }
        }
    }
}
```

---

## Build System

### CMake Configuration (Pamplejuce-based)

**Root:** `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.25)
project(Muse VERSION 1.0.0)

# JUCE submodule
add_subdirectory(JUCE)

# Custom modules
add_subdirectory(modules/clap-juce-extensions EXCLUDE_FROM_ALL)
add_subdirectory(modules/melatonin_inspector)
add_subdirectory(modules/zplane-dsp)

# Plugin target
juce_add_plugin(Muse
    FORMATS VST3 AU AUv3 Standalone
    PRODUCT_NAME "Muse"
    COMPANY_NAME "Muse Audio"
    BUNDLE_ID "com.museaudio.muse"
    PLUGIN_MANUFACTURER_CODE Muse
    PLUGIN_CODE Muse
)

# Shared code target (for plugin + tests)
add_library(SharedCode INTERFACE)
file(GLOB_RECURSE SourceFiles CONFIGURE_DEPENDS 
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h"
)
target_sources(SharedCode INTERFACE ${SourceFiles})

# Link modules
target_link_libraries(SharedCode INTERFACE
    zplane-dsp
    melatonin_inspector
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_basics
)

target_link_libraries(Muse PRIVATE SharedCode)
```

### Build Commands (Windows)

```bash
# Configure (first time only)
cmake -B build -G "Visual Studio 17 2022" -A x64

# Build Release
cmake --build build --config Release --target Muse_Standalone
cmake --build build --config Release --target Muse_VST3
cmake --build build --config Release --target Muse_CLAP

# Install (VST3 auto-copies to system folder)
cmake --install build --config Release
```

### Output Locations

```
build/Muse_artefacts/Release/
├── Standalone/Muse.exe                      # 7.3 MB
├── VST3/Muse.vst3/Contents/x86_64-win/     # Auto-copied to:
│   └── Muse.vst3                            # C:\Program Files\Common Files\VST3\
└── CLAP/Muse.clap                           # 7.0 MB
```

---

## Performance Characteristics

### DSP Benchmarks (Release, x64)

| Mode | CPU (48kHz, 512 samples) | Latency | Quality |
|------|--------------------------|---------|---------|
| Authentic | ~0.8% per voice | < 1ms | Reference |
| Efficient | ~0.2% per voice | < 1ms | 98% identical |

### UI Performance

- **Main UI:** 30 FPS timer (33ms interval)
- **GenerativeMouth:** 10 FPS timer (100ms interval, intentional stutter)
- **Parameter Smoothing:** 50ms ramp time (LinearSmoothedValue)
- **Audio Level Smoothing:** Time-constant envelope (10ms attack, 200ms release, buffer-size independent)

### Memory Footprint

- **Plugin Binary:** ~7 MB (Release, static JUCE)
- **Runtime Heap:** ~2-5 MB (depends on JUCE internal allocations)
- **DSP State:** 12 biquads × 2 channels × 8 bytes = ~200 bytes
- **UI Components:** ~100 KB (mostly JUCE overhead)

---

## Known Issues & Future Work

### Current Limitations

1. **Phase 4 Disabled:** Synesthetic intelligence (FFT-based word generation) removed due to thread-safety violations
   - **Issue:** `getActiveEditor()` called from audio thread
   - **Fix:** Refactor using `juce::AsyncUpdater` or UI-side Timer-based analysis

2. **Font Deprecation Warnings:** JUCE 8 Font API changes
   - 15 warnings about `juce::Font::Font` constructor
   - **Impact:** None (cosmetic only)
   - **Fix:** Migrate to `FontOptions` API

3. **No Preset System:** No factory presets or preset browser
   - **Status:** Planned for v1.1

### Future Enhancements

1. **Modularize UI Components:** Move `source/ui/` → `modules/muse_ui/`
   - Benefits: Reusability across projects, faster compile times, easier testing
   - Reference: `melatonin_inspector` structure

2. **SIMD Optimization:** Enable SSE2/NEON paths in `ZPlaneFilter_fast`
   - Current: Scalar fallback only
   - Expected: 1.5-3× speedup on biquad processing

3. **Preset System:** JSON-based preset storage
   - Factory presets: Vowel sweeps, bell resonances, sub-bass shapes
   - User presets: APVTS state save/load

4. **Phase 4 Restoration:** Proper thread-safe synesthetic intelligence
   - FFT analysis on UI thread (separate buffer)
   - Async message generation (no audio thread blocking)

---

## Coding Standards

### File Organization

```cpp
// Header: ClassName.h
#pragma once
#include <juce_*/juce_*.h>  // JUCE headers first
#include "LocalHeaders.h"    // Project headers second

namespace optional {
    class ClassName {
    public:     // Public API first
    private:    // Implementation details last
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClassName)
    };
}
```

### Naming Conventions

- **Classes:** `PascalCase` (e.g., `GenerativeMouth`, `PluginProcessor`)
- **Functions:** `camelCase` (e.g., `setAudioLevel`, `processBlock`)
- **Private Members:** `camelCase_` with trailing underscore (e.g., `audioLevel_`, `filter_`)
- **Constants:** `SCREAMING_SNAKE_CASE` (e.g., `MAX_POLE_RADIUS`, `COLS`)
- **Enums:** `PascalCase` class enums (e.g., `VowelShape::AA`, `RenderMode::Stutter`)

### Comments

```cpp
/**
 * Multi-line doc comment for classes/functions
 * 
 * Describes purpose, behavior, thread-safety, complexity
 */
class Example {
    // Single-line comment for implementation details
    void implementation();
};
```

### Thread Safety Annotations

```cpp
// RT-SAFE: Can be called from audio thread (no allocations, no locks)
void processAudio() noexcept;

// NOT RT-SAFE: UI thread only (may allocate, access UI)
void updateDisplay();

// THREAD-SAFE: Lock-free atomic, callable from any thread
void setLevel(float level) { level_.store(level, std::memory_order_relaxed); }
```

---

## Testing Strategy

### Unit Tests (Catch2)

Located: `tests/`

```bash
# Run all tests
cmake --build build --config Release --target Tests
./build/Tests
```

**Coverage:**
- DSP coefficient calculation
- Shape pair morphing
- Parameter range validation
- Thread-safety of atomics

### Benchmarks (Catch2 Benchmark)

Located: `benchmarks/`

```bash
# Run performance benchmarks
cmake --build build --config Release --target Benchmarks
./build/Benchmarks
```

**Metrics:**
- Biquad cascade throughput
- Coefficient update cost
- SIMD vs scalar performance
- Memory allocation checks

### Manual Testing Checklist

1. **Parameter Automation:** Verify smooth morphing, no zipper noise
2. **Shape Pair Transitions:** Check all 4 pairs load correctly
3. **Audio Reactivity:** Mouth responds to RMS level
4. **CPU Usage:** Monitor with DAW performance meter
5. **Visual Quality:** OLED aesthetic, 10 FPS stutter on mouth
6. **Thread Safety:** No crashes under high load + automation

---

## Version Control

### Git Workflow

```bash
# Feature branch pattern
git checkout -b feature/my-feature
git commit -m "feat: add new feature"
git push origin feature/my-feature
```

### Commit Message Convention

```
feat: Add new feature
fix: Fix bug in component
perf: Optimize DSP routine
docs: Update architecture docs
refactor: Restructure UI layout
test: Add unit test for filter
```

### .gitignore Highlights

```gitignore
build/          # CMake artifacts
Builds/         # Old Projucer builds
*.user          # IDE files
.vs/            # Visual Studio
.vscode/        # VS Code
*.DS_Store      # macOS
```

---

## Contact & License

**Repository:** [github.com/1hoookkk/MuseAudio](https://github.com/1hoookkk/MuseAudio)  
**Branch:** `feature/seance-ui`  
**License:** See `LICENSE` file  
**Company:** Muse Audio  
**Product:** Muse v1.0

---

**Last Updated:** November 7, 2025  
**Document Version:** 1.0  
**Maintainer:** Development Team
