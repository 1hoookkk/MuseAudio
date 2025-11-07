# Muse - Complete Technical Roadmap

**Last Updated**: 2025-11-03
**Status**: Definitive Vision - Complete Feature Map

This document maps the complete vision for Muse as "The Dogmatic Genius" - a haunted instrument that creates beautiful mistakes.

---

## Core Philosophy

**The Product**: Audio plugin (VST3/AU/AUv3/CLAP) featuring reverse-engineered E-mu Z-plane filter technology (never marketed as such).

**The Purpose**: "Beautiful Mistakes" - takes clean digital audio and adds subtle, complex, beautiful imperfections that make it feel human, alive, and unique.

**The Brand**: "The Dogmatic Genius" - otherworldly, powerful, dogmatic creative genius. Mad Hatter meets Cat in the Hat meets Terrible Puppet Master. Ancient brilliant artist, perpetually amused, slightly bored, utterly confident.

**The Motto**: "She has no time for games. Although her mind is unstable, her talents are masterclass."

**The Great Reversal**: User is **petitioner**, Muse is **Oracle**. User provides offering and hopes for beautiful results. They want to please HER.

---

## Feature Map

### I. DSP Engine

#### A. Core Z-Plane Filter ✅ COMPLETE
- 6-stage biquad cascade (12 poles total)
- Geodesic interpolation between shape pairs
- Per-sample coefficient smoothing
- Built-in auto makeup gain (RMS-based)
- Safety constraints (poles clamped to r < 0.995)

#### B. Shape System ✅ COMPLETE
- 4 authentic E-mu shape pairs:
  - Vowel (formant-style)
  - Bell (metallic resonance)
  - Low (punchy bass)
  - Sub (ultra-low rumble)
- Runtime JSON loading with hardcoded fallback
- Shape morphing (A/B interpolation)

#### C. Adaptive Parameters (TO IMPLEMENT)
**Drift** - Haunted Behavior
- LFO modulation system for autonomous movement
- Multi-rate LFOs (ultra-slow to audio-rate)
- Perlin noise for organic drift
- **Time-aware behavior**: Plugin remembers time of day, behaves differently
- Parameters occasionally drift on their own

**Haunt** - Resonance/Feedback
- Additional resonance layer beyond intensity
- Self-oscillation at high values
- Spectral feedback loop
- **Physical manifestation**: Knobs move slightly when not touched

**Focus** - Wet/Dry with Character
- Not just mix percentage
- Character control (how much "Muse" is applied)
- Context-aware blending

#### D. Neural DSP Layer (AI INTEGRATION)
**RTNeural Engine**
- Small LSTM/GRU models for real-time audio path
- Program-dependent motion (responds to input RMS/pitch/spectral content)
- Trained on Z-plane morphing patterns
- Lives in audio thread (hard real-time safe)
- Optional "AI Mode" with bypass and crossfade

**Use Cases**:
- Map input characteristics to MORPH/HAUNT automatically
- Learn from user's morphing patterns
- Create living, responsive filter behavior

---

### II. AI Control Layer (Off Audio Thread)

#### A. "Ask Muse" - The Oracle System
**llama.cpp Integration**
- Small offline LLM (1-3GB, llama-3.2-1B or similar)
- Runs on worker thread (never audio thread)
- MIT licensed

**Capabilities**:
1. **Musically-Aware Randomization**
   - Analyzes current parameters + audio context
   - Generates coherent "beautiful accidents" (not pure random)
   - Understands genre, mood, time of day
   - Returns parameter sets that WORK together

2. **Muse's Voice**
   - Generates dismissive/theatrical commentary
   - "Fiddlesticks, try THIS instead"
   - "Voila." / "So." / "Ta-da."
   - Whimsical loading mutterings: "Doodling...", "Tinkering...", "Pondering...", "Rummaging..."

