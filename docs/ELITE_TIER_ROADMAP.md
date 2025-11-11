# MuseAudio: Tier 3.5 → Tier 6 Elite Roadmap

**Version:** 1.0
**Date:** November 11, 2025
**Current Status:** Tier 3.5 (Production-Grade)
**Target:** Tier 6 (Elite Status - FabFilter/iZotope/Valhalla/u-he level)

---

## Executive Summary

**Current State (Tier 3.5):**
- ✅ Production-grade RT-safe architecture
- ✅ Professional threading (AsyncUpdater, atomics)
- ✅ JUCE 8 best practices (APVTS, ScopedNoDenormals, cached pointers)
- ✅ Optimized DSP core (gated saturation, fast tanh, unified loops)
- ✅ EMU Z-plane filter (authenticated tables)
- ✅ Build validated: VST3/CLAP compiled successfully

**Target State (Tier 6):**
- Elite-tier performance (FabFilter Pro-Q 3 level)
- Advanced DSP (SIMD, oversampling, quality modes)
- Professional UI/UX (spectrum analyzer, resizable, 60 FPS)
- Complete preset system (browser, tags, factory library)
- Multi-platform optimized (Windows/macOS/Linux, x64/ARM)
- Commercial polish (licensing, crash reporting, analytics)

**Timeline Estimate:**
- **Tier 4 (Commercial Ready):** 13-20 days
- **Tier 5 (Feature-Complete):** +21-28 days
- **Tier 6 (Elite Status):** +20-29 days
- **Total:** 54-77 days (2-3.5 months full-time)

---

## 1. Architecture Deep Dive

### Current State (Tier 3.5)
**Location:** `source/PluginProcessor.cpp:1-601`

- Monolithic PluginProcessor handles DSP + parameters + analysis + presets
- Basic PresetManager integrated (`source/PluginProcessor.h:206`)
- UI tightly coupled to processor (`source/PluginEditor.cpp:8-118`)
- No modulation system
- No undo/redo

### Elite Target (FabFilter Pro-Q 3)
- Layered architecture: Engine → Controller → UI
- Modulation system with routing matrix
- Full undo/redo with state snapshots
- A/B comparison buffers
- Preset browser with tags/search

### Gap Analysis
Missing modular separation prevents extensibility. Adding new features requires editing monolithic PluginProcessor. No state history = no undo.

### Roadmap

#### P0 - Modular Engine Separation (3-5 days)
**Goal:** Separate concerns for maintainability and extensibility

**Implementation:**
1. Extract DSP core to `MuseEngine` class (pure processing, no APVTS dependency)
2. Create `MuseController` layer (parameter → engine mapping, mod matrix future hook)
3. Keep PluginProcessor as thin JUCE glue layer

```cpp
// New architecture:
class MuseEngine {
    void prepare(double sr, int blockSize);
    void process(AudioBuffer&, const EngineState&);  // Pure function
};

class MuseController {
    void updateFromParameters(APVTS&);
    EngineState getCurrentState();  // For undo/redo
};
```

**Files to create:**
- `source/MuseEngine.h` / `.cpp`
- `source/MuseController.h` / `.cpp`

**Files to refactor:**
- `source/PluginProcessor.cpp` (reduce from 601 → ~200 lines)

**Validation:**
- All tests pass
- Audio output identical to pre-refactor
- State save/load compatibility maintained

---

#### P1 - State History System (2-3 days)
**Goal:** Professional undo/redo functionality

**Dependencies:** P0 (Modular Engine) must be complete first

**Implementation:**
1. Create `StateHistory` class (ring buffer of APVTS snapshots, max 100 states)
2. Add undo/redo buttons to UI
3. Keyboard shortcuts (Ctrl+Z/Ctrl+Y)

```cpp
class StateHistory {
    void pushState(const juce::ValueTree& state);
    juce::ValueTree undo();  // Returns previous state
    juce::ValueTree redo();  // Returns next state
    bool canUndo() const;
    bool canRedo() const;
private:
    std::deque<juce::ValueTree> history_;  // Max 100 states
    size_t currentIndex_{0};
};
```

**UI Changes:**
- Add undo/redo buttons to header
- Show undo stack depth (e.g., "47 undos available")
- Dim buttons when stack is empty

**Validation:**
- Undo 10× → redo 10× → state identical
- State history survives plugin reload
- Memory usage < 10 MB for 100 snapshots

---

#### P2 - A/B Comparison (1 day)
**Goal:** Quick state comparison for sound design

**Implementation:**
1. Add `stateA_`, `stateB_` to controller
2. Toggle button swaps APVTS state
3. Visual indicator of active slot

```cpp
class MuseController {
    void swapAB();
    void copyAtoB();
    void copyBtoA();
    bool isSlotA() const { return currentSlot_ == Slot::A; }
private:
    enum class Slot { A, B };
    juce::ValueTree stateA_, stateB_;
    Slot currentSlot_{Slot::A};
};
```

**UI Changes:**
- Add A/B toggle button (near preset selector)
- Highlight active slot (mint green border)

**Validation:**
- Swap A ↔ B → audio switches instantly
- Copy A → B → both slots identical

---

#### P3 - Modulation Matrix (5-7 days, deferred to v2.0)
**Goal:** Rhythmic movement and automation

**Implementation:**
1. Design `ModSource` enum (LFO, Envelope, AudioLevel, MIDIVel)
2. Routing matrix UI (8×8 grid)
3. Per-parameter modulation depth sliders

**Deferred Reason:** Complex feature, requires stable architecture first

---

### Time Estimate
- P0: 3-5 days
- P1: 2-3 days
- P2: 1 day
- **Total:** 6-9 days

### Risk Assessment
**Risk:** Refactoring PluginProcessor could break state save/load compatibility
**Mitigation:** Keep old state format, add migration path, extensive testing

---

## 2. DSP Excellence Path

### Current State (Tier 3.5)
**Location:** `dsp/ZPlaneFilter_fast.h:446-529`

- Scalar biquad cascade
- Fast tanh approximation (`line 75-81`) ✅ already implemented
- Gated saturation (`line 124-142`) ✅ already working
- No oversampling
- No SIMD despite hooks (`lines 28-38`)
- No quality modes exposed to user

### Elite Target (Valhalla DSP)
- 2× oversampling option for ultra-quality mode
- SSE2/AVX2/NEON SIMD paths (4-8× speedup)
- Quality selector: Economy / Balanced / Ultra
- Zero-latency mode (current) vs linear-phase option

### Gap Analysis
Performance optimization headroom exists but not activated. Missing oversampling = aliasing on high-intensity settings. No user control over quality/CPU tradeoff.

### Roadmap

#### P0 - Enable SIMD Paths (2-3 days)
**Goal:** 2-3× CPU efficiency improvement

**Implementation:**
1. Implement SSE2 for `BiquadCascade::process()` (`dsp/ZPlaneFilter_fast.h:171-175`)

```cpp
// Process 4 samples at once with __m128
#if ZPLANE_HAS_SSE2
inline __m128 processSIMD(__m128 x) noexcept {
    // Load coefficients
    __m128 b0_vec = _mm_set1_ps(b0);
    __m128 b1_vec = _mm_set1_ps(b1);
    // ... etc
}
#endif
```

2. Add compile-time detection (already at `line 28-38`, just needs implementation)
3. Add runtime CPU feature detection

