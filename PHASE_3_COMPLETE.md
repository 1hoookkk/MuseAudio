# Phase 3: Environmental Voice System - COMPLETE

**Date**: 2025-11-05
**Status**: Implemented and ready for testing

---

## Overview

The TransmissionArea component has been transformed from a contained card/box into an environmental floating word system. Words now appear as separate ghost phenomena floating IN FRONT of the silhouette across the entire canvas.

---

## Core Philosophy

**THE CRITICAL REFRAME**:
- **NOT**: Words emanating FROM her (chatbot behavior)
- **YES**: Words floating IN FRONT of her (separate ghost phenomena she's summoned)

She's the SOURCE, but the words are their OWN entity.

---

## Implementation Details

### Files Modified

#### 1. `source/ui/TransmissionArea.h`
**Major Changes**:
- Added `WordZone` enum with 6 predefined zones across the canvas
- Added `AnimationPhase` enum for multi-stage animation system
- Removed card background rendering (now fully transparent when idle)
- Added fade in/out system (300ms fade in, 2 second fade out)
- Added `calculateZoneBounds()` method for zone positioning
- Added `hideWord()` method for manual dismissal
- Updated `setMessage()` signature to accept zone parameter

**New Animation Phases**:
1. `Idle` - Fully transparent (no rendering)
2. `FadingIn` - Appearing over 300ms
3. `Revealing` - Stutter-frame text reveal (10fps, 1-2 chars per frame)
4. `Visible` - Fully revealed, holding for 2 seconds
5. `FadingOut` - Disappearing over 2 seconds

**Word Zones** (based on 640x480 canvas):
- `AboveHead`: x: 300-400, y: 60-100 (above silhouette)
- `LeftSpace`: x: 100-180, y: 180-260 (left of her)
- `RightSpace`: x: 460-540, y: 180-260 (right - where she gazes)
- `CenterHigh`: x: 280-360, y: 140-180 (center upper)
- `NearMorphKnob`: x: 420-500, y: 80-140 (near primary control)
- `FloatingLow`: x: 200-440, y: 340-380 (lower center space)

#### 2. `source/PluginEditor.cpp`
**Major Changes**:
- Changed `transmissionArea.setBounds()` to cover full canvas (was bottom strip only)
- Added `testFloatingWords()` method for demonstration
- Added "Test Words" button for triggering demo
- Updated constructor to initialize transmission area with zone parameter

**Test Method**:
- Cycles through 6 test messages across different zones
- Demonstrates both Stutter (10fps) and Smooth (60fps) render modes
- Showcases fade in/out system
- Messages appear at 4-second intervals over 24 seconds

#### 3. `source/PluginEditor.h`
**Minor Changes**:
- Added `testFloatingWords()` public method declaration
- Added `testWordsButton` member variable

---

## Technical Architecture

### Animation System Flow

```
User calls setMessage(text, type, zone, mode)
    ↓
AnimationPhase::FadingIn (300ms)
    - fadeAlpha: 0.0 → 1.0
    - Text not yet visible
    ↓
AnimationPhase::Revealing (variable duration)
    - Stutter-frame reveal (10fps or 60fps)
    - 1-2 characters per frame
    - fadeAlpha: 1.0 (fully visible)
    ↓
AnimationPhase::Visible (2 seconds)
    - Hold fully revealed text
    - fadeAlpha: 1.0
    ↓
AnimationPhase::FadingOut (2 seconds)
    - fadeAlpha: 1.0 → 0.0
    - Text disappears
    ↓
AnimationPhase::Idle
    - Fully transparent
    - No rendering
    - Timer stopped
```

### Performance Characteristics

**Timer Frequencies**:
- Stutter mode: 100ms (10 FPS) - default Muse "performance"
- Smooth mode: 16ms (60 FPS) - rare mask slips during struggle
- Fade-out: 16ms (60 FPS) - smooth disappearance

**Memory Usage**: Minimal
- No persistent rendering when idle
- Single string storage
- No texture/image caching

**CPU Usage**: Very low
- Timer only runs during active animation phases
- Early exit in paint() when idle
- Simple rectangle positioning (no complex transforms)

---

## Design System Compliance

✅ **Colors**: All from MuseColors.h
- Text: `Muse::Colors::TextMuseVoice` (lilac/warm tone)
- Background: None (fully transparent)

✅ **Typography**: Georgia serif via `Muse::Typography::getMuseVoiceFont()`

✅ **Spacing**: Zone bounds use coordinate positioning (not spacing tokens - intentional for environmental effect)

✅ **Animations**:
- Fade in: 300ms (design spec compliant)
- Fade out: 2 seconds (gentle, haunting disappearance)
- Stutter-frame: 10fps (100ms intervals - Muse's signature)

---

## Usage Examples

### Basic Usage (Single Zone)
```cpp
transmissionArea.setMessage(
    "Doodling...",
    TransmissionArea::MessageType::Loading,
    TransmissionArea::WordZone::CenterHigh,
    TransmissionArea::RenderMode::Stutter
);
```

### Random Zone Selection
```cpp
auto randomZone = TransmissionArea::getRandomZone();
transmissionArea.setMessage(
    "Pondering...",
    TransmissionArea::MessageType::Loading,
    randomZone,
    TransmissionArea::RenderMode::Stutter
);
```

### Smooth Mode (Mask Slips)
```cpp
transmissionArea.setMessage(
    "Ugh...",
    TransmissionArea::MessageType::Loading,
    TransmissionArea::WordZone::NearMorphKnob,
    TransmissionArea::RenderMode::Smooth  // 60fps - rare struggle
);
```

### Manual Dismissal
```cpp
// Start fade-out immediately (skips Visible phase)
transmissionArea.hideWord();
```

---

## Testing Instructions

### Manual Testing
1. Build the plugin: `cmake --build build --config Release`
2. Launch the standalone: `build/Muse_artefacts/Release/Standalone/Muse.exe`
3. Click the **"Test Words"** button (top-right, below "Inspect")
4. Observe:
   - Words appearing in different zones across the canvas
   - Fade in over 300ms
   - Stutter-frame reveal (10fps)
   - Words fading out after 2 seconds
   - One smooth (60fps) example at 16 seconds

### Expected Behavior
- Words should appear to float in empty space
- No card background or visual container
- Fully transparent when idle (no rendering)
- Smooth fade in, stuttered reveal, smooth fade out
- Messages cycle through all 6 zones over 24 seconds

---

## Integration with Existing Systems

### Parameter Attachments
No changes required - transmission area is purely visual (no audio parameters)

### Melatonin Inspector
Transmission area covers full canvas but remains transparent when idle - inspector can still inspect all other components

### Future "Ask Muse" Button
When implemented, can trigger word animations like:
```cpp
void onAskMuseClicked()
{
    auto zone = TransmissionArea::getRandomZone();
    transmissionArea.setMessage(
        TransmissionArea::getRandomLoadingMessage(),
        TransmissionArea::MessageType::Loading,
        zone,
        TransmissionArea::RenderMode::Stutter
    );

    // After parameter randomization completes...
    juce::Timer::callAfterDelay(2000, [this]() {
        transmissionArea.setMessage(
            TransmissionArea::getRandomSuccessMessage(),
            TransmissionArea::MessageType::Success,
            TransmissionArea::getRandomZone(),
            TransmissionArea::RenderMode::Stutter
        );
    });
}
```

---

## Next Steps

### Immediate Tasks
1. Build and test the system
2. Verify zone positioning with silhouette
3. Adjust zone bounds if needed (currently hardcoded for 640x480)

### Future Enhancements
1. **Dynamic Zone Positioning**: Calculate zones relative to silhouette bounds (not hardcoded)
2. **Zone Avoidance**: Ensure words don't overlap with knobs
3. **Multiple Words**: Support multiple simultaneous floating words (rare, mystical moments)
4. **Per-Zone Styling**: Different fonts/sizes for different zones
5. **Drift Animation**: Slow, subtle movement of words while visible
6. **Integration**: Connect to "Ask Muse" button, preset loading, etc.

### Known Limitations
1. Zone bounds are hardcoded for 640x480 canvas (not dynamic)
2. Only one word at a time (no simultaneous messages)
3. No collision detection with UI elements
4. Test button is temporary (should be removed before release)

---

## Code Quality

### JUCE Best Practices
✅ Component-based architecture
✅ Timer-based animation (no blocking)
✅ Early exit in paint() when idle
✅ Proper enum class usage
✅ const-correct methods
✅ JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR

### Performance
✅ Zero rendering when idle
✅ Timer stopped when not animating
✅ No dynamic allocations in paint()
✅ Simple rectangle calculations

### Maintainability
✅ Clear enum naming
✅ Well-documented header
✅ Separation of concerns (animation logic in component)
✅ Easy to extend (add new zones, animation phases)

---

## Philosophy Alignment

**"She's the SOURCE, but words are their OWN entity"** ✅
- Words float independently in space
- Not contained, not attached to her
- Separate ghost phenomena she's summoned

**"Unpredictable zone selection"** ✅
- `getRandomZone()` helper method provided
- Test method demonstrates varied positioning
- Never same location twice (by design)

**"Sparse and mysterious"** ✅
- Fully transparent when idle
- Gentle fade in/out (not jarring)
- Words disappear completely (not persistent)
- Only one message at a time (restraint)

**"Stutter-frame transmission"** ✅
- 10fps reveal (100ms per frame)
- 1-2 characters per frame (irregular)
- Smooth mode only for rare mask slips

---

## Summary

Phase 3 is **COMPLETE**. The environmental voice system transforms Muse's communication from a contained UI element into floating ghost phenomena that appear and disappear across the canvas. The implementation is performant, maintainable, and perfectly aligned with the "Brutalist Temple" aesthetic.

**Status**: Ready for user testing and refinement.