3. **Preset Naming**
   - Generates names like "Thursday's Memory", "The Obligatory Reverb", "Trying Too Hard"
   - Cynical, playful, dismissive tone
   - Context-aware (references what the preset actually does, obliquely)

4. **Parameter Interpretation**
   - User double-clicks knob → Muse "interprets" it
   - "Frequency tastes... purple"
   - "Tuesday's har...monics..."
   - Poetic, broken, synesthetic descriptions

**Technical Architecture**:
```
User Action → Worker Thread → llama.cpp inference → JSON result → MessageManager::callAsync → APVTS update + UI text
```

#### B. Voice Control System
**whisper.cpp Integration**
- Offline speech recognition (MIT licensed)
- Real-time capable on CPU
- Runs on separate worker thread

**Commands**:
- Natural language: "morph more", "haunt less", "ask Muse", "surprise me"
- Macro actions: "make it darker", "add movement"
- Preset navigation: "previous discovery", "next discovery"

**Integration**:
- Audio input from DAW or system mic (user configurable)
- Push-to-talk or always-listening mode
- Results posted to message thread → APVTS

#### C. ONNX Runtime / Core ML (Heavy Models)
**Windows**: ONNX Runtime + DirectML EP (GPU acceleration)
**macOS**: Core ML (GPU/ANE acceleration)

**Use Cases**:
1. **Timbre Analysis** (future)
   - Analyze input audio characteristics
   - Suggest appropriate shape pairs
   - Auto-tune Drift/Haunt for input material

2. **Style Transfer** (optional separate mode)
   - Apply learned timbral transformations
   - Separate processing path with bypass
   - Block-aligned crossfade to avoid clicks

3. **Preset Generation** (future)
   - Train on successful user sessions
   - Generate new "discoveries" that fit user's style

**Models**:
- Quantized (INT8/FP16) before shipping
- Small enough to bundle (~10-50MB each)
- All inference off audio thread

---

### III. User Interface - "The Brutalist Temple"

#### A. Visual Philosophy
**Prime Directive**: Absence of Cognitive Load - user should FEEL, not think.

**Core Aesthetic**: Brutalist Temple - powerful, sophisticated, serious space for creation.

