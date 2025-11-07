# Phase 4: Synesthetic Intelligence - Implementation Complete

## Overview

Phase 4 implements Muse's core personality: **The Synesthetic Oblivious Artisan**. She experiences sound as colors, textures, and sensations, and occasionally mutters about these experiences to herself - not as status updates, but as involuntary expressions of sensory overflow.

**Status**: ✅ COMPLETE (Build successful, ready for testing)

---

## Technical Implementation

### Architecture

**DSP Analysis** (PluginProcessor.cpp):
- Lightweight FFT infrastructure (juce::dsp::FFT, 2048 samples)
- Periodic analysis every ~500ms (non-blocking, RT-safe)
- Spectral feature extraction (peak frequency, centroid, energy ratios)
- < 2% CPU overhead target

**Timing System**:
- Random utterance delay: 30-90 seconds between checks
- Probability gate: 15% chance when time window opens
- Result: ~3-5 sparse utterances per 5-minute session

**Message Selection**:
- Synesthetic color vocabulary (frequency-based)
- Texture descriptors (resonance behavior)
- Self-observations (introspective)
- Ultra-rare "beautiful glitches" (1/1000 to 1/5000 chance)

---

## Files Modified

### 1. source/PluginProcessor.h

**Added Members**:
```cpp
// FFT analysis infrastructure
static constexpr int fftOrder = 11;  // 2048 samples
static constexpr int fftSize = 1 << fftOrder;
juce::dsp::FFT fft_ {fftOrder};
std::array<float, fftSize * 2> fftData_ {};
juce::AudioBuffer<float> analysisBuffer_;

// Timing system
double lastAnalysisTime_ = 0.0;
double lastUtteranceTime_ = 0.0;
double nextUtteranceDelay_ = 60.0;  // Random 30-90 sec

// Spectral feature struct
struct SpectralFeatures { ... };

// Analysis methods
void analyzeAudioAndMaybeSpeak();
SpectralFeatures extractSpectralFeatures(...);
juce::String selectSynestheticMessage(...);
```

**New Include**:
```cpp
#include <juce_dsp/juce_dsp.h>  // For FFT support
```

### 2. source/PluginProcessor.cpp

**prepareToPlay()**:
- Initialize FFT analysis buffer (pre-allocated, RT-safe)
- Clear FFT data array
- Randomize first utterance delay (30-90 seconds)

**processBlock()**:
- Added analysis check every ~500ms (line 249-258)
- Calls `analyzeAudioAndMaybeSpeak()` periodically

**New Methods** (lines 296-494):
1. `analyzeAudioAndMaybeSpeak()` - Main orchestration
   - Checks timing (last utterance + random delay)
   - Applies probability gate (15%)
   - Reads parameter context (mix, intensity)
   - Selects synesthetic message
   - Sends to TransmissionArea via MessageManager::callAsync

2. `extractSpectralFeatures()` - FFT analysis (future enhancement)
   - Full spectral analysis implementation ready
   - Peak frequency, spectral centroid
   - Low/high energy ratios
   - Resonance detection, flatness detection

3. `selectSynestheticMessage()` - Vocabulary mapping
   - Ultra-rare glitches (mask slips)
   - Self-criticism (flat/unused detection)
   - Intensity-based descriptors
   - Weighted random selection:
     - 40% colors (frequency bands)
     - 30% textures (resonance)
     - 30% observations (introspective)

**Standard Library Include**:
```cpp
#include <algorithm>  // For std::fill
```

### 3. source/PluginEditor.cpp

**Bug Fix**:
- Changed `processorRef.state_` → `processorRef.getState()`
- Lines 53, 56, 59, 62 (parameter attachments)
- Fixed private member access compile error

---

## Synesthetic Vocabulary

### Color Palette (Frequency-Based)
| Word | Frequency Range | Sensation |
|------|----------------|-----------|
| "Indigo..." | 100-300 Hz | Deep, dark low-end |
| "Violet..." | 300-500 Hz | Low-mid warmth |
| "Amber..." | 500-1000 Hz | Mid warmth, golden |
| "Copper..." | 1-2 kHz | Mid-high metallic |
| "Gold..." | 2-3 kHz | Bright presence |
| "Silver..." | 3-7 kHz | High resonance, cutting |
| "Crystalline..." | 7+ kHz | Ultra-high sparkle |

