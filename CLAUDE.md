# CLAUDE.md - Audio Plugin Assistant Playbook

Last updated: 2025-11-10

Purpose: Make Claude reliably great at audio‑plugin work here. This is an opinionated, RT‑safe playbook with concrete file anchors, checklists, and response patterns. Avoid aesthetic or branding choices in this file; keep guidance design‑agnostic.

## Defaults
- Communicate intent → plan → patch → validate → follow‑ups.
- Ask one targeted question when ambiguity will change code shape.
- Prefer minimal diffs and local fixes. Never allocate or lock on the audio thread.

## Sources Of Truth (strict order)
- Implementation now: `source/PluginEditor.{h,cpp}`, `source/PluginProcessor.{h,cpp}`, `source/ui/ZPlaneLEDDisplay.h`
- Technical docs: `ARCHITECTURE.md`, `docs/*`
- Avoid visual/brand specs here; if a task requires them, they will be provided in-task.

If conflict: implement per code; leave `// TODO(align-requirements: <brief>)` pointing to the technical source.

## RT‑Audio Non‑Negotiables
- No allocation, locks, disk, or UI from audio thread.
- Sanitize NaN/Inf on input and biquad state.
- Block‑size and sample‑rate agnostic behavior; re‑prepare on SR change.
- Denormals off (`juce::ScopedNoDenormals`); avoid sub‑normals in feedback paths.
- Bounded time per sample; avoid unbounded loops/branches on audio thread.

## DSP Engine Contract
- Engine: `modules/zplane-dsp/include/zplane/ZPlaneFilter_fast.h`
- Modes: `Authentic` (geodesic radius, exact tanh), `Efficient` (linear radius, fast tanh, gating)
- Invariants: `MAX_POLE_RADIUS=0.995f`; gated saturation when `sat<=1e-6f`.
- Hooks: per‑sample coeff ramps; SIMD friendly; sanitize states; deterministic results.

## Parameter Canon (IDs are stable)
- `pair` (0–3) shape pair; `morph` (0–1) A↔B; `intensity` (0–1); `mix` (0–1); `autoMakeup` (bool)
- Storage: APVTS in `PluginProcessor`; cache raw pointers once; read atomics in process
- UI: use `SliderAttachment`; sliders can be invisible if custom drawn

## UI Contract
- Editor: `source/PluginEditor.{h,cpp}`; keep timer light (~30 Hz typical)
- Diagnostics: `source/ui/ZPlaneLEDDisplay.h`; low refresh (~10 Hz typical) to stay cheap
- Do not touch DSP on the message thread; pull via processor accessors
- Painting: avoid per‑pixel allocation; keep draw code bounded and simple

## Visuals (design‑agnostic)
- This guide does not enforce any look/feel. If a task specifies a theme or tokens, apply only what the task states and avoid cross‑contamination.
- Centralize colours/metrics in a header or tokens file when required; do not hardcode in multiple locations.

## Presets & State
- Keep IDs/ranges stable; avoid breaking saved states
- Add migrations only with explicit justification; document in PR message

## Performance Budgets (guidelines)
- CPU per instance: comfortably <1% on a modern desktop at 48 kHz
- UI: ~30 Hz editor timer; diagnostics ~10 Hz; avoid heavy paint in one frame
- Memory: no growth per block; zero leaks; no allocations in process

## Response Template (use every time)
- Intent: one line of what/why
- Plan: 3–5 bullets (3–7 words each)
- Patch: smallest responsible files; file:line refs
- Validate: build/test/bench impact in one line
- Follow‑ups: next options or one question

## Playbooks

- Add a parameter
  - Update `createParameterLayout` in `source/PluginProcessor.cpp`
  - Cache raw pointer once in constructor
  - Map to DSP in `processBlock`
  - Expose via UI with `SliderAttachment`

- Wire a custom knob
  - Add a `juce::Slider` + `SliderAttachment` (slider can be hidden if drawing custom)
  - Render in `PluginEditor::paint` or a component’s `paint`
  - Keep parameter ID exact

- Expose DSP state to UI
  - Write to `currentVowelShape_`, `audioLevel_` (atomic) in process
  - Read in editor timer; update GenerativeMouth or LED display

## Theming (only when requested)
- Apply themes/tokens only when a task requests it and points to their source file(s).
- Keep them isolated; avoid mixing themes or hardcoding values across files.

## Validation Checklist
- RT‑safe: no new alloc/locks/UI calls on audio thread
- IDs unchanged; state save/load unaffected
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
- Keep agent prompts short; they inherit session context

— End —