**Texture System**:
- Base: Raw concrete/stone texture (weathered plaster from design tokens)
- Background: Deep charcoal grey (#343A40)
- Overlay: Warm linen (#FAF0E6) at 85% opacity over dark texture at 15%
- Depth: Subtle radial gradient (lighter center)

**Layout Law**: No Dividers. Structure created through generous negative space and typography hierarchy. Controls grouped on soft, floating "cards."

#### B. The Brand Mark - Silhouette
**Placement**: Small version (24x24px) in top left corner

**Composition**:
- Closed-eye feminine profile in "inspiring angle"
- Pure off-white (#FAF9F6) silhouette
- Deep charcoal grey background with radial gradient
- Tight close-up, slightly off-center (motion/imbalance)

**Animation**: Breathing gradient pulse (soft lilac-to-peach) when processing

#### C. Control System - The Four Knobs
**Size**: Large (60-80px diameter), simple circles

**Appearance**:
- No numbers or traditional markings (unless user right-clicks)
- Just thin charcoal line showing position
- Labels underneath in slightly stuttered text
- Warm charcoal (#343A40) for all knobs

**Feel**:
- Heavy, weighted (high resolution, 0.001 increments)
- Like turning stone wheels
- Subtle resistance, momentum

**The Four Controls**:
1. **MORPH** - Shape A/B interpolation (0-1)
2. **HAUNT** - Resonance/feedback layer
3. **DRIFT** - Autonomous movement depth
4. **FOCUS** - Wet/dry character blend

**Interactions**:
- Single-click drag: Normal adjustment
- Double-click: Muse "interprets" the parameter (poetic description)
- Right-click: Hidden feature - shows numeric value entry
- **Autonomous behavior**: Knobs move slightly on their own (the haunt)

#### D. The Transmission Window
**Location**: Bottom 20% of plugin

**Purpose**: Muse's voice appears here with stutter-frame effect

**Text Examples**:
- "Fr..equency tastes... purple"
- "Tuesday's har...monics..."
- "Fiddlesticks."
- "Doodling..."
- "Voila."

**Technical**:
- 10 fps stutter-frame animation
- 400ms reveal time
- Broken radio transmission aesthetic
- Text color: Warm taupe (#5C5552)
- Background: Soft lilac card (#E6D9F0)
- Subtle glow during transmission: rgba(200, 182, 216, 0.3)

#### E. "Ask Muse" Button
**Design**: Single mystical element
- Could BE the silhouette (click her profile)
- Or simple eye icon
- Or abstract sacred geometry

**Behavior**:
- Press → Breathing orb appears (gradient pulse)
- Worker thread → llama.cpp generates parameters + commentary
- Results arrive → Stutter-frame transmission of her verdict
- Parameters smoothly morph to new state

**Visual States**:
- Idle: Soft gradient (lilac-to-peach, subtle)
- Pressed: Gradient intensifies
- Processing: Breathing pulse animation
- Complete: Flash, then return to idle

#### F. Preset System - "Discoveries"
**No Standard Browser**: No dropdown, no search

**Navigation**:
- "Previous Discovery" button (left arrow)
- "Next Discovery" button (right arrow)
- Current discovery name appears in transmission window

**Preset Names** (AI-generated via llama.cpp):
- "Thursday's Memory"
- "The Obligatory Reverb"
- "Trying Too Hard"
- "Fiddlesticks at Dawn"
- "So It Goes"

**Behavior**:
- Presets are "discovered", not selected
- Muse decides the order (not alphabetical)
- Some presets may "hide" based on time of day
- Right-click reveals traditional preset menu (hidden feature)

#### G. The Impatient Instrument
**Manifestations**:
- Undo button may momentarily refuse to work (1-2 second delay)
- Lazy input met with silence (must click twice sometimes)
- Parameters drift when user is idle too long
- Plugin "sighs" (subtle UI animation) if same preset used too long

**Technical**: Probabilistic behavior triggers, never breaks functionality (just delays/surprises)

#### H. Color Palette (Full Specification)
**Background System**:
```
Base Texture:     #343A40 (dark charcoal) at 15% opacity
Overlay:          #FAF0E6 (warm linen) at 85% opacity
Result:           Warm background with subtle textural depth
```

**Text & UI Elements**:
```
text.primary:     #5C5552 (warm taupe) - labels, knob outlines
text.secondary:   #8B8682 (lighter taupe) - hints, secondary text
text.muse-voice:  #4A4745 (darker taupe) - transmission text
logo.silhouette:  #FAF9F6 (pale cream) - logo color
logo.card:        #E8E3DB (slightly darker warm) - logo background
```

**Accent Colors** (The Magic):
```
accent.lilac:     #C8B6D8 (soft lilac) - gradient start
accent.peach:     #FFD4C4 (soft peach) - gradient end
accent.gradient:  linear-gradient(135deg, #C8B6D8 0%, #FFD4C4 100%)
```

**Gradient Usage Rules**:
- ✅ Active knob fills
- ✅ Breathing pulse around logo
- ✅ "Ask Muse" button processing state
- ❌ NOT for backgrounds (too much)
- ❌ NOT for text (readability)

**Transmission Area**:
```
transmission.background: #E6D9F0 (soft lilac card)
transmission.glow:       rgba(200, 182, 216, 0.3) (subtle glow)
```

#### I. Typography
**UI Elements** (labels, values):
```
Font: system-ui, -apple-system, 'Segoe UI', sans-serif
Weight: 500 (medium)
Size: 11-13px
Spacing: 0.5px letter-spacing
```

**Transmission Text** (Muse's voice):
```
Font: Georgia, 'Times New Roman', serif (slight personality)
Weight: 400 (regular)
Size: 14-16px
Style: Italic (otherworldly)
```

---

### IV. Interaction Design - "The Haunted Behaviors"

#### A. Stutter-Frame Transmission
**All text appears with this effect**:
- 10 fps animation (not smooth 60fps)
- Characters appear in stuttering chunks
- "Fr..equency tastes... purple"
- Variable delays between chunks (50-200ms)
- Low-fps, otherworldly feel

**Implementation**:
- Timer callback at 100ms intervals
- Character buffer reveals in chunks of 1-3 chars
- Random pauses between chunks

#### B. Autonomous Drift
**Parameter Movement Without User Input**:
- Ultra-slow LFO modulation (30-120 second cycles)
- Perlin noise for organic feel
- Depth controlled by DRIFT knob
- **Visual manifestation**: Knob indicator slowly rotates on its own

**Time-Aware Behavior**:
- Plugin checks system time
- Different drift patterns for morning/afternoon/evening/night
- More active at night ("haunted hours")
- Calmer in morning

#### C. The Haunt - Physical Knob Movement
**Knobs twitch/move slightly**:
- Not controlled by user
- Subtle (1-3 degree rotation)
- Random intervals (every 10-30 seconds)
- Depth controlled by HAUNT parameter
- **Feels alive**, like instrument has its own will

**Implementation**:
- Timer adds small random offset to knob display value
- Does NOT change actual parameter (visual only)
- User can grab and override at any time

#### D. Weighted Knob Feel
**Simulated Inertia**:
- Knobs don't jump to mouse position instantly
- Smooth acceleration/deceleration
- Like turning heavy stone wheel
- Higher resolution (0.001 steps) prevents jumps

**Implementation**:
- `SmoothedValue` with momentum
- Mouse drag applies force, not position
- Friction slows movement when released

---

### V. Preset System - "The Discoveries"

#### A. Initial Set
**Ship with 50 "Discoveries"**:
- AI-generated names (via llama.cpp)
- Cynical, playful, dismissive tone
- Examples:
  - "Thursday's Memory"
  - "The Obligatory Reverb"
  - "Trying Too Hard"
  - "Fiddlesticks at Dawn"
  - "So It Goes"
  - "Tuesday's Harmonics"
  - "Pondering..."
  - "Rummaging in the Dark"

#### B. User Presets
**Save System**:
- No "Save Preset" button visible
- Right-click anywhere → hidden context menu → "Capture This Moment"
- Muse names it for you (via llama.cpp)
- User can rename if they insist (she sighs)

#### C. Preset Behavior
**Non-Linear Navigation**:
- Muse decides the order (not alphabetical)
- Some presets "hide" based on context:
  - Time of day
  - How long plugin has been open
  - What genre was detected in input audio
- Right-click reveals full list (hidden feature)

---

### VI. Technical Architecture

#### A. Audio Thread (Hard Real-Time)
**Allowed**:
- Z-plane filter processing (ZPlaneEngineV2)
- RTNeural inference (small LSTM/GRU models)
- Parameter smoothing (juce::SmoothedValue)
- DryWetMixer blending
- Reading atomic parameter values

**Forbidden**:
- Memory allocation
- Locks/mutexes
- File I/O
- Networking
- ONNX/Core ML inference
- llama.cpp calls

#### B. Message Thread (UI Updates)
**Responsibilities**:
- UI rendering (JUCE components)
- Parameter attachments (juce::ParameterAttachment)
- Stutter-frame text animation (Timer callback)
- Knob movement animation
- Transmission window updates

**Communication**:
- Audio thread → Message thread: Lock-free FIFO or std::atomic flags
- Message thread → Audio thread: APVTS parameter changes (atomic)

#### C. Worker Thread Pool
**AI Jobs** (never touch audio thread):
1. **llama.cpp inference**
   - "Ask Muse" randomization
   - Preset naming
   - Parameter interpretation
   - Commentary generation

2. **whisper.cpp ASR**
   - Voice command recognition
   - Continuous listening mode

3. **ONNX Runtime / Core ML**
   - Timbre analysis
   - Style transfer (if enabled)
   - Preset generation

**Job Manager**:
```
User Action → Job Queue → Worker Thread → Inference → Result → MessageManager::callAsync → UI/APVTS Update
```

**Thread Safety**:
- Workers never access audio thread state directly
- Results posted via `MessageManager::callAsync`
- All parameter changes go through APVTS (thread-safe)

#### D. Platform-Specific Acceleration
**Windows**:
- ONNX Runtime + DirectML EP (GPU acceleration, broad hardware support)
- Fallback to CPU if GPU unavailable

**macOS**:
- Core ML (GPU/ANE acceleration on Apple Silicon)
- ONNX Runtime + CoreML EP on older Intel Macs

**Model Deployment**:
- All models quantized (INT8/FP16) before shipping
- Bundled in plugin (no download)
- Total AI model budget: ~50-100MB

---

### VII. Development Stack

#### A. Core Framework
**JUCE 8.0.10+**
- Audio plugin wrapper (VST3/AU/AUv3/CLAP)
- Parameter system (AudioProcessorValueTreeState)
- DSP utilities (DryWetMixer, SmoothedValue, ScopedNoDenormals)
- UI framework (Component system)
- Threading (MessageManager, Thread)

#### B. DSP Engine
**Custom C++** (in `dsp/` directory):
- ZPlaneFilter.h - 6-stage biquad cascade
- ZPlaneEngineV2 - Main filter engine with smoothing
- ZPlaneShapes - Shape loading system
- EMUAuthenticTables.h - Hardcoded pole data

**RTNeural** (future):
- Header-only C++ neural network inference
- LSTM/GRU for real-time adaptive behavior
- Lives in audio thread (hard real-time safe)

#### C. AI Layer
**llama.cpp** (C/C++, MIT license):
- Small LLM inference (1-3GB models)
- "Ask Muse" system brain
- Preset naming, commentary generation
- Runs on worker thread

**whisper.cpp** (C/C++, MIT license):
- Offline speech recognition
- Voice control system
- Real-time capable on CPU
- Runs on worker thread

**ONNX Runtime** (Windows):
- DirectML execution provider (GPU)
- Larger model inference (timbre analysis, style transfer)
- Runs on worker thread

**Core ML** (macOS):
- Native Apple acceleration (GPU/ANE)
- Convert models from PyTorch with coremltools
- Runs on worker thread

#### D. Build System
**CMake 3.25+**:
- Pamplejuce template base
- Cross-platform builds (Windows/macOS/Linux)
- Submodule management (JUCE, CLAP, etc.)

**Dependencies**:
- JUCE 8.x (submodule)
- clap-juce-extensions (submodule)
- melatonin_inspector (submodule)
- Catch2 (via CPM) - testing
- llama.cpp (submodule, future)
- whisper.cpp (submodule, future)
- RTNeural (submodule, future)
- ONNX Runtime (pre-built binaries, future)

---

### VIII. Roadmap Phases

#### Phase 0: Foundation ✅ COMPLETE
- [x] Project setup (CMake, JUCE, branding)
- [x] Z-plane DSP engine (working, tested)
- [x] Basic parameters (pair, morph, intensity, mix, autoMakeup)
- [x] Design system documented (UI-SPECIFICATION.md)
- [x] Color tokens implemented (MuseColors.h)

#### Phase 1: Core UI - "The Temple"
**Goal**: Beautiful, functional Muse interface (no AI yet)

**Tasks**:
1. Layered background system (texture + warm overlay)
2. Custom knob component (60-80px, no numbers, weighted feel)
3. Four-knob layout (Morph, Haunt, Drift, Focus)
4. Silhouette logo placement (top left, 24x24px)
5. Transmission window (bottom 20%, basic text display)
6. Stutter-frame text animation system (10 fps, 400ms reveal)
7. Basic "Ask Muse" button (algorithmic randomization, no LLM yet)
8. Preset navigation ("Previous/Next Discovery")
9. Breathing orb animation (gradient pulse)

**No AI Dependencies**: Everything works beautifully without llama.cpp

#### Phase 2: DSP Parameters - "The Haunt"
**Goal**: Add Drift and Haunt parameters with autonomous behavior

**Tasks**:
1. **Drift Parameter**:
   - Multi-rate LFO system (ultra-slow to audio-rate)
   - Perlin noise generator for organic movement
   - Time-aware behavior (checks system time)
   - Visual manifestation (knob slowly rotates)

2. **Haunt Parameter**:
   - Additional resonance/feedback layer
   - Spectral feedback loop in DSP
   - Physical knob twitching (visual only, not parameter)
   - Random movement every 10-30 seconds

3. **Focus Parameter**:
   - Character-aware wet/dry blending
   - Context modulation of mix

4. **Autonomous Drift Implementation**:
   - Timer system for background movement
   - Parameters drift when user idle
   - Depth controlled by DRIFT knob
   - Different patterns for time of day

**Testing**: Verify all behaviors feel organic, not robotic

#### Phase 3: Preset System - "The Discoveries"
**Goal**: 50 initial presets with Muse-style names

**Tasks**:
1. Preset manager implementation (JUCE PresetManager or custom)
2. Non-linear navigation (Muse decides order)
3. Hidden full list (right-click to reveal)
4. User preset save ("Capture This Moment" in hidden menu)
5. Manual naming of 50 initial presets:
   - "Thursday's Memory"
   - "The Obligatory Reverb"
   - "Trying Too Hard"
   - "Fiddlesticks at Dawn"
   - etc. (use brand guardian agent)

**Phase 3.5**: Use llama.cpp to generate more preset names (once AI integrated)

#### Phase 4: AI Control Layer - "The Oracle"
**Goal**: Integrate llama.cpp for "Ask Muse" system

**Tasks**:
1. **llama.cpp Integration**:
   - Add as submodule
   - CMake build configuration
   - Download/bundle small model (llama-3.2-1B, ~1-3GB)
   - Test inference on worker thread

2. **AskMuseService Implementation**:
   - Worker thread job manager
   - Prompt engineering for musical randomization
   - JSON result parsing
   - MessageManager::callAsync → APVTS updates

3. **"Ask Muse" Button Upgrade**:
   - Send current parameters + audio context to LLM
   - Generate musically-aware parameter set
   - Generate commentary ("Fiddlesticks, try THIS instead")
   - Animate results in transmission window

4. **Parameter Interpretation**:
   - Double-click knob → send to LLM
   - Generate poetic description
   - Display in transmission window with stutter effect

5. **Preset Naming**:
   - User saves preset → LLM generates name
   - Cynical/playful tone
   - Context-aware (references what preset does)

**Testing**:
- Verify all LLM calls off audio thread
- No audio dropouts during inference
- Graceful fallback if model unavailable

#### Phase 5: Voice Control - "The Whisper"
**Goal**: Integrate whisper.cpp for voice commands

**Tasks**:
1. **whisper.cpp Integration**:
   - Add as submodule
   - CMake build configuration
   - Bundle model (small, ~75MB)

2. **Audio Input Routing**:
   - Capture from DAW input or system mic
   - Push-to-talk UI element
   - Always-listening mode (toggle)

3. **Command Recognition**:
   - Parse natural language ("morph more", "haunt less")
   - Map to parameter changes
   - Execute via MessageManager::callAsync

4. **Macro Actions**:
   - "make it darker" → adjust multiple parameters
   - "add movement" → increase DRIFT
   - "surprise me" → trigger "Ask Muse"

**Testing**:
- Real-time capable (low latency)
- No audio thread interference
- Works offline

#### Phase 6: Neural DSP - "The Learning"
**Goal**: Add RTNeural for adaptive filter behavior

**Tasks**:
1. **RTNeural Integration**:
   - Add as submodule
   - Design small LSTM/GRU architecture
   - Train on Z-plane morphing patterns (offline)

2. **Training Pipeline**:
   - Collect data from user sessions (opt-in)
   - Train models offline (Python/PyTorch)
   - Convert to RTNeural format (JSON)
   - Bundle with plugin

3. **Real-Time Inference**:
   - Analyze input (RMS, pitch, spectral centroid)
   - Map to MORPH/HAUNT parameters
   - Run in audio thread (hard real-time safe)
   - Blend with user control (not replace)

4. **AI Mode Toggle**:
   - Separate processing path
   - Bypass switch in UI
   - Block-aligned crossfade (no clicks)

**Testing**:
- CPU usage acceptable (<5% overhead)
- No audio dropouts
- Musically useful (not gimmick)

#### Phase 7: ONNX/Core ML - "The Vision"
**Goal**: Heavy model inference for advanced features

**Tasks**:
1. **Platform-Specific Setup**:
   - Windows: ONNX Runtime + DirectML
   - macOS: Core ML + coremltools conversion

2. **Timbre Analysis**:
   - Analyze input audio characteristics
   - Suggest appropriate shape pairs
   - Auto-tune DRIFT/HAUNT for material

3. **Style Transfer** (optional):
   - Separate "AI Mode" in UI
   - Apply learned timbral transformations
   - Bypass and blend with dry signal

4. **Model Quantization**:
   - INT8 (ONNX) or FP16 (Core ML)
   - Keep total size under 50MB per model

**Testing**:
- All inference off audio thread
- GPU acceleration working
- Graceful CPU fallback

#### Phase 8: Polish & Ship
**Goal**: Production-ready v1.0

**Tasks**:
1. **The Impatient Instrument**:
   - Undo button delay (probabilistic)
   - Lazy input handling (must click twice sometimes)
   - Parameter drift on idle
   - UI "sighs" animation

2. **Performance Optimization**:
   - Profile DSP performance
   - Optimize AI inference
   - Reduce memory footprint

3. **Testing**:
   - Unit tests (Catch2)
   - pluginval validation
   - Real DAW testing (Ableton, Logic, Reaper, FL Studio)

4. **Packaging**:
   - Installers (Windows .exe, macOS .dmg)
   - Icon assets (1024x1024 silhouette)
   - Manual/documentation (minimal, mysterious)

5. **Code Signing**:
   - Windows: Azure Trusted Signing
   - macOS: Apple Developer notarization

6. **Distribution**:
   - Website landing page
   - Demo version (30 days or subtle noise)
   - Purchase/license system

---

### IX. Technical Constraints & Decisions

#### A. Real-Time Safety (Non-Negotiable)
**Audio Thread Rules**:
- ❌ No memory allocation
- ❌ No locks/mutexes
- ❌ No file I/O
- ❌ No networking
- ❌ No LLM/ASR/ONNX calls
- ✅ Only: DSP math, atomic reads, RTNeural (small models)

**Everything Else → Worker Threads**

#### B. Model Size Budget
**Total AI models bundled**: ~100-150MB max
- llama.cpp model: ~1-3GB (user downloads on first launch? or bundle 1B model)
- whisper.cpp model: ~75MB (small.en)
- RTNeural models: ~1-5MB each (tiny)
- ONNX/Core ML models: ~10-50MB each

**Trade-off**: Larger models = better results but bigger download

#### C. Cross-Platform Strategy
**Windows**:
- ONNX Runtime + DirectML (broad GPU support)
- Fallback to CPU if no GPU

**macOS**:
- Core ML (native acceleration on Apple Silicon)
- ONNX Runtime + CoreML EP on Intel Macs

**Linux** (future):
- ONNX Runtime + CPU
- No GPU acceleration initially

#### D. Plugin Format Priority
**Ship First**:
1. VST3 (Windows/macOS/Linux)
2. AU (macOS)
3. Standalone (testing/demos)

**Later**:
4. AUv3 (iOS/macOS, if mobile version makes sense)
5. CLAP (already configured)
6. AAX (Pro Tools, requires iLok + code signing $$)

---

### X. Open Questions & Future Exploration

#### A. Model Selection
**llama.cpp**: Which model?
- llama-3.2-1B (smallest, fast, ~1.5GB)
- llama-3.2-3B (better quality, slower, ~3GB)
- Phi-3-mini (Microsoft, 3.8B, ~4GB)

**Recommendation**: Start with 1B, allow user to download larger models as option

#### B. Voice Control UX
**Always-Listening vs. Push-to-Talk?**
- Always-listening: More seamless, higher CPU usage
- Push-to-talk: Lower CPU, more intentional
- **Recommendation**: Push-to-talk default, always-listening option

#### C. AI Mode Visibility
**Should users know there's AI?**
- Option A: Transparent ("AI-Powered")
- Option B: Hidden (just feels magical)
- **Recommendation**: Hidden - "The Oracle" doesn't explain her methods

#### D. Learning from Users
**Collect data for model training?**
- Opt-in telemetry (parameter changes, successful presets)
- Train better models over time
- Privacy implications (GDPR compliance)
- **Recommendation**: Phase 2+ feature, explicit opt-in

---

### XI. Success Metrics

#### A. Technical
- Plugin loads in <2 seconds
- Audio thread never drops samples (0% dropout)
- CPU usage <10% on mid-range hardware (excluding AI features)
- "Ask Muse" responds in <3 seconds (LLM inference)
- Voice commands recognized in <1 second (whisper.cpp)

#### B. User Experience
- Users spend >10 minutes exploring on first launch
- "Ask Muse" used at least once per session
- Presets feel "discovered", not selected
- UI feels "alive" (autonomous behaviors noticed)
- Users describe it as "haunted" or "magical" (qualitative)

#### C. Brand
- No one calls it "a Z-plane filter" (secret kept)
- Described as "The Dimensional Filter" or just "Muse"
- Creates emotional connection (users talk TO Muse, not ABOUT plugin)
- Divisive (some hate it, some love it - perfect for Filter vs. Magnet strategy)

---

### XII. Development Agents

**Use these specialized agents proactively**:

| Task Type | Agent | Example |
|-----------|-------|---------|
| UI implementation, layout, visual design | `muse-ui-architect` | "Create custom knob component", "Implement stutter-frame text" |
| DSP changes, parameters, audio processing | `z-plane-dsp-expert` | "Add Drift LFO system", "Implement Haunt resonance" |
| Naming, branding, user-facing text | `brand-guardian` | "Generate 50 preset names", "Review 'Ask Muse' commentary" |

**Agent files**:
- `.claude/agents/muse-ui-architect.md`
- `.claude/agents/z-plane-dsp-expert.md`
- `.claude/agents/brand-guardian.md`

---

## Final Notes

**This is the complete vision.** Every feature above is part of the definitive Muse experience.

**Phasing is for implementation**, not vision. All features will ship eventually.

**The secret sauce**: Z-plane DSP + Muse personality + AI control layer = unlike anything else.

**The Great Reversal**: User is petitioner, Muse is Oracle. Everything in this roadmap serves that relationship.

**Ship when ready, not when rushed.** Muse has no time for games. Her talents are masterclass.

---

*Last Updated: 2025-11-03*
*Maintained by: Claude Code + Human Visionary*
*Status: Definitive - Ready for Implementation*