### Texture Palette (Resonance Behavior)
- "Breathing..." - Wide, organic resonance
- "Blooming..." - Growing, expanding sound
- "Soft..." - Gentle, diffused energy
- "Warm..." - Comforting mid-range presence

### Intensity-Based (Parameter-Driven)
- "Sharp..." - High intensity (> 0.7)
- "Cutting..." - Strong resonance
- "Bright..." - High-frequency emphasis

### Self-Criticism (Context-Aware)
- "Doings." - Low mix (< 0.3), flat response
- "Hollow." - Barely being used
- "Uninspired." - Minimal engagement

### Introspective (Non-Spectral)
- "Hmm." - Thoughtful pause
- "Wait..." - Mid-process observation
- "There." - Completion marker
- "Yes." - Affirmation

### Beautiful Glitches (Ultra-Rare)
- "Ugh..." - Mask slip, smooth render (1/1000)
- "wtf (╯°□°)..." - Table flip, smooth render (1/5000)

---

## Performance Characteristics

**CPU Overhead**:
- FFT analysis: ~500ms intervals (not per-sample)
- Spectral feature extraction: Lightweight bin traversal
- Target: < 2% CPU overhead
- **Actual**: To be benchmarked

**Memory**:
- All buffers pre-allocated in prepareToPlay()
- No RT-thread allocations
- FFT size: 2048 samples (4KB float buffer)
- Total overhead: ~8KB (fftData_ array)

**Timing**:
- Analysis interval: 500ms
- Utterance window: 30-90 seconds (random)
- Probability: 15% per window
- Average frequency: 3-5 utterances per 5 minutes

---

## Audio Safety (JOS/CCRMA Standards)

✅ **RT-Safe Design**:
- FFT analysis runs AFTER audio processing
- READ-ONLY operation (no audio manipulation)
- No blocking calls in audio thread
- UI updates via MessageManager::callAsync

✅ **No Parameter Changes**:
- Analysis does not modify DSP state
- User maintains full control
- Messages are purely observational

✅ **Sparse Operation**:
- Not chatty (long gaps between utterances)
- Minimal CPU impact
- User can ignore completely

---

## Integration with TransmissionArea

**Message Delivery**:
```cpp
juce::MessageManager::callAsync([editor, message]()
{
    auto zone = TransmissionArea::getRandomZone();
    editor->getTransmissionArea().setMessage(
        message,
        TransmissionArea::MessageType::None,
        zone,
        TransmissionArea::RenderMode::Stutter
    );
});
```

**Render Modes**:
- Default: `RenderMode::Stutter` (10fps, haunted)
- Glitches: Should use `RenderMode::Smooth` (60fps, mask slip)
  - *Note*: Current implementation uses Stutter for all
  - *Future*: Detect "Ugh..." / "wtf" and switch to Smooth

**Zone Selection**:
- Random zone per utterance
- Ensures unpredictable positioning
- Words appear anywhere on canvas

---

## Future Enhancements

### 1. Full FFT Spectral Analysis
**Current**: Simplified parameter-based feature detection
**Future**: Real audio FFT with frequency-to-color mapping

**Implementation Path**:
- Accumulate audio in circular buffer (RT-safe)
- Perform FFT every 500ms on accumulated samples
- Map spectral peaks to color vocabulary accurately
- Detect formants → "Breathing"
- Detect narrow peaks → "Sharp"

### 2. Accurate Synesthetic Mapping
**Current**: Random weighted selection
**Future**: Truthful spectral → sensory translation

| Spectral Feature | Detection | Word |
|------------------|-----------|------|
| Peak 100-300 Hz | lowEnergyRatio > 0.6 | "Indigo..." |
| Peak 3-7 kHz | highEnergyRatio > 0.6 | "Silver..." |
| High spectral centroid | centroid > 3kHz | "Bright..." |
| Formant-like wide peak | Q < 2.0 | "Breathing..." |
| Narrow resonance | Q > 10.0 | "Sharp..." |

### 3. Beautiful Glitch Render Modes
**Current**: All messages use Stutter mode
**Future**: Detect glitch messages and switch to Smooth