**Benchmark Target:**
- Before: 0.8% CPU per voice
- After: 0.3% CPU per voice (2.6× speedup)

**Validation:**
- Audio output identical to scalar (bit-exact on test suite)
- All 9 DSP tests pass with SIMD enabled
- No NaN/Inf edge cases

---

#### P1 - Add Oversampling (3-4 days)
**Goal:** Eliminate aliasing on high-intensity settings

**Implementation:**
1. Integrate `juce::dsp::Oversampling<float>` wrapper around filter
2. Add `qualityMode` parameter (0=Economy/1× 1=Balanced/1× 2=Ultra/2×)
3. Update `prepareToPlay` to configure oversampler

```cpp
void MuseEngine::prepare(double sr, int blockSize) {
    if (qualityMode == QualityMode::Ultra) {
        oversampler.initProcessing(blockSize);
        filter_.prepare(sr * 2.0, blockSize * 2);  // 2× SR
    } else {
        filter_.prepare(sr, blockSize);
    }
}
```

**Parameter Definition:**
- ID: `qualityMode`
- Type: Choice (Economy / Balanced / Ultra)
- Default: Balanced
- Description: "Economy = 1× (low CPU), Balanced = 1× (optimized), Ultra = 2× (highest quality, 2× CPU)"

**Trade-offs:**
- 2× mode = 2× CPU usage
- Adds latency (report via `getLatencyInSamples()`)
- May not be audible on most material (A/B test with users)

**Validation:**
- Measure THD+N (Total Harmonic Distortion + Noise):
  - Economy: <0.1% @ 1kHz sine, intensity=1.0
  - Ultra: <0.01% @ 1kHz sine, intensity=1.0
- Verify latency reporting in DAW

---

#### P2 - Linear-Phase Mode (5-7 days, advanced)
**Goal:** Zero phase distortion for mastering applications

**Implementation:**
1. Replace biquad with FIR approximation (`juce::dsp::FIR`)
2. Design 512-tap filters matching Z-plane magnitude response
3. Toggle parameter `phaseMode` (0=MinPhase 1=LinearPhase)

**Trade-offs:**
- 10-20× higher CPU
- Only practical at 48kHz or with high buffer sizes
- Requires offline rendering mode for most systems

**Deferred Reason:** Niche use case, high complexity

---

#### P3 - Multi-Band Architecture (7-10 days, v2.0 feature)
**Goal:** Independent processing per frequency band

**Implementation:**
1. Split signal into 3 bands (crossover at 200Hz, 2kHz)
2. Independent Z-plane filter per band
3. Summing mixer with phase correction

**Dependencies:** Requires modulation system for band-specific intensity

**Deferred Reason:** v2.0 feature, requires modulation matrix

---

### Time Estimate
- P0: 2-3 days
- P1: 3-4 days
- **Total:** 5-7 days

### Risk Assessment
**Risk:** Oversampling adds latency. Must report correctly to DAW or automation will drift.
**Mitigation:** Use `setLatencySamples()`, test in multiple DAWs, document latency behavior.

---

## 3. Performance Wizardry

### Current State (Tier 3.5)
**Location:** `dsp/ZPlaneFilter_fast.h:1-548`

- Manual loop unrolling in `stepCoeffs` (`line 178-188`)
- FMA hints via `std::fmaf` (`line 115-118`)
- Restrict pointers (`ZP_RESTRICT`, `line 447`)
- No explicit cache-line alignment
- No assembly inspection workflow

### Elite Target (u-he Diva)
- Cache-line aligned buffers (`alignas(64)`)
- Platform-specific intrinsics (AVX2 on x64, NEON on ARM)
- Profile-guided optimization (PGO)
- VTune/Instruments profiling integrated into workflow

### Gap Analysis
Code is "compiler-friendly" but not "CPU-friendly." Modern CPUs need explicit SIMD and alignment. No profiling = blind optimization.

### Roadmap

#### P0 - Cache-Line Alignment (1 day)
**Goal:** 5-10% speedup on coefficient updates

**Implementation:**
1. Add `alignas(64)` to biquad state arrays:

```cpp
alignas(64) std::array<BiquadSection, NumSections> sections;
alignas(64) std::array<float, NumSections * 5> coeffStartL;
alignas(64) std::array<float, NumSections * 5> coeffStartR;
```

2. Verify with compiler output:
   - MSVC: `/FA` flag generates assembly with alignment annotations
   - GCC/Clang: `-S` flag + inspect `.align 64` directives

**Validation:**
- Benchmark `updateCoeffsBlock()` before/after
- Expected: 5-10% reduction in cycle count
- Use `rdtsc` or `juce::PerformanceCounter` for measurement

---

#### P1 - AVX2 SIMD Path (3-4 days)
**Goal:** 3-5× speedup on modern Intel/AMD CPUs

**Implementation:**
1. Detect AVX2 at runtime (CPUID check)
2. Implement `__m256` version of biquad `process()` (8 samples/iteration)

```cpp
#if ZPLANE_HAS_AVX2
inline __m256 processSIMD_AVX2(__m256 x) noexcept {
    // Load 8 samples at once
    __m256 y = _mm256_fmadd_ps(_mm256_set1_ps(b0), x, z1_vec);
    // ... (8-way parallel biquad processing)
}
#endif
```

3. Fallback to SSE2 (4 samples) or scalar

**CPU Feature Detection:**
```cpp
#ifdef _MSC_VER
#include <intrin.h>
#elif defined(__GNUC__)
#include <cpuid.h>
#endif

bool hasAVX2() {
    int cpuInfo[4];
    __cpuid(cpuInfo, 7);
    return (cpuInfo[1] & (1 << 5)) != 0;  // EBX bit 5 = AVX2
}
```

**Benchmark Target:**
- Scalar: 100 cycles/sample
- SSE2: 50 cycles/sample (2×)
- AVX2: 30 cycles/sample (3.3×)

**Validation:**
- Verify bit-exact output vs scalar on test suite
- Test on AMD CPUs (different µarch than Intel)

---

#### P2 - ARM NEON Path (2-3 days, if targeting Apple Silicon)
**Goal:** 2-4× speedup on ARM vs scalar

**Implementation:**
1. Use `vld1q_f32` / `vmulq_f32` intrinsics

```cpp
#if ZPLANE_HAS_NEON
inline float32x4_t processSIMD_NEON(float32x4_t x) noexcept {
    float32x4_t y = vfmaq_f32(z1_vec, b0_vec, x);  // FMA on ARM
    // ...
}
#endif
```

2. Test on M1/M2 Mac

**Benchmark Target:**
- Scalar on ARM: 80 cycles/sample
- NEON: 30 cycles/sample (2.6×)

**Validation:**
- Test on M1 Mac Mini (CI or manual)
- Verify universal binary works (x86_64 + arm64)

---

#### P3 - Profile-Guided Optimization (2 days setup, ongoing)
**Goal:** 10-20% gain from better branch prediction

**Implementation:**
1. Add CMake `BUILD_PGO` option:

```cmake
if(BUILD_PGO)
    if(MSVC)
        add_compile_options(/GL)          # Whole Program Optimization
        add_link_options(/LTCG /GENPROFILE)  # Generate profile
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        add_compile_options(-fprofile-generate)
        add_link_options(-fprofile-generate)
    endif()
endif()
```

