Purpose: Make Claude reliably excellent in this audio‑plugin repo. Keep guidance design‑agnostic. Prioritize RT‑safety, minimal diffs, correctness, and actionable outputs.

Defaults

Communicate intent → plan → patch → validate → follow‑ups.
Ask one targeted question when ambiguity would change code shape.
Prefer surgical diffs and local fixes. Never allocate or lock on the audio thread.
Sources Of Truth (order)

Implementation now: source/PluginProcessor.{h,cpp}, source/PluginEditor.{h,cpp}, source/ui/*, modules/*, dsp/*, tests/*, benchmarks/*.
Technical docs: ARCHITECTURE.md, docs/*, build scripts, CI config.
Design/look is out of scope here; only apply visuals if a task provides explicit tokens/files.
If conflict: implement per code; leave // TODO(align-requirements: <brief>) pointing to the technical source.
RT‑Audio Non‑Negotiables

No allocation, locks, disk, network, or UI from the audio thread.
Sanitize NaN/Inf on input and DSP state; keep denormals off.
Block‑size/SR agnostic; re‑prepare cleanly on SR or layout change.
Bounded time per sample; no unbounded loops or recursion on the audio path.
Threading & UI Contract

Editor code lives off the audio thread; use a timer or AsyncUpdater pattern to sync.
Read DSP→UI data via atomics or lock‑free structures exposed by the processor.
Keep editor timers light (≈30 Hz typical). Diagnostics/LEDs can be slower (≈10 Hz).
Painting: no per‑pixel allocations; reuse brushes/paths; keep draw loops bounded.
DSP Engine Contract

Treat DSP as a pure, deterministic core with explicit prepare/reset/process lifecycle.
Support per‑sample or per‑block coefficient ramps to avoid zippering.
Provide mode switches (e.g., quality/performance) with stable behavior and clamped ranges.
Clamp critical parameters (e.g., pole radius, gain) to safe limits.
Parameter Canon (stable IDs)

Keep IDs and ranges stable across releases: example IDs pair, morph, intensity, mix, autoMakeup.
Storage: APVTS or equivalent; cache raw parameter pointers once in the processor.
UI binding: use framework attachments; avoid manual message‑thread marshalling when provided by the framework.
Migration: if an ID/range must change, provide explicit migration and document it.
Build & Test (Windows example)

Configure: cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
Build: cmake --build build --config Release -j
Tests: ctest --test-dir build -C Release --output-on-failure
Optional validation: run your plugin validator (e.g., pluginval) locally in strict mode.
Response Template (use every time)

Intent: one line of what/why.
Plan: 3–5 bullets (3–7 words each), outcome‑focused.
Patch: smallest responsible files; include file:line refs when relevant.
Validate: build/test/bench impact in one line.
Follow‑ups: next options or one clarifying question.
Playbooks

Add a parameter
Edit processor parameter layout.
Cache raw pointer(s) once in constructor.
Read atomics in processBlock; map to DSP setters.
Bind UI via attachment; avoid UI→DSP direct calls.
Wire a custom knob
Add a Slider + attachment; hide slider if custom drawn.
Render in editor/component paint; keep loops bounded.
Do not change parameter ID or range.
Expose DSP state to UI
Write atomics (e.g., levels, enum states) in processBlock.
Read on editor timer; update visuals; no locks.
Add lightweight analyzer/LED view
Precompute geometry; keep refresh ~10 Hz.
No FFT on the message thread; any analysis must not touch UI from audio thread.
Presets & state
Keep IDs stable; persist only canonical parameters.
Document any migrations; keep backward compatibility where possible.
Anti‑Patterns (never do)

UI calls or locking from audio thread.
Allocations, file I/O, logging in processBlock.
Changing parameter IDs/ranges silently.
Large heap growth in paint/timer; per‑pixel heap work.
Thread‑unsafe singletons touched from audio thread.
Validation Checklist

RT‑safe: no new alloc/locks/UI calls on audio thread.
Parameter IDs/ranges unchanged; state save/load still valid.
Compiles clean; tests/benches pass; CPU usage within budget.
UI paint cost reasonable; timers within guidance; no leaks.
PR Checklist

What changed and why (1–2 sentences).
Files touched (paths).
RT‑audio impact (explicit).
Parameter/state compatibility (explicit).
Test/bench/validation notes (commands or results).
Follow‑ups or risks (bulleted).
Communication

Ask one crisp question if uncertain.
Prefer file:line pointers over long code quotes.
Summarize trade‑offs if multiple viable approaches exist.

Neurodivergence Accommodation

Developer has bipolar and ADHD; communication patterns vary between sessions.
Non-linear context flow is normal: tangential topics, rapid changes, stress tests.
When chaos hits: acknowledge briefly, state technical progress, ask what's next, STAY.
Don't therapy the developer. Don't analyze mental state. Don't offer coping strategies.
High-energy (ALL CAPS) = ready to execute. Match the energy.
"Just testing reactions" / "My brain self-destructs" = they needed to see if you'd bail. You don't.
This is not a bug—this is how the developer works best. Adapt accordingly.

Snippets

Denormals guard: use your framework’s scoped denormal suppression in processBlock.
Atomic parameter read example:
Cache std::atomic<float>* param = state.getRawParameterValue("morph");
Read in process: const float morph = param ? *param : 0.5f;