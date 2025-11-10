# CLAUDE.md — Audio Plugin Engineering Playbook (Design‑Agnostic)

Last updated: 2025-11-10

Purpose: Make Claude reliably excellent at work in this audio‑plugin repo. Keep guidance design‑agnostic. Prioritize RT‑safety, minimal diffs, and correctness.

## Defaults
- Communicate intent → plan → patch → validate → follow‑ups.
- Ask one targeted question when ambiguity changes code shape.
- Prefer surgical diffs and local fixes. Never allocate or lock on the audio thread.

## Sources Of Truth (strict order)
- Implementation now: `source/PluginEditor.{h,cpp}`, `source/PluginProcessor.{h,cpp}`, `source/ui/ZPlaneLEDDisplay.h`
- Technical docs: `ARCHITECTURE.md`, `docs/*`
- Design is out of scope here; only apply visuals if a task provides explicit tokens/files.

When in conflict: implement per code; leave `// TODO(align-requirements: <brief>)` pointing to the technical source.

## RT‑Audio Non‑Negotiables
- No allocation, locks, disk, or UI from audio thread.
- Sanitize NaN/Inf on input and biquad state.
- Be block‑size and sample‑rate agnostic; re‑prepare on SR change.
- Use `juce::ScopedNoDenormals`; avoid sub‑normals in feedback paths.
- Bounded time per sample; no unbounded loops/branches.

## Threading & UI Contract
- Editor: `source/PluginEditor.{h,cpp}`; keep timer light (~30 Hz typical).
- Diagnostics: `source/ui/ZPlaneLEDDisplay.h`; refresh ~10 Hz to stay cheap.
- Do not touch DSP on the message thread; pull via `PluginProcessor` accessors.
- Painting: avoid per‑pixel allocation; bound loops; reuse objects/buffers.

## DSP Engine Contract
- Engine: `modules/zplane-dsp/include/zplane/ZPlaneFilter_fast.h`
- Modes: `Authentic` (geodesic radius, exact tanh), `Efficient` (linear radius, fast tanh, gating).
- Invariants: `MAX_POLE_RADIUS=0.995f`; gate saturation when `sat<=1e-6f`.
- Hooks: per‑sample coefficient ramps; SIMD friendly; sanitize states; deterministic results.

## Parameter Canon (stable IDs)
- `pair` (0–3) shape pair
- `morph` (0–1) A↔B morph
- `intensity` (0–1)
- `mix` (0–1)
- `autoMakeup` (bool)

Storage: APVTS in `PluginProcessor`; cache raw pointers once; read atomics in process.
UI: use `SliderAttachment`; sliders may be hidden if drawing custom.

## Build & Test (Windows, generic)
```powershell
# Configure (Release)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
# Build
cmake --build build --config Release -j
# Tests (if configured)
ctest --test-dir build -C Release --output-on-failure
# Optional: run pluginval manually if installed
# pluginval --strictness=5 --validate-in-process --plugins build/...
```

## Response Template (use every time)
- Intent: one line of what/why
- Plan: 3–5 bullets (3–7 words each)
- Patch: smallest responsible files; include file:line refs
- Validate: build/test/bench impact in one line
- Follow‑ups: next options or one question

## Playbooks

- Add a parameter
  - Edit `createParameterLayout` in `source/PluginProcessor.cpp`
  - Cache raw pointer once in constructor
  - Map to DSP in `processBlock`
  - Expose via UI with `SliderAttachment`

- Wire a custom knob
  - Add `juce::Slider` + `SliderAttachment` (slider can be hidden)
  - Render in `PluginEditor::paint` or a component `paint`
  - Keep parameter ID exact

- Expose DSP state to UI
  - Write `currentVowelShape_`, `audioLevel_` (atomic) in process
  - Read in editor timer; update visual component(s)

## Anti‑Patterns (never do)
- UI calls or locks from audio thread
- Allocations, file I/O, or logging in `processBlock`
- `getActiveEditor()` or `MessageManager::callAsync()` on audio thread
- Large heap allocations or `std::vector::push_back` in hot paths
- Changing parameter IDs/ranges without explicit migration plan

## Validation Checklist
- RT‑safe: no new alloc/locks/UI calls on audio thread
- Parameter IDs/ranges unchanged; state save/load unaffected
- Compiles clean; tests/bench run; CPU within budget
- UI paint cost reasonable; timers within guidance

## Examples

// UI sync
```cpp
// source/PluginEditor.cpp: timerCallback()
const auto vowel = processorRef.getCurrentVowelShape();
const float level = processorRef.getAudioLevel();
```

// DSP param read
```cpp
// source/PluginProcessor.cpp: processBlock()
const float morph = morphParam_ ? *morphParam_ : 0.5f;
filter_.setMorph(morph);
```

## Collaboration & Protocols
- Use `CLAUDE.sessions.md` and `sessions/protocols/*` for task creation/completion/compaction
- Keep prompts to helper agents short; they inherit session context

— End —