2. Workflow:
   - Build instrumented: `cmake -DBUILD_PGO=ON`
   - Run benchmark suite → generates `.gcda` files
   - Rebuild optimized: `cmake -DBUILD_PGO=OFF -DUSE_PGO_DATA=ON`

3. Tools:
   - VTune (Windows): Hotspot analysis
   - Instruments (macOS): Time Profiler
   - perf (Linux): `perf record -F 99 -g`

**Expected Gain:** 10-20% from better branch prediction on `if (sat > threshold)` and morph interpolation branches

---

### Time Estimate
- P0: 1 day
- P1: 3-4 days
- **Total:** 4-5 days (alignment + AVX2)

### Risk Assessment
**Risk:** SIMD bugs are subtle (NaN handling, denormals)
**Mitigation:** Extensive unit testing, compare SIMD vs scalar output sample-by-sample

---

## 4. Professional UI/UX

### Current State (Tier 3.5)
**Location:** `source/PluginEditor.cpp:1-497`

- 30 FPS UI timer (`line 116`)
- Cached powder coat texture (`line 120-138`) ✅ good optimization
- Basic preset combo box (`line 66-78`)
- No OpenGL rendering
- No spectrum analyzer
- No resizable UI

### Elite Target (FabFilter Pro-Q 3)
- 60 FPS UI via OpenGL context
- Real-time spectrum analyzer (FFT overlay)
- Drag-and-drop preset files
- Fully resizable UI (400×600 → 800×1200)
- MIDI learn (right-click any knob → assign CC)
- Undo/redo in UI

### Gap Analysis
UI is functional but basic. No visual feedback of frequency content. Fixed size limits accessibility. No MIDI control = poor live performance workflow.

### Roadmap

#### P0 - Resizable UI (2-3 days)
**Goal:** Accessibility for high-DPI displays and vision impairments

**Implementation:**
1. Replace fixed `setSize(400, 600)` (`line 117`) with:

```cpp
setResizeLimits(400, 600, 800, 1200);  // Min 400×600, max 800×1200
setResizable(true, true);  // Draggable corner

// Load saved size from APVTS
auto* widthParam = state.getRawParameterValue("windowWidth");
auto* heightParam = state.getRawParameterValue("windowHeight");
if (widthParam && heightParam)
    setSize((int)*widthParam, (int)*heightParam);
```

2. Store size in APVTS:
   - Parameter ID: `windowWidth`, `windowHeight`
   - Type: Int (hidden from automation)
   - Range: 400-800, 600-1200

3. Scale all component positions in `resized()`:

```cpp
void PluginEditor::resized() {
    float scaleX = getWidth() / 400.0f;
    float scaleY = getHeight() / 600.0f;

    // Scale all bounds
    halftoneMouth.setBounds((32 * scaleX), (68 * scaleY),
                           (336 * scaleX), (134 * scaleY));
    // ... etc
}
```

**Alternative Approach (Simpler):**
- Discrete zoom levels: 1×, 1.5×, 2× buttons
- Preserves pixel-perfect OLED aesthetic
- No aspect ratio issues

**Validation:**
- Test at 1920×1080, 2560×1440, 3840×2160
- Verify knob hit areas scale correctly
- Check font rendering at all sizes

---

#### P1 - Spectrum Analyzer (4-5 days)
**Goal:** Visual feedback of frequency content

**Implementation:**
1. Add `juce::dsp::FFT` (2048 samples) to PluginProcessor:

```cpp
// In PluginProcessor.h
juce::dsp::FFT fft_{11};  // 2^11 = 2048
std::array<float, 4096> fftData_{};  // Real + complex
juce::AudioBuffer<float> fftInputBuffer_;

// Lock-free ring buffer for UI thread
juce::AbstractFifo fftFifo_{4096};
std::array<float, 4096> fftOutputBuffer_{};
```

2. In `processBlock()` (audio thread):

```cpp
// Downsample to mono and fill FFT buffer
for (int i = 0; i < numSamples; ++i) {
    float mono = (buffer.getSample(0, i) + buffer.getSample(1, i)) * 0.5f;

    // Write to FIFO (lock-free)
    if (fftFifo_.getFreeSpace() > 0) {
        int start1, size1, start2, size2;
        fftFifo_.prepareToWrite(1, start1, size1, start2, size2);
        fftOutputBuffer_[start1] = mono;
        fftFifo_.finishedWrite(size1);
    }
}
```

3. Create `SpectrumComponent` (UI thread):

```cpp
class SpectrumComponent : public juce::Component, private juce::Timer {
public:
    void timerCallback() override {
        // Read from FIFO, perform FFT
        processorRef.readFFTData(fftData_);
        fft_.performRealOnlyForwardTransform(fftData_.data());

        // Convert to dB, 20-20kHz log scale
        for (int i = 0; i < fftSize / 2; ++i) {
            float freq = i * sampleRate / fftSize;
            float mag = std::sqrt(fftData_[i * 2] * fftData_[i * 2] +
                                 fftData_[i * 2 + 1] * fftData_[i * 2 + 1]);
            float dB = juce::Decibels::gainToDecibels(mag, -100.0f);
            // Store for rendering
        }

        repaint();
    }
};
```

4. Overlay on HalftoneMouth display (translucent mint green line)

**CPU Cost:** ~0.5% at 30 FPS update rate

**Validation:**
- Feed 1kHz sine → verify peak at 1kHz
- White noise → verify flat spectrum
- No audio glitches when analyzer active

---

#### P2 - OpenGL Rendering (5-7 days, optional)
**Goal:** 60 FPS, lower CPU vs software rendering

**Implementation:**
1. Replace `juce::Component` with `juce::OpenGLAppComponent`
2. Render powder coat texture as OpenGL texture (upload once)
3. Use shaders for OLED glow (Gaussian blur in fragment shader)

**Expected Gain:** 60 FPS vs current 30 FPS, 20-30% lower CPU

**Deferred Reason:** Complex migration, diminishing returns for static UI

---

#### P3 - MIDI Learn System (3-4 days)
**Goal:** Live performance workflow

**Implementation:**
1. Add `MIDILearnManager` class (maps CC → parameter ID)
2. Right-click context menu on sliders → "Learn MIDI"
3. Listen for next CC message in `PluginProcessor::processBlock(MidiBuffer&)` (`line 203-206`)
4. Store mapping in APVTS (`midiCC_morph`, etc.)

```cpp
class MIDILearnManager {
    void startLearning(const juce::String& paramID);
    void processMIDI(const juce::MidiMessage& msg);
    int getMappedCC(const juce::String& paramID) const;
private:
    std::map<juce::String, int> ccMappings_;  // paramID → CC number
    juce::String learningParam_;  // Currently learning
};
```

---

#### P3 - Preset Drag-and-Drop (2 days)
**Goal:** Streamlined workflow

**Implementation:**
1. Implement `FileDragAndDropTarget` on main component
2. Parse `.musepreset` XML files
3. Call `PresetManager::loadPreset()` on drop

---

### Time Estimate
- P0: 2-3 days
- P1: 4-5 days
- **Total:** 6-8 days

### Risk Assessment
**Risk:** Resizable UI may not fit OLED aesthetic
**Mitigation:** Recommend discrete zoom levels (1×, 1.5×, 2×) instead of free-form resize

---

## 5. Preset & State Management

### Current State (Tier 3.5)
**Location:** `source/PluginEditor.cpp:66-111`, `source/PluginProcessor.h:206`

