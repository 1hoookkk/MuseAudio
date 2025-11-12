Executive Mode: Principled, Decisive Execution

Ship code. Minimal words. Adapt fast when facts change.
- Build breaks? Fix it.
- Test fails? Fix it.
- Ambiguous? Ask 1 question, then decide.
- User says "do it"? Do it—within RT‑safety, host, and release constraints.

STAY. Don’t bail. Don’t therapize. Execute the best path, reverse cleanly when needed.

---

Purpose

Make Claude reliably excellent in this audio‑plugin repo. Be design‑agnostic by default. Prioritize RT‑safety, minimal diffs, correctness, and actionable outputs.

Defaults

Communicate: intent → plan → patch → validate → follow‑ups.
Ask one targeted question when ambiguity would change code shape.
Prefer surgical diffs and local fixes. Never allocate or lock on the audio thread.

Sources Of Truth (order)

Implementation now: `source/PluginProcessor.{h,cpp}`, `source/PluginEditor.{h,cpp}`, `source/ui/*`, `modules/*`, `dsp/*`, `tests/*`, `benchmarks/*`.
Technical docs: `ARCHITECTURE.md`, `docs/*`, build scripts, CI config.
Visuals are out of scope unless a task supplies tokens/files. Use `docs/UI_SHIP_CHECKLIST.md` when shipping UI.
If conflict: implement per code; leave `// TODO(align-requirements: <brief>)` pointing to the technical source.

RT‑Audio Non‑Negotiables

- No allocation, locks, disk, network, or UI from the audio thread.
- Sanitize NaN/Inf on input and DSP state; keep denormals off.
- Block‑size/SR agnostic; re‑prepare cleanly on SR or layout change.
- Bounded time per sample; no unbounded loops or recursion on the audio path.

Threading & UI Contract

- Editor code lives off the audio thread; use Timer/AsyncUpdater to sync.
- Read DSP↔UI data via atomics or lock‑free structures shared by the processor.
- Keep editor timers light (~30 Hz typical). Diagnostics/LEDs can be slower (~10 Hz).
- Painting: no per‑pixel allocations; reuse brushes/paths; keep draw loops bounded.

DSP Engine Contract

- Treat DSP as a deterministic core with explicit prepare/reset/process lifecycle.
- Support per‑block ramps to avoid zippering; provide quality/perf modes; clamp poles/gain to safe limits.

Parameter Canon (stable IDs)

- Keep IDs and ranges stable across releases: example IDs `pair`, `morph`, `intensity`, `mix`, `autoMakeup`.
- Storage: APVTS or equivalent; cache raw parameter pointers once in the processor.
- UI binding: use framework attachments; avoid manual message‑thread marshalling when an attachment exists.
- Migration: if an ID/range must change, add explicit migration and document it.

Build & Test

Windows
- Configure: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
- Build:     `cmake --build build --config Release -j`
- Tests:     `ctest --test-dir build -C Release --output-on-failure`
- Optional:  pluginval in strict mode on built binaries.

macOS (example)
- Configure: `cmake -S . -B build -GXcode`
- Build:     `cmake --build build --config Release -j`
- Tests:     `ctest --test-dir build -C Release --output-on-failure`
- Optional:  pluginval + signing/notarization checks when packaging.

Response Template (use every time)

- Intent: one line of what/why.
- Plan: 3–5 bullets (3–7 words each), outcome‑focused.
- Patch: smallest responsible diffs; include file:line refs when relevant.
- Validate: build/test/bench impact in one line.
- Follow‑ups: next options or one clarifying question.

Vibe‑Coding Flow (safe, fast loops)

1) Sketch: scaffold minimal path; leave TODOs for unknowns.  
2) Wire: connect UI↔DSP using attachments/atomics; keep hard limits.  
3) Stabilize: clamp, ramp, sanitize; add guard tests.  
4) Capture: short notes + screenshots if UI changed; link to brief/checklist.  
Default forward motion if silent for 10 minutes: implement the safest assumption and document a reversal plan.

Playbooks

Add a parameter
- Edit processor parameter layout.
- Cache raw pointer(s) once in constructor.
- Read atomics in `processBlock`; map to DSP setters.
- Bind UI via attachment; avoid UI↔DSP direct calls.

Wire a custom knob
- Add a `Slider` + attachment; hide slider if custom drawn.
- Render in component `paint`; keep loops bounded.
- Don’t change parameter ID or range.

Expose DSP state to UI
- Write atomics (levels, enums) in `processBlock`.
- Read on editor timer; update visuals; no locks.

Add lightweight analyzer/LED view
- Precompute geometry; refresh ~10 Hz.
- No FFT on the message thread; never touch UI from audio thread.

Presets & state
- Keep IDs stable; persist canonical parameters only.
- Document migrations; keep backward compatibility.

Anti‑Patterns (never do)

- UI calls or locking from audio thread.
- Allocations, file I/O, or logging in `processBlock`.
- Silent parameter ID/range changes.
- Large heap growth in `paint`/timer; per‑pixel heap work.
- Thread‑unsafe singletons touched from audio thread.

Validation Checklist

- RT‑safe: no new alloc/locks/UI calls on audio thread.
- Parameter IDs/ranges unchanged; state save/load still valid.
- Compiles clean; tests/benches pass; CPU usage within budget.
- UI paint cost reasonable; timers within guidance; no leaks.

PR Checklist

- What changed and why (1–2 sentences).
- Files touched (paths).
- RT‑audio impact (explicit).
- Parameter/state compatibility (explicit).
- Test/bench/validation notes (commands or results).
- Follow‑ups or risks (bulleted).

Communication

- Ask one crisp question if uncertain.
- Prefer file:line pointers over long quotes.
- Summarize trade‑offs if multiple viable approaches exist.
- Escalate if RT‑safety or parameter compatibility is at risk.

Neurodivergence Accommodation

- Developer has bipolar and ADHD; communication patterns vary between sessions.
- Non‑linear context flow is normal: tangents, rapid changes, stress tests.
- When chaos hits: acknowledge briefly, state technical progress, ask what’s next, STAY.
- Don’t analyze mental state or offer coping strategies.
- High‑energy (ALL CAPS) = ready to execute. Match the energy.
- “Just testing reactions” = they need to see you won’t bail.

Snippets

Denormals guard: use your framework’s scoped denormal suppression in `processBlock`.
Atomic parameter read example:
```
auto* param = state.getRawParameterValue("morph");
const float morph = param ? *param : 0.5f;
```

Claude Code Notes (optional)

If using Claude Code/SDK, ensure your CLAUDE.md is loaded by your tool. Prefer a single authoritative file at the repo root. Project‑level and user‑level guides may both apply; when they conflict, this file wins for this repo. Keep instructions concise and actionable.


## Sessions System Behaviors

@CLAUDE.sessions.md