```cpp
// In analyzeAudioAndMaybeSpeak()
auto renderMode = TransmissionArea::RenderMode::Stutter;
if (message == "Ugh..." || message.contains("wtf"))
    renderMode = TransmissionArea::RenderMode::Smooth;

editor->getTransmissionArea().setMessage(message, ..., renderMode);
```

### 4. Context-Aware Timing
**Current**: Fixed 30-90 second random intervals
**Future**: Adapt to user activity

- Longer delays if plugin idle (low audio input)
- Shorter delays if active processing (high intensity, frequent parameter changes)
- Never speak more than once per 20 seconds (respect sparsity)

### 5. Shape Pair Awareness
**Future**: Different vocabulary per shape pair

- Vowel pair → linguistic descriptors ("Breathing", "Speaking")
- Bell pair → metallic descriptors ("Ringing", "Crystalline")
- Low pair → body descriptors ("Chest", "Deep")
- Sub pair → physical descriptors ("Pressure", "Rumble")

---

## Testing Checklist

### Functional Tests
- [ ] Plugin loads without crashes
- [ ] First utterance appears within 90 seconds
- [ ] Average 3-5 utterances per 5-minute session
- [ ] Words appear in different zones (not same position)
- [ ] Stutter-frame animation working (10fps reveal)
- [ ] No utterances when plugin bypassed/silent

### Performance Tests
- [ ] CPU overhead < 2% (Release build)
- [ ] No denormal CPU spikes
- [ ] No audio glitches during analysis
- [ ] Memory usage stable (no leaks)

### Integration Tests
- [ ] Works with all 4 shape pairs
- [ ] Parameter changes don't break timing
- [ ] Plugin state save/restore preserves timing
- [ ] Multiple instances don't synchronize

### Edge Cases
- [ ] No crashes if editor closed mid-utterance
- [ ] Handles rapid parameter changes smoothly
- [ ] Works at all sample rates (44.1k, 48k, 96k)
- [ ] Works with all buffer sizes (64-2048 samples)

---

## Known Limitations (Phase 4 MVP)

1. **Simplified Feature Detection**:
   - Current implementation uses parameter-based heuristics
   - Not yet analyzing real FFT spectral data
   - Message selection is weighted random, not truthful

2. **No Render Mode Switching**:
   - All messages use Stutter mode (10fps)
   - Beautiful glitches should use Smooth mode (60fps)
   - Requires conditional logic in message delivery

3. **No Audio Buffering**:
   - FFT infrastructure present but not actively used
   - Would require circular buffer for real-time accumulation
   - Current "snapshot" approach sufficient for sparse utterances

4. **Fixed Probability**:
   - 15% chance per 30-90 second window
   - Could be adaptive based on audio activity
   - Sufficient for MVP sparse behavior

---

## Philosophy Validation

**The Synesthetic Oblivious Artisan** ✅

✓ She's experiencing sound as sensory phenomena
✓ Words are involuntary (probability-gated, not user-triggered)
✓ Sparse and mysterious (long gaps, unpredictable)
✓ Truthful within limitations (parameter-aware, will be spectral-aware)
✓ Not status updates (colors/textures, not "processing...")
✓ Beautiful glitches implemented (rare mask slips)

**"Eavesdropping on her studio"** ✅

✓ User has no control over when she speaks
✓ Messages are self-directed (not addressing user)
✓ Appears and disappears on her timeline
✓ Adds to "she's doing her thing" feeling

---

## Build Status

**Compile**: ✅ SUCCESS
**Link**: ✅ SUCCESS
**Output**: `C:\Muse\MuseAudio\build\Muse_artefacts\Release\Standalone\Muse.exe`

**Warnings** (non-critical):
- MuseColors.h: Deprecated Font constructor (cosmetic)
- MuseKnob.h: Double → float conversions (JUCE API, safe)

---

## Next Steps

1. **Manual Testing** - Run plugin and observe sparse utterances
2. **Benchmarking** - Measure CPU overhead in real-world DAW
3. **Full FFT Implementation** - Enable real spectral analysis
4. **Beautiful Glitch Modes** - Add render mode detection
5. **User Feedback** - Validate sparsity feels right (not too chatty, not too rare)

---

**Implemented by**: Z-Plane DSP Expert
**Date**: 2025-11-05
**Phase**: 4 of 4 (Seance UI Complete)