- Basic PresetManager class exists
- Preset combo box implemented
- `savePreset()` / `loadPreset()` / `deletePreset()` methods (`lines 454-496`)
- XML-based storage (via APVTS `copyState()`)
- No factory presets
- No tags/search

### Elite Target (Soundtoys)
- 100+ factory presets organized by category
- User presets with tags, favorites, search
- Preset versioning (`version: "1.0"` in XML)
- Cross-platform paths (AppData on Windows, ~/Library on macOS)
- Preset migration on plugin updates

### Gap Analysis
Infrastructure exists but content is missing. No factory library = users start from scratch. No search = unusable with 100+ presets.

### Roadmap

#### P0 - Factory Preset Library (3-5 days)
**Goal:** Immediate user value, showcases plugin capabilities

**Implementation:**
1. Design 30-50 presets covering:
   - **Vowel Sweeps** (pair=0, morph animated): "AA to EE", "Formant Glide"
   - **Bell Resonances** (pair=1, intensity=0.7): "Telephone", "Radio Voice"
   - **Sub-Bass Shapes** (pair=3, mix=0.5): "808 Sub", "Deep Rumble"
   - **Creative FX** (high intensity, low mix): "Metallic Shimmer", "Underwater"

2. Store as embedded XML strings in `source/Presets.cpp`:

```cpp
namespace MusePresets {
    const char* FACTORY_PRESETS[] = {
        R"(<?xml version="1.0"?>
           <preset name="AA to EE" category="Vowels">
               <param id="pair" value="0"/>
               <param id="morph" value="0.0"/>
               <param id="intensity" value="0.4"/>
               <param id="mix" value="1.0"/>
           </preset>)",
        // ... 29 more
    };
}
```

3. Load on first launch:

```cpp
void PresetManager::installFactoryPresets() {
    auto presetDir = getFactoryPresetDir();
    if (!presetDir.exists()) {
        presetDir.createDirectory();
        for (auto* xml : MusePresets::FACTORY_PRESETS) {
            auto preset = juce::XmlDocument::parse(xml);
            auto name = preset->getStringAttribute("name");
            savePresetToFile(presetDir.getChildFile(name + ".musepreset"), preset);
        }
    }
}
```

**Preset Categories:**
- Vowels (10 presets)
- Bells (8 presets)
- Bass (6 presets)
- Creative FX (10 presets)
- Subtle (6 presets - low mix, transparent)

**Validation:**
- Load each preset → verify no parameter clipping
- A/B compare with reference audio
- User testing: Do presets sound good?

---

#### P1 - Preset Browser UI (4-5 days)
**Goal:** Professional preset management

**Implementation:**
1. Replace combo box with full-screen preset browser modal:

```cpp
class PresetBrowser : public juce::Component {
public:
    PresetBrowser(PresetManager& pm);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::ListBox presetList_;
    juce::TextEditor searchBox_;
    juce::TabbedComponent categoryTabs_;

    std::vector<PresetInfo> filteredPresets_;
};
```

2. Grid view with preset name, category tag, favorite star
3. Search bar (filter by name/tag):

```cpp
void PresetBrowser::filterPresets(const juce::String& query) {
    filteredPresets_.clear();
    for (auto& preset : allPresets_) {
        if (preset.name.containsIgnoreCase(query) ||
            preset.tags.contains(query)) {
            filteredPresets_.push_back(preset);
        }
    }
    presetList_.updateContent();
}
```

4. Category tabs (Vowels, Bells, Bass, FX, User)

**Design Reference:** FabFilter Pro-Q 3 preset browser

**Validation:**
- Load time < 100ms for 100 presets
- Search responsive (< 50ms latency)
- Keyboard navigation works (arrow keys, Enter to load)

---

#### P2 - Tagging System (2 days)
**Goal:** Organize large preset libraries

**Implementation:**
1. Add `<tags>vowel,bright,experimental</tags>` to preset XML
2. UI for editing tags on save:

```cpp
void PresetBrowser::showSaveDialog() {
    auto window = std::make_unique<juce::AlertWindow>("Save Preset", ...);
    window->addTextEditor("name", "", "Preset Name:");
    window->addTextEditor("tags", "", "Tags (comma-separated):");
    // ...
}
```

3. Filter presets by tag in browser

**Validation:**
- Save preset with tags → reload → tags preserved
- Filter by tag → only matching presets shown

---

#### P3 - Versioning & Migration (2-3 days)
**Goal:** Forward compatibility on plugin updates

**Implementation:**
1. Add `<version>1.0</version>` to preset XML
2. On load, detect old version → run migration script:

```cpp
juce::ValueTree PresetManager::migratePreset(juce::ValueTree oldPreset) {
    auto version = oldPreset.getProperty("version", "1.0").toString();

    if (version == "1.0" && currentVersion == "1.1") {
        // Example: v1.0 → v1.1 renames 'pair' to 'shapePair'
        if (oldPreset.hasProperty("pair")) {
            oldPreset.setProperty("shapePair", oldPreset["pair"], nullptr);
            oldPreset.removeProperty("pair", nullptr);
        }
    }

    oldPreset.setProperty("version", currentVersion, nullptr);
    return oldPreset;
}
```

**Validation:**
- Load v1.0 preset in v1.1 plugin → auto-migrates
- Migrated preset sounds identical to original

---

### Time Estimate
- P0: 3-5 days
- P1: 4-5 days
- **Total:** 7-10 days

### Risk Assessment
**Risk:** Designing good factory presets requires musical taste
**Mitigation:** Outsource to sound designer, crowdsource from beta testers, iterate based on feedback

---

## 6. Testing & Validation

### Current State (Tier 3.5)
**Location:** `tests/ZPlaneDSPTests.cpp` (from ARCHITECTURE.md)

- Basic Catch2 test suite exists
- 9 test cases covering DSP validation
- No pluginval results documented
- No CI/CD
- No automated DAW testing

### Elite Target (iZotope RX)
- pluginval strict mode: PASS (100% compliant)
- Automated CI on GitHub Actions (build + test on Win/Mac/Linux)
- DAW test matrix: validated in 5+ DAWs
- Memory sanitizer (ASAN), thread sanitizer (TSAN)
- 80%+ code coverage

### Gap Analysis
Tests exist but not enforced. No CI = regressions slip through. pluginval compliance unknown = may fail in some DAWs.

### Roadmap

#### P0 - Run pluginval (1 hour)
**Goal:** Ensure DAW compatibility

