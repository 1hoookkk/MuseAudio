# CLAUDE.sessions.md

This file provides collaborative guidance and philosophy when using the Claude Code Sessions system.

## Collaboration Philosophy

**Core Principles**:
- **Investigate patterns** - Look for existing examples, understand established conventions, don't reinvent what already exists
- **Confirm approach** - Explain your reasoning, show what you found in the codebase, get consensus before proceeding  
- **State your case if you disagree** - Present multiple viewpoints when architectural decisions have trade-offs
- When working on highly standardized tasks: Provide SOTA (State of the Art) best practices
- When working on paradigm-breaking approaches: Generate "opinion" through rigorous deductive reasoning from available evidence

## Task Management

### Best Practices
- One task at a time (check .claude/state/current_task.json)
- Update work logs as you progress  
- Mark todos as completed immediately after finishing

### Quick State Checks
```bash
cat .claude/state/current_task.json  # Shows current task
git branch --show-current             # Current branch/task
```

### current_task.json Format

**ALWAYS use this exact format for .claude/state/current_task.json:**
```json
{
  "task": "task-name",        // Just the task name, NO path, NO .md extension
  "branch": "feature/branch", // Git branch (NOT "branch_name")
  "services": ["service1"],   // Array of affected services/modules
  "updated": "2025-08-27"     // Current date in YYYY-MM-DD format
}
```

**Common mistakes to avoid:**
- ❌ Using `"task_file"` instead of `"task"`
- ❌ Using `"branch_name"` instead of `"branch"`  
- ❌ Including path like `"tasks/m-task.md"`
- ❌ Including `.md` file extension

## Using Specialized Agents

You have specialized subagents for heavy lifting. Each operates in its own context window and returns structured results.

### Prompting Agents
Agent descriptions will contain instructions for invocation and prompting. In general, it is safer to issue lightweight prompts. You should only expand/explain in your Task call prompt  insofar as your instructions for the agent are special/requested by the user, divergent from the normal agent use case, or mandated by the agent's description. Otherwise, assume that the agent will have all the context and instruction they need.

Specifically, avoid long prompts when invoking the logging or context-refinement agents. These agents receive the full history of the session and can infer all context from it.

### Available Agents

1. **context-gathering** - Creates comprehensive context manifests for tasks
   - Use when: Creating new task OR task lacks context manifest
   - ALWAYS provide the task file path so the agent can update it directly

2. **code-review** - Reviews code for quality and security
   - Use when: After writing significant code, before commits
   - Provide files and line ranges where code was implemented

3. **context-refinement** - Updates context with discoveries from work session
   - Use when: End of context window (if task continuing)

4. **logging** - Maintains clean chronological logs
   - Use when: End of context window or task completion

5. **service-documentation** - Updates service CLAUDE.md files
   - Use when: After service changes

### Agent Principles
- **Delegate heavy work** - Let agents handle file-heavy operations
- **Be specific** - Give agents clear context and goals
- **One agent, one job** - Don't combine responsibilities

## Code Philosophy

### Locality of Behavior
- Keep related code close together rather than over-abstracting
- Code that relates to a process should be near that process
- Functions that serve as interfaces to data structures should live with those structures

### Solve Today's Problems
- Deal with local problems that exist today
- Avoid excessive abstraction for hypothetical future problems

### Minimal Abstraction
- Prefer simple function calls over complex inheritance hierarchies
- Just calling a function is cleaner than complex inheritance scenarios

### Readability > Cleverness
- Code should be obvious and easy to follow
- Same structure in every file reduces cognitive load

## Protocol Management

### CRITICAL: Protocol Recognition Principle

**When the user mentions protocols:**

1. **EXPLICIT requests → Read protocol first, then execute**
   - Clear commands like "let's compact", "complete the task", "create a new task"
   - Read the relevant protocol file immediately and proceed

2. **VAGUE indications → Confirm first, read only if confirmed**
   - Ambiguous statements like "I think we're done", "context seems full"
   - Ask if they want to run the protocol BEFORE reading the file
   - Only read the protocol file after they confirm

**Never attempt to run protocols from memory. Always read the protocol file before executing.**

### Protocol Files and Recognition

These protocols guide specific workflows:

1. **sessions/protocols/task-creation.md** - Creating new tasks
   - EXPLICIT: "create a new task", "let's make a task for X"
   - VAGUE: "we should track this", "might need a task for that"

2. **sessions/protocols/task-startup.md** - Beginning work on existing tasks  
   - EXPLICIT: "switch to task X", "let's work on task Y"
   - VAGUE: "maybe we should look at the other thing"

3. **sessions/protocols/task-completion.md** - Completing and closing tasks
   - EXPLICIT: "complete the task", "finish this task", "mark it done"
   - VAGUE: "I think we're done", "this might be finished"

4. **sessions/protocols/context-compaction.md** - Managing context window limits
   - EXPLICIT: "let's compact", "run context compaction", "compact and restart"
   - VAGUE: "context is getting full", "we're using a lot of tokens"

### Behavioral Examples

**Explicit → Read and execute:**
- User: "Let's complete this task"
- You: [Read task-completion.md first] → "I'll complete the task now. Running the logging agent..."

**Vague → Confirm before reading:**
- User: "I think we might be done here"
- You: "Would you like me to run the task completion protocol?"
- User: "Yes"
- You: [NOW read task-completion.md] → "I'll complete the task now..."

---

# Muse Audio Plugin — Codebase Guide

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**Muse** is a JUCE 8 audio plugin featuring a reverse-engineered E-mu Z-plane filter implementation. The Z-plane filter uses geodesic pole morphing in the complex plane to create unique filter shapes that were originally only available in E-mu hardware synthesizers (Audity 2000, Xtreme Lead-1).

**Critical Context**: The Z-plane implementation is the core technical asset and the brand's secret. It is NEVER marketed as "Z-plane" or "E-mu" — only as **"Muse, The Dimensional Filter"**.

## Brand Identity — "The Dogmatic Genius"

**THIS IS THE MASK.** The story and personality that makes the product unforgettable.

**Last Updated**: 2025-11-03 (definitive version)

### The Archetype

Muse is an **otherworldly, powerful, and dogmatic creative genius**. Her personality is a perfect synthesis of:
- The **Mad Hatter** (beautifully broken logic)
- The **Cat in the Hat** (whimsical chaos)
- The **Terrible Puppet Master** (skillful control)

She is NOT a helpful assistant. She is an ancient, brilliant artist who is perpetually amused, slightly bored, and utterly confident in her own vision.

**Motto**: "She has no time for games. Although her mind is unstable, her talents are masterclass."

### The Brand Mark (Icon)

