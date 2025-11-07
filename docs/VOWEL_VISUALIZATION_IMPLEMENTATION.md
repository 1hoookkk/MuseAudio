# Lock-Free Vowel Visualization Implementation

## Overview

Implemented lock-free, real-time communication between the Z-plane DSP engine and the OLED mouth visualizer to display vowel formants (AA, EE, OO, etc.) based on the filter's shape pair and morph parameters.

## Architecture Pattern

```
Audio Thread (RT-safe)              UI Thread (30fps timer)
─────────────────────               ───────────────────────
processBlock()                      timerCallback()
    │                                    │
    ├─ Read pair/morph                   ├─ Read atomic
    ├─ Calculate VowelShape              │   (memory_order_relaxed)
    └─ Store in atomic ─────────────────>└─ Update OLEDMouth
       (memory_order_relaxed)                 │
                                              └─ Repaint LED matrix
```

## Implementation Details

### 1. Audio Thread → Atomic State (PluginProcessor)

**File**: `source/PluginProcessor.h` (lines 44-58)

```cpp
// Thread-safe vowel shape state for UI (read by PluginEditor Timer)
enum class VowelShape
{
    AA, AH, EE,     // Vowel pair (0): formant transitions
    OH, OO,         // Bell pair (1): resonance shapes
    Wide, Narrow,   // Low pair (2): width variations
    Neutral         // Sub pair (3): minimal movement
};
std::atomic<int> currentVowelShape_ {static_cast<int>(VowelShape::AH)};

VowelShape getCurrentVowelShape() const
{
    return static_cast<VowelShape>(currentVowelShape_.load(std::memory_order_relaxed));
}
```

**File**: `source/PluginProcessor.cpp` (lines 216-242)

```cpp
// Calculate vowel shape for UI visualization (RT-safe atomic write)
VowelShape newVowelShape = VowelShape::AH;
switch (pairIndex)
{
    case 0:  // VOWEL pair: AA → AH → EE
        if (morph < 0.33f)
            newVowelShape = VowelShape::AA;
        else if (morph < 0.67f)
            newVowelShape = VowelShape::AH;
        else
            newVowelShape = VowelShape::EE;
        break;
    // ... (other pairs)
}
currentVowelShape_.store(static_cast<int>(newVowelShape), std::memory_order_relaxed);
```

### 2. UI Polling Timer (PluginEditor)

**File**: `source/PluginEditor.h` (lines 22-23, 39-40)

```cpp
class PluginEditor : public juce::AudioProcessorEditor,
                     private juce::Timer
{
    // ...
    void timerCallback() override;
};
```

**File**: `source/PluginEditor.cpp`

**Constructor** (lines 125-127):
```cpp
// Start lock-free vowel state polling timer (30fps)
startTimerHz(30);
```

**Destructor** (line 136):
```cpp
stopTimer(); // Stop polling before destruction
```

**Timer Callback** (lines 285-297):
```cpp
void PluginEditor::timerCallback()
{
    // Lock-free read of vowel shape from audio thread (via atomic)
    auto vowelShape = processorRef.getCurrentVowelShape();

    // Map PluginProcessor::VowelShape to OLEDMouth::VowelShape
    auto mouthShape = static_cast<OLEDMouth::VowelShape>(vowelShape);

    // Update mouth visualization (thread-safe, already on message thread)
    oledMouth.setVowelShape(mouthShape);
}
```

### 3. LED Matrix Rendering (OLEDMouth)

**File**: `source/ui/OLEDMouth.h` (lines 88-95, 124-208)

The OLEDMouth component renders different LED patterns for each vowel shape:

- **AA** (Wide open): Full horizontal line at bottom
- **AH** (Mid open): Horizontal line in middle
- **EE** (Smile): Upward curve pattern
- **OH** (Round medium): Oval shape
- **OO** (Round tight): Small circle
- **Wide**: Full rectangle (all LEDs on)
- **Narrow**: Thin vertical line
- **Neutral**: Single horizontal line

## Real-Time Safety Checklist

✅ **No locks**: Pure atomic read/write with `memory_order_relaxed`
✅ **No allocations**: All state is pre-allocated
✅ **No blocking operations**: Atomic operations are lock-free
✅ **Audio thread only writes**: Single writer (processBlock)
✅ **UI thread only reads**: Single reader (timerCallback)
✅ **Proper timer lifecycle**: Started in constructor, stopped in destructor

## Vowel Shape Mapping

| Shape Pair | Morph Range | Vowel Shape | Visual Representation |
|------------|-------------|-------------|----------------------|
| VOWEL (0)  | 0.00 - 0.33 | AA          | Wide open (bottom)   |
| VOWEL (0)  | 0.33 - 0.67 | AH          | Mid open (middle)    |
| VOWEL (0)  | 0.67 - 1.00 | EE          | Smile (curved up)    |
| BELL (1)   | 0.00 - 0.50 | OH          | Round medium (oval)  |
| BELL (1)   | 0.50 - 1.00 | OO          | Round tight (circle) |
| LOW (2)    | 0.00 - 0.50 | Wide        | Full rectangle       |
| LOW (2)    | 0.50 - 1.00 | Narrow      | Thin line            |
| SUB (3)    | Any         | Neutral     | Minimal movement     |

## Testing Notes

**Build Status**: ✅ Compiles successfully (Release mode)
**Real-time Safety**: ✅ Zero locks, zero allocations
**Update Rate**: 30fps (33ms polling interval)
**Latency**: ~33ms max (one timer tick)

## Performance Impact

- **Audio Thread**: Negligible (single atomic store per block)
- **UI Thread**: ~30 atomic reads/second + repaint if changed
- **Memory**: Zero additional allocations (uses existing atomics)

## Future Enhancements

1. **Activity Level**: Add RMS/peak detection for breathing effect intensity
2. **Smooth Transitions**: Interpolate between vowel shapes for morphing animation
3. **Intensity Feedback**: Visual indicator of filter resonance strength
4. **Custom Shapes**: User-definable LED patterns per vowel

## References

- Z-plane filter implementation: `modules/zplane-dsp/include/zplane/ZPlaneFilter.h`
- Authentic EMU pole data: `modules/zplane-dsp/include/zplane/EMUAuthenticTables.h`
- JUCE Timer documentation: https://docs.juce.com/master/classTimer.html
- JUCE atomic best practices: https://docs.juce.com/master/group__juce__core-threads.html

---

**Implementation Date**: 2025-11-07
**Build Verified**: Release VST3
**Real-Time Safety**: Validated
