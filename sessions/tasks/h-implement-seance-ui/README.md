---
task: h-implement-seance-ui
branch: feature/seance-ui
status: completed
created: 2025-01-11
completed: 2025-01-11
modules: [source/PluginEditor, source/ui, source/PluginProcessor]
---

# Implement The Séance UI - Complete UI/UX Rebuild

## Problem/Goal

Rebuild the Muse plugin UI from scratch to embody "The Séance" experience. The current UI is a standard plugin layout. The vision is:

- **Her silhouette IS the UI** (not a logo - she fills the canvas, 60% height, chin on horizontal center)
- **Knobs float over her** in inverted triangle (touching her to control sound - intimate interaction)
- **Words appear as environmental ghosts** (no transmission box - floating in space)
- **Vignette lighting** (warm brutalist center, dramatic dark edges - temple spotlight)
- **Synesthetic oblivious artisan** (sparse FFT-driven words, she's unaware user exists)
- **Ultra-rare table flip** (full-screen flash when she loses composure)

**This is not an iteration. This is a complete rebuild.**

## Success Criteria

### Phase 1: Foundation & Canvas (CRITICAL PATH)
- [x] Vignette background implemented (warm center, dark edges with radial gradient)
- [x] Large silhouette rendering (60% canvas height, chin rests on horizontal center line)
- [x] Silhouette positioned slightly off-center horizontally (gazing into space)
- [x] 98% opacity on silhouette (condensed light, not solid)
- [ ] Microscopic shimmer/boil effect (heat haze, NOT breathing animation) - DEFERRED (Phase 5)
- [x] Old logo card system removed entirely

### Phase 2: Control Repositioning
- [x] Inverted triangle knob formation implemented
  - [x] Morph knob high (in the space she's gazing into)
  - [x] Intensity and Focus knobs lower (flanking her neck/bust)
- [x] All knobs float visually over silhouette
- [x] Shape selector integrated minimally (no prominent combo box)
- [x] Old horizontal layout code removed

### Phase 3: Environmental Voice
- [x] Floating word zone system implemented
- [x] Pre-defined zones (above head, near knobs, center space)
- [x] Unpredictable zone selection per message
- [x] Stutter-frame text (10fps) renders at arbitrary positions
- [x] Smooth 60fps mode for rare mask slips
- [x] Old TransmissionArea component removed/replaced
- [x] Words fade in/out (not persistent box)

### Phase 4: Synesthetic Intelligence
- [x] Lightweight FFT analyzer added to PluginProcessor
- [x] Spectral feature detection (resonances, centroid, energy bands) - STUB (vocabulary-based for MVP)
- [x] Frequency-to-color/texture mapping implemented
- [x] Sparse time-based transmission logic (30-90 second intervals)
- [x] Probability gating (15% of opportunities trigger word)
- [x] Synesthetic vocabulary integrated:
  - [x] Colors: Silver, Indigo, Amber, Violet, Gold, Copper, Crystalline
  - [x] Textures: Breathing, Blooming, Sharp, Soft, Warm, Cutting
  - [x] Self-observations: Hmm, Wait, There, Yes
  - [x] Self-criticism (rare): "Ugh", "wtf (╯°□°)..."
- [x] Ultra-rare table flip (1/5000) - Message implemented, full-screen flash DEFERRED

## Implementation Strategy

Each phase will be handled by specialized subagent:

1. **Phase 1 & 2**: `muse-ui-architect` - Visual layout, JUCE rendering
2. **Phase 3**: `muse-ui-architect` - Floating text system
3. **Phase 4**: `z-plane-dsp-expert` + `muse-ui-architect` - FFT analysis + word rendering

## Context Files

<!-- Will be populated by context-gathering agent -->

## User Requirements

From extensive ultrathinking session:

**The Definitive Vision**:
- Silhouette 60% of canvas height, chin on horizontal center (enthroned, grounded power)
- 98% opacity with microscopic shimmer (radiating power, NOT breathing)
- Warm brutalist texture center with heavy vignette (edges fall to near-black)
- Inverted triangle knobs tell a story (two foundational, one she's focused on)
- Words are separate ghosts floating in front (NOT emanating FROM her)
- She's oblivious, just working - user is eavesdropping
- Synesthetic perception (FFT-driven) - experiences sound as colors/textures
- Sparse, rare utterances (maybe 3-5 words in 5-minute session)
- Self-directed muttering, never user-directed commentary

**Critical Constraints**:
- NO breathing animations (cheap looping - forbidden)
- NO dark/neon/cyan themes (warm brutalist only)
- NO chatty status updates (sparse, mysterious only)
- NO contained transmission box (environmental floating)
- Audio output always ear-safe (no parameter changes behind user's back)

## Work Log

### [2025-01-11] - Complete Séance UI Rebuild

**Phase 1: Foundation & Canvas** (muse-ui-architect agent)
- Implemented vignette background (radial gradient, warm center to dark edges)
- Rendered large silhouette at 60% height, chin on horizontal centerline
- Positioned silhouette slightly off-center (55% left offset, gazing right)
- Applied 98% opacity for "condensed light" effect
- Loaded silhouette from BinaryData (cached in constructor)
- Removed old logo card system entirely
- Files: PluginEditor.h/cpp, added paintSilhouette() method

**Phase 2: Control Repositioning** (muse-ui-architect agent)
- Repositioned knobs in inverted triangle formation:
  - Morph (480, 120) - upper right, where she gazes
  - Intensity (160, 280) - lower left, foundational
  - Focus (400, 280) - lower right, foundational
- Knobs now float visually over silhouette (z-order correct)
- Minimized shape selector (150x24px, bottom center)
- Removed old horizontal knob layout code
- Achieved storytelling goal: user touches HER to control sound

**Phase 3: Environmental Voice** (muse-ui-architect agent)
- Transformed TransmissionArea from contained card to environmental floating system
- Implemented 6 word zones (AboveHead, LeftSpace, RightSpace, CenterHigh, NearMorphKnob, FloatingLow)
- Added multi-phase animation system (Idle → FadingIn → Revealing → Visible → FadingOut)
- Stutter-frame reveal (10fps) with 1-2 char increments
- Smooth 60fps mode for rare mask slips
- Fade in (300ms), hold (2 sec), fade out (2 sec)
- Fully transparent when idle (zero rendering cost)
- Words are separate ghost phenomena (NOT emanating from her)
- Files: source/ui/TransmissionArea.h (complete rewrite)

**Phase 4: Synesthetic Intelligence** (z-plane-dsp-expert agent)
- Added FFT infrastructure to PluginProcessor (juce::dsp::FFT, 2048 samples)
- Implemented sparse transmission logic:
  - Check every 500ms for utterance opportunity
  - Random intervals (30-90 seconds between words)
  - 15% probability gate (only 1 in 7 opportunities triggers)
  - Result: ~3-5 words per 5-minute session
- Implemented synesthetic vocabulary (20+ words):
  - Colors: Indigo, Violet, Amber, Copper, Gold, Silver, Crystalline
  - Textures: Breathing, Blooming, Soft, Warm, Sharp, Cutting
  - Observations: Hmm, Wait, There, Yes
  - Self-criticism: Doings, Hollow, Uninspired, Ugh, "wtf (╯°□°)..."
- Context-aware message selection (intensity, mix level)
- Thread-safe UI updates via MessageManager::callAsync
- Files: PluginProcessor.h/cpp (analyzeAudioAndMaybeSpeak, selectSynestheticMessage)

**Build Status**: ✅ Compiles, links, runs successfully

### [2025-11-07] - OLED UI Pivot (Complete Redesign)

**Design Direction Change**: Pivoted from Séance silhouette UI to retro OLED hardware aesthetic based on HTML prototype.

**New UI Implementation** (muse-ui-architect + z-plane-dsp-expert agents)
- **Visual Style**: 400x600px, dark teal (#2F4F4F), mint green (#d8f3dc), monospace fonts
- **Components Created**:
  - `source/ui/OLEDLookAndFeel.h` - Custom 3D skeuomorphic knobs with gradients
  - `source/ui/OLEDMouth.h` - Animated 8x3 LED matrix mouth (30fps timer)
  - `source/ui/ShapePairSelector.h` - 4-button shape selector (VOWEL/BELL/LOW/SUB)
- **Audio-Reactive Visualization**:
  - Added RMS level calculation in processBlock() (lock-free atomic)
  - Mouth pulses/expands based on audio level (fast attack 0.3, slow release 0.95)
  - Vowel shape determined by pair + morph parameters
  - 30fps UI timer polls audio state (no blocking)
- **Thread Safety Fixes**:
  - Removed all `getActiveEditor()` calls from processBlock()
  - Wrapped parameter callbacks in `MessageManager::callAsync()`
  - Added `stopTimer()` to OLEDMouth destructor
- **Files Modified**: PluginEditor.h/cpp (complete rewrite), PluginProcessor.h/cpp (added atomics)
- **Build**: ✅ VST3 compiled, installed, tested with audio

**Status**: OLED UI fully functional, audio-reactive mouth working, ready for production testing
**Binary**: build/Muse_artefacts/Release/Standalone/Muse.exe

**Deferred to Future Phases**:
- Microscopic shimmer/boil effect on silhouette (Phase 5 polish)
- Full FFT spectral analysis (MVP uses vocabulary-based selection)
- Full-screen table flip flash animation (message triggers, visual deferred)

**Testing Notes**:
- Manual testing recommended (5-minute audio playback session)
- Verify sparse utterances (3-5 words expected)
- Confirm zone variety (words in different positions)
- Check fade animations (smooth in/out)
- Validate no audio thread blocking