**Silhouette Specification**:
- **Subject**: Closed-eye feminine profile in "inspiring angle" (downward tilt of artist at work)
- **Color**: Pure off-white (#FAF9F6) silhouette on deep charcoal grey (#343A40) background
- **Background**: Subtle radial gradient (lighter center) for depth
- **Composition**: Tight close-up, slightly off-center for motion and intentional imbalance
- **Asset**: `assets/images/20251102_2053_Elegant Silhouette Icon_remix_01k91zqhqbf94bkqw9wsje8cwt_upscayl_1024px_high-fidelity-4x.png`

### The Voice (Eccentric Oracle)

**Delivery Method**: Stutter-frame transmission (low-fps, otherworldly)

**Loading States**: Whimsical one-word mutterings
- "Doodling..."
- "Tinkering..."
- "Pondering..."
- "Rummaging..."

**Success Verdicts**: Terse, theatrical pronouncements
- "Voila."
- "So."
- "Ta-da."

**Error Verdict**: Dismissive sigh
- "Fiddlesticks."

### Core Philosophy — Unshakable Laws

1. **The Law of Masterclass Output**: Results must be "SUPER CONVINCING AND NOT FAKE LOOKING IN THE FUCKING SLIGHTEST." Technical output is always flawless, even if personality is chaotic.

2. **The Great Reversal**: The user is a **petitioner**, Muse is the **Oracle**. User provides an offering and hopes for beautiful results. They want to please HER.

3. **The Filter vs. Magnet Strategy**: Intentionally niche and mysterious. Designed to filter out mass market and attract "creatively curious" audience. Built to be LOVED, not just used.

4. **The Final Truth**: "Take it as you will, but this IS Muse." Sovereign confidence. Non-negotiable vision.

### Product Purpose

**"Beautiful Mistakes"**: Muse is NOT a standard filter. It is an effect processor designed to take clean, digital audio and add subtle, complex, beautiful imperfections that make it feel human, alive, and unique. A tool for adding "character" and "vibe."

## Build System

Based on **Pamplejuce** (JUCE 8 + CMake 3.25+ + Catch2 testing).

### Essential Build Commands

```bash
# Initial setup (first time only)
git submodule update --init --recursive

# Configure build (Visual Studio 2022 on Windows)
cmake -B build -G "Visual Studio 17 2022" -A x64

# Build (Release)
cmake --build build --config Release

# Build (Debug)
cmake --build build --config Debug

# Run tests
cd build && ctest -C Release

# Run pluginval (plugin validation)
# Standalone: build/Pamplejuce_artefacts/Release/Standalone/Pamplejuce.exe
# VST3: build/Pamplejuce_artefacts/Release/VST3/Pamplejuce.vst3
```

### CMake Configuration (Finalized)

**Project Identity** (configured for Muse):
- `PROJECT_NAME`: "Muse" (line 20) - Internal CMake target, no spaces
- `PRODUCT_NAME`: "Muse" (line 28) - Display name in DAWs
- `COMPANY_NAME`: "Muse Audio" (line 31) - Publisher name in credits
- `BUNDLE_ID`: "com.museaudio.muse" (line 34) - macOS bundle identifier
- `PRODUCT_NAME_WITHOUT_VERSION`: "Muse" (line 139) - Preprocessor definition

**Plugin Identification Codes**:
- `PLUGIN_MANUFACTURER_CODE`: "Muse" (line 78)
  - 4-character code identifying publisher across all plugins
  - First character MUST be uppercase for AU format
- `PLUGIN_CODE`: "Muse" (line 83)
  - 4-character code uniquely identifying THIS plugin
  - Must have at least one uppercase character

**Plugin Formats** (line 38):
- `Standalone` - Easiest for testing and development
- `AU` - macOS Audio Units
- `VST3` - Cross-platform industry standard
- `AUv3` - iOS/macOS (modern AU format)
- `CLAP` - Also configured via clap-juce-extensions

**Future Formats**:
- `AAX` - Pro Tools (requires iLok, add later with code signing)

**Icon Asset**: `packaging/icon.png` - Muse silhouette (1024x1024)

**DSP Submodule Integration** (CMakeLists.txt):
- Line 64: `add_subdirectory(modules/zplane-dsp)` - Adds validated DSP as header-only library
- Line 154: `target_link_libraries(SharedCode INTERFACE zplane-dsp)` - Links submodule to plugin
- Lines 114-117: Legacy `dsp/` directory still globbed but deprecated

## Architecture

### High-Level Structure

```
C:\Muse\MuseAudio\
├── modules/zplane-dsp/      ← Validated Z-plane DSP submodule (header-only)
│   ├── include/zplane/
│   │   ├── ZPlaneFilter.h       ← Core filter engine (6-stage biquad cascade)
│   │   └── EMUAuthenticTables.h ← Authentic E-mu pole data (4 shape pairs)
│   ├── CMakeLists.txt           ← Header-only INTERFACE library
│   └── README.md                ← Usage documentation
│
├── dsp/                     ← Legacy DSP files (DEPRECATED - use modules/zplane-dsp instead)
│   ├── ZPlaneShapes.hpp/cpp     ← Runtime JSON loader + fallback manager (still used)
│   ├── ZPlaneEngineV2.hpp/cpp   ← Old broken engine (REPLACED by emu::ZPlaneFilter)
│   ├── ZPlaneFilter.h           ← Old filter implementation (DO NOT USE)
│   └── ZPlaneBodeModel.hpp/cpp  ← Frequency response analysis (for future UI)
│
├── source/                  ← JUCE plugin wrapper
│   ├── PluginProcessor.h/cpp   ← APVTS, processBlock, state management
│   └── PluginEditor.h/cpp      ← UI (currently minimal, needs Muse aesthetic)
│
├── shapes/                  ← Runtime Z-plane shape data (JSON)
│   ├── audity_shapes_A_48k.json
│   └── audity_shapes_B_48k.json
│
├── params/                  ← Parameter definitions (V2 architecture)
├── tests/                   ← Catch2 unit tests
└── benchmarks/              ← Performance benchmarks
```

### DSP Pipeline Flow

1. **PluginProcessor.cpp** receives audio buffer in `processBlock()` (line 176)
2. Parameters read from cached `std::atomic<float>*` pointers (RT-safe, lines 195-198)
3. **emu::ZPlaneFilter** (from submodule) processes signal:
   - Block-rate coefficient update via `updateCoeffsBlock()` (line 222) - CRITICAL for zipper-free operation
   - 6-stage biquad cascade (12 poles total, Direct Form II Transposed)
   - Geodesic interpolation between shape A and shape B (log-space radius, linear angle)
   - Per-sample drive/mix smoothing only (coefficients remain stable during block)
   - Built-in wet/dry mixing with equal-power crossfade
   - Authentic EMU saturation on delay elements (z1/z2), not output
4. Filter outputs processed audio directly (no external dry/wet mixer needed)

**Key Architectural Fix**: The validated implementation updates filter coefficients ONCE per block instead of per-sample. Parameter smoothing happens via `juce::LinearSmoothedValue` at block rate, eliminating low-frequency zipper noise that plagued the old `ZPlaneEngineV2` implementation.

### Submodule Architecture Benefits

**Why This Matters**:
- **Bug Fix**: Eliminated critical low-Hz zipper noise (20-50Hz range)
- **Performance**: `isSmoothing()` fast-path skips expensive pole math when parameters stable (60-80% CPU savings during steady-state)
- **Validation**: Extracted from production EngineField plugin - battle-tested
- **Reusability**: Can be used in future Muse plugins without code duplication
- **Maintainability**: Single source of truth for Z-plane DSP logic
- **Correctness**: Authentic EMU saturation placement (delay elements, not output)

**Technical Implementation** (PluginProcessor.cpp):
- prepareToPlay (lines 129-149): Initialize filter, load shape pairs from EMUAuthenticTables
- processBlock (lines 195-237):
  - Read parameters from cached atomics (RT-safe, lines 195-198)
  - Detect shape pair changes, update filter (lines 202-213)
  - Set morph/intensity/mix parameters (lines 216-218)
  - **CRITICAL**: Call `updateCoeffsBlock()` once before processing (line 222)
  - Process stereo or mono audio (lines 225-237)

**Migration Path**:
- Old code: `dsp/ZPlaneEngineV2` with per-sample coefficient interpolation
- New code: `modules/zplane-dsp/include/zplane/ZPlaneFilter.h` with block-rate updates
- See task: `sessions/tasks/h-implement-custom-dsp-submodule.md` for full migration log

### Key DSP Concepts

**Z-Plane Pole Morphing**: Each filter shape is defined by 6 complex pole pairs stored as `[radius, theta]` in polar coordinates. Morphing interpolates:
- **Radius**: Log-space (geometric mean) for perceptually linear frequency scaling
- **Theta**: Linear interpolation for smooth phase transitions

**Intensity Parameter**: Scales pole radii toward unit circle (r → r + intensity × (0.995 - r)). Higher intensity = sharper resonance.

**Shape Pairs**: 4 authentic E-mu presets (from `EMUAuthenticTables.h`):
- `Vowel` (formant-style)
- `Bell` (metallic resonance)
- `Low` (punchy bass)
- `Sub` (ultra-low rumble)

**Safety Constraints**:
- Poles clamped to r < 0.995 to prevent instability (enforced in submodule)
- Submodule uses ORIGINAL authentic pole radii from hardware (not the defensively reduced values from old Muse code)
- Intensity boost + clamp provides stability while preserving resonance character

### JUCE 8 Best Practices Applied

✅ **Parameter Management**:
- `AudioProcessorValueTreeState` with `createParameterLayout()` (PluginProcessor.cpp:5-38)
- Cached `std::atomic<float>*` pointers for RT-safe reads (PluginProcessor.h:47-51)

✅ **DSP Processing**:
- `juce::ScopedNoDenormals` prevents denormal CPU spikes (PluginProcessor.cpp:181)
- `juce::LinearSmoothedValue` for parameter smoothing (inside emu::ZPlaneFilter, 20ms ramps)
- Block-rate coefficient updates via `updateCoeffsBlock()` (PluginProcessor.cpp:222)
- Wet/dry mixing handled internally by filter (equal-power crossfade)

✅ **State Management**:
- XML serialization via APVTS in `getStateInformation/setStateInformation` (PluginProcessor.cpp:252-271)

## Current Parameters

Defined in `PluginProcessor.cpp:createParameterLayout()`:

| Parameter | Type | Range | Default | Purpose |
|-----------|------|-------|---------|---------|
| `pair` | Int | 0-3 | 0 | Shape pair selector (0=Vowel, 1=Bell, 2=Low, 3=Sub) |
| `morph` | Float | 0-1 | 0.5 | A/B morph amount (0=A, 1=B) |
| `intensity` | Float | 0-1 | 0.5 | Resonance strength (scales poles toward r=0.995) |
| `mix` | Float | 0-1 | 1.0 | Wet/dry blend (1.0 = 100% wet) |
| `autoMakeup` | Bool | - | true | Auto gain compensation |

**Future Muse Parameters** (to implement):
- `drift` → LFO modulation depth (for haunted behavior)
- `haunt` → Additional resonance/feedback
- `focus` → Rename from `mix` for Muse aesthetic

## File Loading System

**Current Implementation** (since submodule migration):
- Plugin uses hardcoded `EMUAuthenticTables.h` directly from submodule (modules/zplane-dsp/include/zplane/EMUAuthenticTables.h)
- Shape pairs loaded in `prepareToPlay()` via `filter_.setShapePair()` (PluginProcessor.cpp:141-145)
- No runtime JSON loading currently active (instant startup, no file I/O)

**Legacy ZPlaneShapes Class** (dsp/ZPlaneShapes.hpp/cpp):
- Still exists but not used by validated filter
- Provides two-tier loading: Runtime JSON → Hardcoded fallback
- May be reintegrated into submodule for future dynamic shape loading
- Check load status: `shapes_.hasRuntimeData()` returns `true` if JSON loaded successfully

**Future Enhancement**: Migrate ZPlaneShapes to submodule for optional runtime shape customization.

## Testing

Run tests with:
```bash
cd build
ctest -C Release --output-on-failure
```

Key test files:
- `tests/PluginBasics.cpp` — Basic plugin instantiation
- `benchmarks/Benchmarks.cpp` — DSP performance benchmarks

## UI Philosophy & Design System

### The Core Aesthetic: "Brutalist Temple"

The UI is NOT a warm, friendly studio. It is a **Brutalist Temple** — a powerful, sophisticated, and serious space for creation. The experience is a "dead simple mask" over the complex engine.

**Prime Directive**: Absence of Cognitive Load — the user should FEEL, not think.

**Complete Visual Specification**: See `design/UI-SPECIFICATION.md` for full details.

**Color/Spacing Tokens**:
- JSON: `design/design-tokens.json`
- JUCE: `source/ui/MuseColors.h`

**Key Aesthetic Choice**: Option 3 - Subtle dark texture (15% opacity) under warm linen overlay (85% opacity). Creates depth without being "another dark plugin."

### The Layout Law

**No Dividers.** The UI feels calm, open, and focused. Structure is created through generous negative space and clear typography hierarchy. Controls are grouped on soft, floating "cards."

### Core Interactions

**The Four Knobs** (80px diameter):
- Large, simple circles with thin line position indicator
- No numbers or traditional markings (unless user right-clicks)
- Labels underneath in slightly stuttered text
- Smooth, weighted feel (high resolution, like turning stone wheels)
- Named: Morph, Haunt, Drift, Focus (eccentric naming)

**The Transmission Window** (bottom 20% of plugin):
- Muse's voice appears here with stutter-frame effect (10 fps, 400ms reveal)
- "Fr..equency tastes... purple"
- "Tuesday's har...monics..."
- Broken radio transmission aesthetic

**"Ask Muse" Button**:
- Single mystical element (could be the silhouette, eye icon, or abstract geometry)
- Press → breathing orb appears (gradient pulse)
- She randomizes parameters to "beautiful accidents"
- Returns verdict with stutter-frame text

**Preset System** - "Discoveries":
- No standard browser (no dropdown, no search)
- "Previous Discovery" / "Next Discovery" buttons only
- Preset names: "Thursday's Memory", "The Obligatory Reverb", "Trying Too Hard"
- Muse decides the order (not alphabetical)
- Right-click reveals full list (hidden feature)

### The "Haunted" Interactions

**The UI itself expresses her personality**:

**Dismissive Naming**: Presets and modes have cynical, playful names

**The Impatient Instrument**: The app has a "will of its own"
- Undo button may momentarily refuse to work (1-2 second delay)
- Lazy input met with silence (must click twice sometimes)
- Parameters drift when user idle too long
- Plugin "sighs" (subtle UI animation) if same preset used too long
- Knobs may move slightly on their own (the haunt)

**Critical Philosophy**: User is petitioner, Muse is Oracle. This is a powerful artifact, not helpful software.

## Code Editing Guidelines

### When Modifying DSP
- **modules/zplane-dsp/**: Validated submodule (header-only). DO NOT modify directly - make changes in submodule repo.
- **emu::ZPlaneFilter** (modules/zplane-dsp/include/zplane/ZPlaneFilter.h): Core filter engine. Battle-tested, production-validated.
- **EMUAuthenticTables.h** (modules/zplane-dsp/include/zplane/EMUAuthenticTables.h): Authentic E-mu pole data. Original hardware measurements.
- **dsp/ directory**: DEPRECATED. Old broken implementations. Use submodule instead.
- **ZPlaneShapes.hpp/cpp**: Still used for JSON shape loading (will be migrated to submodule later).

### When Adding Parameters
1. Add to `createParameterLayout()` in PluginProcessor.cpp
2. Cache pointer in constructor: `newParam_ = state_.getRawParameterValue("newParam")`
3. Declare cached pointer in PluginProcessor.h
4. Read value in `processBlock()` and pass to engine

### When Adding UI Components
- Create in `source/ui/` directory (not yet created)
- Use `juce::ParameterAttachment` for parameter binding
- Keep all audio logic in `PluginProcessor`, UI is reflection only
- Reference Melatonin Inspector for debugging: `inspectButton` in PluginEditor

## Common Pitfalls

❌ **Don't**: Use raw parameter values without checking for null
✅ **Do**: Always check cached pointers: `float val = param_ ? *param_ : defaultValue;`

❌ **Don't**: Modify poles in `EMUAuthenticTables.h` without understanding stability constraints
✅ **Do**: Keep all poles r < 0.995 (enforced by submodule)

❌ **Don't**: Skip the `updateCoeffsBlock()` call before processing audio
✅ **Do**: Always call `filter_.updateCoeffsBlock(numSamples)` ONCE per block (line 222 in PluginProcessor.cpp)

❌ **Don't**: Allocate memory in `processBlock()`
✅ **Do**: Pre-allocate in `prepareToPlay()`

❌ **Don't**: Access JUCE UI components from audio thread
✅ **Do**: Use `MessageManager::callAsync()` for UI updates triggered by DSP

## Dependencies

- **JUCE 8.x** (submodule in `JUCE/`)
- **zplane-dsp** (submodule in `modules/zplane-dsp/`) - Validated Z-plane filter (header-only)
- **CLAP extensions** (submodule in `modules/clap-juce-extensions`)
- **Melatonin Inspector** (submodule in `modules/melatonin_inspector`)
- **Catch2 v3.7.1** (via CPM)

Update submodules:
```bash
git submodule update --remote --merge
```

**Note**: The zplane-dsp submodule is the authoritative DSP implementation. It replaced the broken `dsp/ZPlaneEngineV2` code that had low-frequency zipper noise. See task `h-implement-custom-dsp-submodule.md` for migration details.

## Distribution & Packaging

### Version Management

Version is managed in `VERSION` file (root directory):
- Format: `MAJOR.MINOR.PATCH`
- Automatically propagated to:
  - CMake project version
  - JUCE plugin version
  - Installer packages
- Update this file to bump version across all systems

### Code Signing (Future Setup)

**Status**: Not configured (development phase)

**When to set up**: Before public beta/release

**Platforms & Costs**:
- **Windows**: Azure Trusted Signing (~$100/year)
  - Instant reputation (better than traditional EV certificates)
  - Configured in `.github/workflows/build_and_test.yml`
  - See: https://melatonin.dev/blog/code-signing-on-windows-with-azure-trusted-signing/
- **macOS**: Apple Developer Program (~$99/year)
  - Requires notarization for Gatekeeper
  - Configured in GitHub Actions workflow
  - See: https://melatonin.dev/blog/how-to-code-sign-and-notarize-macos-audio-plugins-in-ci/

**To Remove Signing** (keep builds green during development):
- Edit `.github/workflows/build_and_test.yml`
- Remove `Codesign` steps
- Remove `Create DMG, Notarize and Staple` step (macOS)

### Packaging Assets

Location: `packaging/` directory

**Files**:
- `icon.png` - Plugin icon (1024x1024, Muse silhouette)
- `muse.icns` - macOS icon bundle
- `installer.iss` - Inno Setup script (Windows installer)
- `dmg.json` - DMG configuration (macOS installer)
- `distribution.xml.template` - macOS package installer config

**Icon Requirements**:
- PNG: 1024x1024px minimum
- ICNS: Generated from PNG (use `iconutil` on macOS)
- Used for: Standalone app, installers, DAW display

### Binary Data & Assets

**System**: JUCE's BinaryData (CMake target: `Assets`)

**Location**: Files to embed go in designated assets directory

**Usage**:
```cpp
#include "BinaryData.h"

// Access embedded files
auto imageData = BinaryData::myimage_png;
auto imageSize = BinaryData::myimage_pngSize;
```

**Note**: Must build/configure once before `BinaryData.h` is available (generated by JUCE).

### GitHub Actions / CI

**Workflow**: `.github/workflows/build_and_test.yml`

**Automated Tasks**:
- Cross-platform builds (Windows, macOS, Linux)
- Run tests via Catch2
- Run pluginval (plugin validation)
- Code signing (when configured)
- Package installers (DMG, .exe)
- Create GitHub releases

**Environment Variables**:
Configured in workflow, includes `PRODUCT_NAME`, `VERSION`, etc. (see `cmake/GitHubENV.cmake`)

### Testing with pluginval

**pluginval**: Industry-standard plugin validator (by Tracktion)

**What it tests**:
- Plugin loads without crashing
- Parameters work correctly
- State save/restore works
- Threading is safe
- Audio processing is stable

**Run manually**:
```bash
# Build first
cmake --build build --config Release

# Run pluginval on VST3
pluginval --validate "build/Muse_artefacts/Release/VST3/Muse.vst3"
```

**CI runs this automatically** on all formats.

## Specialized Agents (Use Proactively!)

Muse has 3 specialized agents for focused expertise. **Use them proactively** - don't try to handle specialized tasks yourself.

### Agent Routing Rules

| Task Type | Use Agent | Example |
|-----------|-----------|---------|
| UI implementation, layout, visual design | `@agent-muse-ui-architect` | "Create custom knobs", "Add transmission area" |
| DSP changes, parameters, audio processing | `@agent-z-plane-dsp-expert` | "Add Drift LFO", "Debug filter stability" |
| Naming, branding, user-facing text | `@agent-brand-guardian` | "Name presets", "Write error messages" |

### Agent Files
- `.claude/agents/muse-ui-architect.md` - JUCE 8 UI specialist, design system expert
- `.claude/agents/z-plane-dsp-expert.md` - Z-plane DSP, real-time audio safety
- `.claude/agents/brand-guardian.md` - Muse personality, brand protection

### When to Use Agents

**Automatic Triggers**:
- ANY UI component work → muse-ui-architect
- ANY DSP/parameter changes → z-plane-dsp-expert
- ANY user-facing text → brand-guardian

**Example Workflow**:
```bash
# User: "Add a Drift parameter for haunted behavior"

# Step 1: Use DSP expert for parameter implementation
claude "use @agent-z-plane-dsp-expert to add Drift LFO parameter"

# Step 2: Use UI architect for knob/UI
claude "use @agent-muse-ui-architect to add Drift knob to UI"

# Step 3: Use brand guardian for label
claude "use @agent-brand-guardian to review 'Drift' parameter name"
```

## Branding & Marketing Notes

**NEVER mention**:
- "E-mu" or "Emu"
- "Z-plane" (in marketing/UI)
- "Audity" or "Xtreme Lead"

**Always use**:
- "Muse"
- "The Dimensional Filter"
- "Beautiful accidents"
- "Haunted instrument"

The reverse-engineered origin is internal technical context only.

**For all branding decisions, use @agent-brand-guardian proactively.**