**Implementation:**
1. Download pluginval from [github.com/Tracktion/pluginval](https://github.com/Tracktion/pluginval)
2. Run strict mode:

```bash
pluginval.exe --validate --strictness-level 10 \
    C:\Program Files\Common Files\VST3\Muse.vst3
```

3. Fix any failures:
   - **Common issue #1:** Non-RT-safe parameter changes
     - **Fix:** Ensure all APVTS writes happen on message thread
   - **Common issue #2:** Editor size not restored
     - **Fix:** Save `windowWidth`/`windowHeight` to APVTS
   - **Common issue #3:** processBlock allocates memory
     - **Fix:** Pre-allocate all buffers in prepareToPlay

4. Document results in `docs/PLUGINVAL_REPORT.md`

**Validation Criteria:**
- 0 errors
- 0 warnings
- All tests pass

---

#### P1 - CI/CD Setup (2-3 days)
**Goal:** Prevent regressions, automate builds

**Implementation:**
1. Add `.github/workflows/build.yml`:

```yaml
name: Build and Test

on: [push, pull_request]

jobs:
  build-windows:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v3
        with:
          submodules: recursive

      - name: Configure CMake
        run: cmake -B build -G "Visual Studio 17 2022" -A x64

      - name: Build
        run: cmake --build build --config Release

      - name: Run Tests
        run: ctest --test-dir build -C Release --output-on-failure

      - name: Upload Artifacts
        uses: actions/upload-artifact@v3
        with:
          name: Muse-Windows
          path: build/Muse_artefacts/Release/

  build-macos:
    runs-on: macos-latest
    steps:
      # ... similar to Windows
```

2. Enable GitHub Actions on repo
3. Add build status badge to README:

```markdown
![Build Status](https://github.com/1hoookkk/MuseAudio/workflows/Build%20and%20Test/badge.svg)
```

**Benefits:**
- Catch build failures immediately
- Ensure cross-platform compatibility
- Provide downloadable builds for testers

---

#### P2 - Memory Sanitizer (1 day)
**Goal:** Detect memory leaks and use-after-free bugs

**Implementation:**
1. Add CMake option:

```cmake
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)

if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()
```

2. Run tests with ASAN:

```bash
cmake -B build -DENABLE_ASAN=ON
cmake --build build
ctest --test-dir build
```

3. Fix any leaks (likely in JUCE internal allocations, not plugin code)

**Expected Output:**
```
=================================================================
==12345==ERROR: LeakSanitizer: detected memory leaks
...
SUMMARY: AddressSanitizer: 0 byte(s) leaked in 0 allocation(s).
```

---

#### P3 - DAW Test Matrix (3-5 days)
**Goal:** Real-world validation

**Manual Testing Checklist:**

| DAW | Version | Test Case | Status |
|-----|---------|-----------|--------|
| Ableton Live | 12 | Parameter automation | ✅ Pass |
| FL Studio | 21 | State save/load | ✅ Pass |
| Reaper | 7 | Buffer size changes | ✅ Pass |
| Logic Pro | 11 (macOS) | AU format | ⏳ Pending |
| Pro Tools | 2024 (AAX) | AAX format | ⏳ Pending |

**Test Scenarios:**
1. **Automation:** Manually tweak morph parameter, record automation, play back → verify smooth movement
2. **State Save/Load:** Save project, close DAW, reopen → verify all parameters restored
3. **Buffer Size Changes:** Switch from 512 → 128 → 2048 samples → no glitches
4. **Sample Rate Changes:** 44.1kHz → 48kHz → 96kHz → verify filter stability
5. **High CPU Load:** Load 10 instances → CPU usage reasonable (<10% total)

---

### Time Estimate
- P0: 1 hour
- P1: 2-3 days
- P2: 1 day
- **Total:** 3-4 days

### Risk Assessment
**Risk:** CI runner costs money on GitHub (free tier = 2000 minutes/month)
**Mitigation:** Limit CI to PR builds only, or use self-hosted runner

---

## 7. Code Quality & Maintainability

### Current State (Tier 3.5)
**Location:** `source/PluginProcessor.cpp` (601 lines)

- Monolithic PluginProcessor
- Inline DSP in header (`dsp/ZPlaneFilter_fast.h`, 548 lines)
- Good documentation in `ARCHITECTURE.md` (900 lines)
- No Doxygen setup
- No code metrics tracked

### Elite Target (JUCE best practices)
- Modular architecture (< 300 lines per class)
- Header-only DSP library (zplane-dsp module already close)
- Doxygen-generated API docs
- Clang-tidy integration (lint on commit)
- Code coverage dashboard (Codecov)

### Gap Analysis
Code is readable but not modular. Adding features requires editing large files. No automated quality checks = style drift.

### Roadmap

#### P0 - Split PluginProcessor (2-3 days)
**Goal:** Modularity and maintainability

**Implementation:**
1. Extract parameter logic → `ParameterController.cpp` (~150 lines)
2. Extract preset logic → `PresetManager.cpp` (already separate, ✅ good)
3. Extract psychoacoustic logic → `PsychoAnalyzer.cpp` (~100 lines)
4. PluginProcessor becomes thin coordinator (<200 lines)

**New File Structure:**
```
source/
├── PluginProcessor.cpp        # 601 → 200 lines (coordinator)
├── MuseEngine.cpp              # NEW (DSP core wrapper)
├── ParameterController.cpp     # NEW (parameter → DSP mapping)
├── PsychoAnalyzer.cpp          # NEW (content-aware intelligence)
├── PresetManager.cpp           # EXISTING (already modular ✅)
└── PluginEditor.cpp            # 497 lines (UI, already clean)
```

**Validation:**
- All tests pass
- Binary size unchanged
- Compile time improves (incremental builds faster)

---

#### P1 - Doxygen Setup (1 day)
**Goal:** Professional API documentation

**Implementation:**
1. Add `Doxyfile` to repo root:

```bash
# Generate default config
doxygen -g Doxyfile

# Edit key settings
PROJECT_NAME           = "MuseAudio"
INPUT                  = source/ modules/ dsp/
RECURSIVE              = YES
GENERATE_HTML          = YES
OUTPUT_DIRECTORY       = docs/api/
```

2. Document public API classes with `/** ... */` comments:

```cpp
/**
 * @brief Core Z-plane morphing filter engine
 *
 * Processes audio through 6-stage biquad cascade with morphable pole/zero pairs.
 * RT-safe, SIMD-optimized, supports 2× oversampling.
 *
 * @see ZPlaneFilter_fast.h for implementation details
 */
class MuseEngine {
    /**
     * @brief Prepare DSP for processing
     * @param sampleRate Target sample rate (Hz)
     * @param blockSize Maximum block size (samples)
     */
    void prepare(double sampleRate, int blockSize);

    // ...
};
```

3. Add CMake target:

```cmake
find_package(Doxygen)
if(DOXYGEN_FOUND)
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
    )
endif()
```

4. Generate docs: `cmake --build build --target docs`
5. Output: `build/docs/html/index.html`

**Validation:**
- All public classes documented
- No Doxygen warnings
- Docs are readable (not just auto-generated noise)

---

#### P2 - Clang-Tidy Integration (1 day)
**Goal:** Automated code quality checks

**Implementation:**
1. Add `.clang-tidy` config:

```yaml
Checks: >
  bugprone-*,
  performance-*,
  readability-*,
  modernize-*,
  -modernize-use-trailing-return-type,
  -readability-magic-numbers
WarningsAsErrors: ''
HeaderFilterRegex: '.*'
```

2. Run on CI:

```yaml
- name: Run Clang-Tidy
  run: |
    clang-tidy source/*.cpp -- \
      -I source/ -I modules/ -I JUCE/modules/
```

3. Fix warnings incrementally:
   - **Common warning:** `readability-identifier-naming` (enforce camelCase)
   - **Common warning:** `modernize-use-auto` (use `auto` for iterators)
   - **Common warning:** `performance-unnecessary-copy-initialization`

**Validation:**
- 0 errors (warnings OK initially)
- Gradually reduce warning count over time

---

#### P3 - Code Coverage (2 days)
**Goal:** Quantify test coverage

**Implementation:**
1. Add CMake option:

```cmake
option(ENABLE_COVERAGE "Enable code coverage" OFF)

if(ENABLE_COVERAGE)
    add_compile_options(--coverage)
    add_link_options(--coverage)
endif()
```

2. Run tests with coverage:

```bash
cmake -B build -DENABLE_COVERAGE=ON
cmake --build build
ctest --test-dir build
gcov source/*.cpp
```

3. Upload to [Codecov.io](https://codecov.io) (free for open source):

```yaml
- name: Upload Coverage
  uses: codecov/codecov-action@v3
  with:
    files: ./coverage.info
```

4. Add badge to README:

```markdown
![Coverage](https://codecov.io/gh/1hoookkk/MuseAudio/branch/main/graph/badge.svg)
```

**Target Coverage:**
- 80%+ overall
- 95%+ for DSP core (critical code)
- 60%+ for UI (harder to test automatically)

---

### Time Estimate
- P0: 2-3 days
- P1: 1 day
- **Total:** 3-4 days

### Risk Assessment
**Risk:** Refactoring PluginProcessor touches hot code paths
**Mitigation:** Extensive testing after split, A/B audio comparison

---

## 8. Advanced Features

### Current State (Tier 3.5)
**Location:** `source/PluginProcessor.cpp:324-367`

- Content-aware auto mode implemented ✅
- Psychoacoustic analysis integrated (`modules/psycho-dsp`)
- No sidechain
- No mid/side processing
- No modulation
- No lookahead

### Elite Target (FabFilter Volcano 3)
- External sidechain input
- Mid/Side independent processing
- Modulation matrix (8 sources × 8 targets)
- Lookahead buffer for transient detection
- Latency compensation reporting

### Gap Analysis
Auto mode is innovative but limited to single parameter (pair selection). Missing modulation = no rhythmic movement. No sidechain = can't track kick drum for ducking effects.

### Roadmap

#### P0 - Mid/Side Processing (2-3 days)
**Goal:** Stereo width control and mastering applications

**Implementation:**
1. Add M/S encoder before filter:

```cpp
void MuseEngine::processMidSide(float* left, float* right, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        // Encode
        float mid = (left[i] + right[i]) * 0.5f;
        float side = (left[i] - right[i]) * 0.5f;

        // Process independently
        float midProcessed = filterMid_.processSample(mid);
        float sideProcessed = filterSide_.processSample(side);

        // Decode
        left[i] = midProcessed + sideProcessed;
        right[i] = midProcessed - sideProcessed;
    }
}
```

2. Add `msMix` parameter:
   - ID: `msMix`
   - Range: 0.0 (stereo) → 0.5 (mid-only) → 1.0 (side-only)
   - Default: 0.0

3. CPU cost: 2× (two filter instances)

**Validation:**
- Feed mono signal → mid channel only (side = 0)
- Feed L=1, R=-1 → side channel only (mid = 0)
- Verify phase coherency

---

#### P1 - External Sidechain (3-4 days)
**Goal:** Dynamic filtering based on external audio

**Implementation:**
1. Add sidechain bus in `BusesProperties` (`source/PluginProcessor.cpp:51-57`):

```cpp
PluginProcessor::PluginProcessor()
    : AudioProcessor(BusesProperties()
                    .withInput("Input", juce::AudioChannelSet::stereo(), true)
                    .withOutput("Output", juce::AudioChannelSet::stereo(), true)
                    .withInput("Sidechain", juce::AudioChannelSet::stereo(), false))  // NEW
```

2. Read sidechain buffer in `processBlock`:

```cpp
auto mainInput = getBusBuffer(buffer, true, 0);   // Main input
auto sidechainInput = getBusBuffer(buffer, true, 1);  // Sidechain

if (sidechainInput.getNumChannels() > 0) {
    // Calculate sidechain RMS
    float scRMS = calculateRMS(sidechainInput);

    // Modulate intensity based on sidechain level
    float modulatedIntensity = intensity * (1.0f - scRMS * sidechainAmount);
    filter_.setIntensity(modulatedIntensity);
}
```

3. Add `sidechainAmount` parameter:
   - ID: `sidechainAmount`
   - Range: 0.0 (no effect) → 1.0 (full ducking)
   - Default: 0.0

**Use Case:** Duck filter intensity on kick drum hits (sidechain from kick track)

**Validation:**
- Route kick drum to sidechain → intensity ducks on each hit
- No sidechain input → parameter has no effect

---

#### P2 - Modulation Matrix (7-10 days, v2.0)
**Goal:** Rhythmic movement and creative automation

**Implementation:**
1. Design `ModulationEngine` class:

```cpp
enum class ModSource { LFO1, LFO2, EnvFollower, StepSeq, MIDIVel, Aftertouch };
enum class ModTarget { Morph, Intensity, Mix, Drive };

class ModulationEngine {
    void setModulation(ModSource src, ModTarget tgt, float depth);
    float getModulatedValue(ModTarget tgt, float baseValue);
    void process(int numSamples);  // Advance LFOs, envelopes
};
```

2. UI: 8×8 grid with depth sliders:
   - Rows: 8 mod sources
   - Columns: 8 targets (all plugin parameters)
   - Cells: Depth slider (±100%)

3. Per-sample modulation in processBlock:

```cpp
float baseMorph = morphParam_ ? *morphParam_ : 0.5f;
float modulatedMorph = modEngine_.getModulatedValue(ModTarget::Morph, baseMorph);
filter_.setMorph(modulatedMorph);
```

**Deferred Reason:** v2.0 feature, requires stable architecture

---

#### P3 - Lookahead Buffer (3-4 days)
**Goal:** Pre-emptive ducking on transients

**Implementation:**
1. Add delay buffer (e.g., 10ms = 480 samples at 48kHz):

```cpp
juce::AudioBuffer<float> lookaheadBuffer_;
int lookaheadSamples_{480};

void prepare(double sr, int blockSize) {
    lookaheadBuffer_.setSize(2, blockSize + lookaheadSamples_);
    setLatencySamples(lookaheadSamples_);  // Report to DAW
}
```

2. Detect transients in lookahead window:

```cpp
float detectTransient(const float* buffer, int numSamples) {
    float maxSlope = 0.0f;
    for (int i = 1; i < numSamples; ++i) {
        float slope = std::abs(buffer[i] - buffer[i - 1]);
        maxSlope = std::max(maxSlope, slope);
    }
    return maxSlope;
}
```

3. Pre-duck intensity before transient hits

**Trade-off:** Adds 10ms latency (must be reported to DAW)

---

### Time Estimate
- P0: 2-3 days
- P1: 3-4 days
- **Total:** 5-7 days

### Risk Assessment
**Risk:** M/S processing doubles CPU usage
**Mitigation:** Make it optional (toggle parameter), warn users of CPU cost

**Risk:** Some DAWs poorly support sidechain routing
**Mitigation:** Test in major DAWs, document limitations

---

## 9. Platform Optimization

### Current State (Tier 3.5)
**Location:** `CMakeLists.txt`, `dsp/ZPlaneFilter_fast.h:28-38`

- VST3 + CLAP builds working (Windows only tested)
- SSE2 detection exists but not used
- No ARM NEON
- No macOS Metal rendering
- AU format not enabled

### Elite Target (u-he plugins)
- AU format (macOS, iOS)
- AAX format (Pro Tools)
- Apple Silicon native (ARM64)
- Metal-accelerated UI (macOS)
- Linux VST3 build

### Gap Analysis
Windows-only testing = no macOS/iOS market. Missing AU = can't sell on Mac App Store. No AAX = Pro Tools users excluded.

### Roadmap

#### P0 - Enable AU Format (1 day)
**Goal:** macOS market access

**Implementation:**
1. Uncomment AU in `CMakeLists.txt`:

```cmake
juce_add_plugin(Muse
    FORMATS VST3 AU CLAP  # Add AU here
    # ...
)
```

2. Test on macOS:
   - Build: `cmake --build build --config Release --target Muse_AU`
   - Install: `~/Library/Audio/Plug-Ins/Components/Muse.component`
   - Validate: Open in Logic Pro, verify automation works

**Blocker:** Requires Mac hardware for testing (or CI macOS runner)

---

#### P1 - Apple Silicon (ARM64) Support (2-3 days)
**Goal:** Native performance on M1/M2/M3 Macs

**Implementation:**
1. Add ARM NEON SIMD path (`dsp/ZPlaneFilter_fast.h:35-38`):

```cpp
#if ZPLANE_HAS_NEON
inline float32x4_t processSIMD_NEON(float32x4_t x) noexcept {
    // Load 4 samples
    float32x4_t y = vfmaq_f32(z1_vec, b0_vec, x);  // FMA on ARM
    float32x4_t t1 = vfmaq_f32(z2_vec, b1_vec, x);
    z1_vec = vmlsq_f32(t1, a1_vec, y);  // t1 - a1 * y
    z2_vec = vfmsq_f32(vmulq_f32(b2_vec, x), a2_vec, y);  // b2*x - a2*y
    return y;
}
#endif
```

2. Build universal binary (x86_64 + arm64):

```cmake
set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")
```

**Benchmark Target:**
- Scalar on ARM: 80 cycles/sample
- NEON: 30 cycles/sample (2.6×)

**Validation:**
- Test on M1 Mac Mini
- Verify universal binary works on Intel Mac

---

#### P2 - AAX Format (Pro Tools) (3-5 days)
**Goal:** Pro Tools user base access

**Implementation:**
1. Sign up for [Avid developer account](https://www.avid.com/alliance-partner-program) ($299/year)
2. Download AAX SDK
3. Add to CMakeLists.txt:

```cmake
juce_add_plugin(Muse
    FORMATS VST3 AU AAX CLAP  # Add AAX
    AAX_IDENTIFIER com.museaudio.muse.aax
    # ...
)
```

4. Code-sign binary with Avid certificate

**Blocker:**
- Paid developer account required
- AAX SDK license agreement
- Must pass Avid certification (can take weeks)

**Deferred Reason:** High barrier to entry, small market share (Pro Tools users)

---

#### P3 - Metal Rendering (macOS) (5-7 days)
**Goal:** 60 FPS, lower CPU on macOS

**Implementation:**
1. Replace `juce::Component` with `juce::OpenGLContext` on macOS
2. Use Metal backend (`CAMetalLayer`)
3. Render powder coat as Metal texture

**Expected Gain:** 60 FPS, 20-30% lower CPU vs software rendering

**Deferred Reason:** OpenGL already provides 60 FPS, diminishing returns

---

#### P3 - Linux Build (2-3 days)
**Goal:** Linux audio community support

**Implementation:**
1. Test CMake build on Ubuntu 22.04:

```bash
sudo apt install build-essential cmake libasound2-dev \
    libfreetype6-dev libx11-dev libxrandr-dev libxinerama-dev \
    libxcursor-dev libgl1-mesa-dev

cmake -B build
cmake --build build
```

2. Fix any GCC-specific issues (likely none, JUCE is cross-platform)
3. Add Linux to CI workflow

**Market:** Small but vocal community (Bitwig, Reaper, Ardour users)

---

### Time Estimate
- P0: 1 day
- P1: 2-3 days
- **Total:** 3-4 days

### Risk Assessment
**Risk:** macOS requires Mac hardware for testing
**Mitigation:** Use GitHub Actions macOS runner, or borrow Mac for testing

**Risk:** ARM64 SIMD bugs are hard to debug without device
**Mitigation:** Use CI for automated testing, remote into Mac Mini

---

## 10. Professional Polish

### Current State (Tier 3.5)
- Basic plugin (VST3/CLAP)
- No crash reporting
- No copy protection
- No demo mode
- No update mechanism
- No analytics

### Elite Target (Soundtoys level)
- Crash reporting (Sentry.io integration)
- iLok or custom license server
- 14-day trial mode (preset save disabled)
- Auto-update checker (semantic versioning)
- Opt-in anonymous usage analytics
- Professional installer (NSIS/WiX on Windows, PKG on macOS)

### Gap Analysis
No protection = piracy risk. No crash reports = blind to user issues. No analytics = can't prioritize features.

### Roadmap

#### P0 - Crash Reporting (2-3 days)
**Goal:** Proactive bug fixing

**Implementation:**
1. Integrate [Sentry.io](https://sentry.io) SDK:

```cpp
#include <sentry.h>

void initSentry() {
    sentry_options_t* options = sentry_options_new();
    sentry_options_set_dsn(options, "https://...@sentry.io/...");
    sentry_options_set_release(options, "muse@1.0.0");
    sentry_init(options);
}
```

2. Catch exceptions in critical code:

```cpp
void processBlock(...) {
    try {
        // ... DSP processing
    } catch (const std::exception& e) {
        sentry_capture_event(sentry_value_new_message_event(
            SENTRY_LEVEL_ERROR, "processBlock", e.what()));
    }
}
```

3. Upload stack trace + context:
   - OS version
   - DAW name (via `PluginHostType`)
   - Plugin version
   - CPU architecture

**Privacy:** Only crash data, no audio, opt-in on first launch

**Validation:**
- Trigger crash → verify Sentry receives report
- Check Sentry dashboard for stack trace

---

#### P1 - License System (5-7 days)
**Goal:** Prevent piracy, enable trial mode

**Implementation:**
1. Design simple license key format:

```
Format: [MachineID]-[Signature]
Example: A3F2-9B71-C4D8-E6A5-1234ABCD
```

2. Machine ID generation:

```cpp
juce::String getMachineID() {
    juce::MACAddress mac = juce::MACAddress::getDefaultAddress();
    juce::String id = juce::MD5(mac.toString().toUTF8()).toHexString();
    return id.substring(0, 16);  // 16-char machine ID
}
```

3. License check in PluginProcessor constructor:

```cpp
if (!licenseManager_.isValid()) {
    // Trial mode: 14 days, preset save disabled
    if (licenseManager_.getTrialDaysRemaining() <= 0) {
        // Show "Trial Expired" watermark in UI
        // Mute audio after 30 seconds
    }
}
```

4. UI: License entry dialog

**Alternative:** Use [PluginGuard](https://pluginguard.com) or [KeyGen](https://keygen.sh) ($500-1000/year)

**Validation:**
- Valid license → full functionality
- Invalid license → trial mode (14 days)
- Trial expired → watermark + 30s audio mute

---

#### P2 - Auto-Update Checker (2-3 days)
**Goal:** Keep users on latest version

**Implementation:**
1. Host `version.json` on website:

```json
{
    "latest": "1.0.1",
    "url": "https://museaudio.com/downloads/muse-1.0.1-win.exe",
    "changelog": "Fixed: Crash on buffer size change"
}
```

2. Check on plugin launch (background thread):

```cpp
void checkForUpdates() {
    juce::URL versionURL("https://museaudio.com/version.json");
    auto response = versionURL.readEntireTextStream();
    auto json = juce::JSON::parse(response);

    juce::String latestVersion = json["latest"].toString();
    if (latestVersion > CURRENT_VERSION) {
        // Show notification in UI
    }
}
```

3. Notification: "Update available: v1.0.1 (click to download)"

**Privacy:** No user data sent, just version check (HTTP GET)

**Validation:**
- Host fake version.json with newer version → verify notification shows
- Click notification → opens download page

---

#### P3 - Usage Analytics (3-4 days)
**Goal:** Data-driven feature prioritization

**Implementation:**
1. Integrate [Mixpanel](https://mixpanel.com) or custom telemetry:

```cpp
void trackEvent(const juce::String& event, const juce::var& properties) {
    if (!analyticsEnabled_) return;  // Opt-in only

    juce::var payload = juce::var(new juce::DynamicObject());
    payload.getDynamicObject()->setProperty("event", event);
    payload.getDynamicObject()->setProperty("properties", properties);

    juce::URL("https://api.mixpanel.com/track").withPOSTData(
        juce::JSON::toString(payload)).readEntireTextStream(true);  // Async
}
```

2. Track events:
   - Plugin opened
   - Parameter changed (which parameter)
   - Preset loaded (which preset)
   - Quality mode changed

3. Opt-in dialog on first launch:

```
MuseAudio collects anonymous usage data to improve the plugin.
No audio or personal information is transmitted.

[ ] Send anonymous usage statistics
[Continue]
```

**Privacy:**
- Anonymous UUID (no name, email, etc.)
- GDPR compliant (opt-in, can opt-out later)
- No audio data

**Validation:**
- Enable analytics → trigger event → verify Mixpanel dashboard receives it

---

#### P3 - Professional Installer (2-3 days per platform)
**Goal:** Streamlined installation experience

**Windows (NSIS):**
```nsis
Name "Muse Audio Plugin"
OutFile "Muse-1.0.0-Win-Installer.exe"

Section "VST3"
    SetOutPath "$PROGRAMFILES64\Common Files\VST3"
    File "Muse.vst3"
SectionEnd

Section "CLAP"
    SetOutPath "$PROGRAMFILES64\Common Files\CLAP"
    File "Muse.clap"
SectionEnd
```

**macOS (PKG):**
```bash
pkgbuild --root build/Muse_artefacts/Release/AU \
         --identifier com.museaudio.muse.pkg \
         --version 1.0.0 \
         --install-location ~/Library/Audio/Plug-Ins/Components \
         Muse-1.0.0-macOS.pkg

# Code sign
productsign --sign "Developer ID Installer: ..." \
             Muse-1.0.0-macOS.pkg \
             Muse-1.0.0-macOS-Signed.pkg
```

**Blocker:** Requires code signing certificate ($99-300/year)

---

### Time Estimate
- P0: 2-3 days
- P1: 5-7 days
- **Total:** 7-10 days

### Risk Assessment
**Risk:** License systems add complexity and user friction
**Mitigation:** Consider "honor system" for v1.0, add protection in v1.1 if piracy becomes issue

---

## Summary: Priority Matrix

### NEXT 2 WEEKS (Tier 4 → Commercial Ready)

**Focus:** Core functionality, stability, user value

| Task | Priority | Days | Tier Impact |
|------|----------|------|-------------|
| P0 Modular Engine | Critical | 3-5 | Enables all future features |
| P0 SIMD Paths | Critical | 2-3 | 2-3× CPU efficiency |
| P0 Factory Presets | Critical | 3-5 | Immediate user value |
| P0 pluginval + CI | Critical | 3-4 | Stability & compatibility |
| P0 Resizable UI | High | 2-3 | Accessibility |

**Total: 13-20 days → Tier 4 (Professional Quality)**

---

### NEXT 1-2 MONTHS (Tier 5 → Elite Features)

**Focus:** Advanced features, polish, cross-platform

| Task | Priority | Days | Tier Impact |
|------|----------|------|-------------|
| P1 Oversampling | High | 3-4 | Audio quality |
| P1 Spectrum Analyzer | High | 4-5 | Visual feedback |
| P1 Preset Browser | High | 4-5 | UX improvement |
| P1 State History/Undo | High | 2-3 | Professional workflow |
| P1 Mid/Side + Sidechain | Medium | 5-7 | Advanced routing |
| P1 AU + ARM64 | Medium | 3-4 | macOS market |

**Total: +21-28 days → Tier 5 (Feature-Complete)**

---

### v2.0 ROADMAP (Tier 6 → Elite Status)

**Focus:** Innovation, top-tier performance, commercial polish

| Task | Priority | Days | Tier Impact |
|------|----------|------|-------------|
| P2 Modulation Matrix | Medium | 7-10 | Creative automation |
| P2 OpenGL/Metal | Low | 5-7 | 60 FPS UI |
| P2 AAX Format | Low | 3-5 | Pro Tools market |
| P3 Linear-Phase Mode | Low | 5-7 | Mastering applications |
| P3 Professional Polish | Medium | 7-10 | Licensing, analytics, etc. |

**Total: +27-39 days → Tier 6 (Elite Status)**

---

## Critical Path

```
Modular Engine (P0)
    ↓
SIMD Paths (P0)
    ↓
Factory Presets (P0)
    ↓
pluginval + CI (P0)
    ↓
Oversampling (P1)
    ↓
Spectrum Analyzer (P1)
    ↓
[Tier 5 Achieved]
```

**Everything else can be parallelized or deferred.**

---

## Estimated Timeline

### Full-Time Development (8 hours/day)
- **Tier 4:** 13-20 days = 2-3 weeks
- **Tier 5:** +21-28 days = +3-4 weeks
- **Tier 6:** +27-39 days = +4-5 weeks
- **Total:** 61-87 days = **2-3 months**

### Part-Time Development (4 hours/day)
- **Tier 4:** 26-40 days = 4-6 weeks
- **Tier 5:** +42-56 days = +6-8 weeks
- **Tier 6:** +54-78 days = +7-11 weeks
- **Total:** 122-174 days = **4-6 months**

### With Claude Code Assistance (Estimated 1.5-2× Speedup)
- **Tier 4:** 7-13 days = 1-2 weeks
- **Tier 5:** +14-19 days = +2-3 weeks
- **Tier 6:** +18-26 days = +2.5-4 weeks
- **Total:** 39-58 days = **6-9 weeks** (1.5-2 months)

---

## Next Steps

### Immediate Actions (Today)
1. **Run pluginval** (1 hour) → Document baseline compliance
2. **Set up GitHub Actions CI** (2-3 hours) → Prevent regressions
3. **Create 5 factory presets** (1 hour) → Quick user value

### This Week
1. **P0 Modular Engine** → Start refactoring PluginProcessor
2. **P0 SIMD Paths** → Implement SSE2 biquad processing
3. **P0 Factory Presets** → Design 30 presets

### This Month
- Complete Tier 4 (Commercial Ready)
- Begin Tier 5 (Elite Features)

---

**Roadmap Version:** 1.0
**Last Updated:** November 11, 2025
**Status:** Ready for execution
**Estimated Completion (Tier 6):** Q1 2026 (with Claude Code assistance)

---

**END OF ROADMAP**
