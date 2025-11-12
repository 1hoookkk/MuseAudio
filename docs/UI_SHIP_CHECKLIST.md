# UI Shipping Checklist (JUCE Plugin)

Use this for pre‑release and launch‑day verification. Link this doc (or the GitHub issue template) to each release PR. If you don't have mocks, pair this with the brief below and annotated screenshots.

Brief: `docs/UI_BRIEF_TEMPLATE.md`

## 0) Alignment & Scope
- [x] One‑page brief completed (goals, personas, non‑goals, acceptance). See `docs/UI_BRIEF_TEMPLATE.md`.
- [x] Release target and owner assigned (Dev, QA, Design/PM).  
      Target: v1.0 (November 2025) — Owner: Dev (Hooki) · QA: Self · Design: Self
- [x] Component inventory listed:  
      `HalftoneMouth` (LED mouth), custom knobs in `PluginEditor`, tooltip value popups, optional Melatonin Inspector (Debug), APVTS attachments; Preset backend present (`PresetManager`).
- [x] Token source locked: `design/muse-design-system.json` (OLED Séance; locked 2025‑11‑10).  
      Note: Shipping skin uses a dark‑teal chassis; OLED area strictly follows mint‑on‑black tokens.

## 1) Visual Consistency & States
- [ ] Typography, color, spacing match tokens; no ad‑hoc values.  
      OLED area: black + mint only; chassis: dark teal (shipping skin).
- [ ] All interactive states covered: idle, hover, focus, drag, disabled, error, learn/midi‑map.  
      Focus ring visible on knobs; hover glow planned.
- [ ] Knobs/sliders: value arcs (N/A — custom line/dot), detents (N/A), double‑click reset (Yes), velocity fine‑tune (Yes), tooltip/value box (Yes).
- [ ] Meters/scopes: LED mouth respects 10 FPS snap; repaint budget maintained; clipping colors N/A.
- [ ] Icons/art packaged; no missing textures at runtime; Hi‑DPI assets present.  
      Icons: `packaging/icon.png`, `packaging/muse.icns`.
- [ ] Visual baseline screenshots captured for key views (store under `docs/ui-baselines/v1.0/`).

Current state (2025‑11‑12):
- [x] LipHalftone mode renders a filled, no‑teeth mouth (dense grid 60×28) matching mock; driven by `pair`+`morph` with `intensity` affecting thickness.  
  File: `source/ui/HalftoneMouth.h` (Style::LipHalftone)
- [x] Footer branding removed (no “AUDIOFABRICA”) in both editors.  
  Files: `source/PluginEditor.cpp`, `source/MusePluginEditor.cpp`
- [ ] Optional 10 FPS mouth snap gating (hardware feel) – not yet implemented; UI currently updates each frame.

## 2) Layout, Resizing, and Scaling
- [ ] Window size fixed at 400×600; no resize in v1.0; contents remain centred/aligned.
- [ ] Scaling matrix: Windows 100/125/150/200%, macOS 1×/2× (Retina). No blurriness or clipped text.
- [ ] Host zoom + (N/A) plugin internal zoom interact correctly.
- [ ] Multi‑monitor and mixed DPI (drag window between monitors) verified.

## 3) Accessibility & Input
- [ ] Keyboard navigation (Tab/Shift+Tab) reaches all controls in logical order.
- [ ] `AccessibilityHandler` names/roles/states set for controls (labels read correctly).
- [ ] Visible focus ring on all focusable elements; contrast at recommended levels for text/UI.
- [ ] Mouse, trackpad, pen tested; wheel gesture increments are predictable and bounded.
- [ ] Screen reader smoke (VoiceOver on macOS, Narrator on Windows) announces core controls.

## 4) Functional UI Flows
- [ ] Parameter binding: UI ↔ processor sync, no zippering; smoothing when dragging.
- [ ] Undo/redo updates UI; default value reset works via double‑click or context menu.
- [ ] Bypass/mute/solo/compare A/B UI states sync with audio engine. (N/A for v1.0)
- [ ] Preset flows update UI immediately; dirty indicator behaves correctly.  
      Backend present (`PresetManager`); minimal UI affordances in v1.0.
Verification notes (2025‑11‑12):
- [x] `HalftoneMouth` receives `pair`, `morph`, `intensity`, and audio level each frame from `PluginEditor`/`MusePluginEditor` timer.  
  Files: `source/PluginEditor.cpp:timerCallback`, `source/MusePluginEditor.cpp:timerCallback`

## 5) Performance & Responsiveness
- [ ] UI thread frame time within target (8–16 ms under interaction).
- [ ] Repaints throttled; timers use sensible intervals (`HalftoneMouth` 60 Hz, mouth snap 10 Hz); no busy loops.
- [ ] Opening/closing editor is instantaneous and leak‑free; profile allocations on open.
- [ ] Idle CPU usage minimal in heavy hosts and on low‑power laptops.
Notes:
- [ ] Implement optional mouth snap gate at 10 Hz without affecting knob response; add toggle/constant in `HalftoneMouth`.

## 6) Presets, Browser, and Session Recall
- [ ] Preset list loads quickly; sorting/filters stable; long names truncate gracefully.
- [ ] Program change integration (if supported) updates UI safely.
- [ ] Session save/load in target hosts restores UI state exactly.
- [ ] Factory vs. user preset badges; write‑protection errors show clear UI messaging.

## 7) Host Integration & Automation
- [ ] Automation write/read reflect UI movements; touch/latch/read modes behave.
- [ ] Parameter text formatting (units, decimals) matches spec; automation lanes readable.
- [ ] MIDI learn / external mapping (if present) provides clear on‑screen affordances.
- [ ] Host suspend/resume/transport changes don't freeze or desync the UI.

## 8) Error, Empty, and Edge States
- [ ] Missing data (shapes, presets, resources) triggers non‑blocking UI messages.
- [ ] Licensing/activation states render clear CTAs; offline mode degrades gracefully. (N/A v1.0)
- [ ] File dialogs (import/export presets) remember last path; invalid files show guidance.

## 9) Copy, Branding, and Diagnostics
- [ ] Product name, version, and build channel visible (e.g., in About or status bar).  
      PRODUCT_NAME "Muse" · COMPANY_NAME "Muse Audio" · VERSION from `VERSION`/CMake.
- [ ] Keyboard modifiers and platform terms localized (Cmd vs Ctrl, Option vs Alt) per OS.
- [ ] Tooltips concise and consistent; no lorem ipsum; links and shortcuts accurate.
- [ ] Optional: small diagnostics panel gated behind a debug flag for QA capture.  
      Melatonin Inspector included in Debug builds.

## 10) Packaging & Identifiers
- [ ] Formats built as required: VST3, AU, AUv3, Standalone; CLAP id configured.  
      CMake: PRODUCT_NAME "Muse" · COMPANY_NAME "Muse Audio" · BUNDLE_ID `com.museaudio.muse`.
- [ ] Icons, plist/manifests updated; notarization/signing where applicable.  
      Assets: `packaging/icon.png`, `packaging/muse.icns`; see Pamplejuce signing guides.
- [ ] Version string matches CHANGELOG and host‑visible metadata.
Build evidence (2025‑11‑12):
- [x] Built Debug: Standalone (`build/Muse_artefacts/Debug/Standalone/Muse.exe`), VST3 (installed to `C:\Program Files\Common Files\VST3\Muse.vst3`), CLAP (`build/Muse_artefacts/Debug/CLAP/Muse.clap`).
- [ ] AU/AUv3 not verified (macOS required).

## 11) Host Matrix (Smoke Targets)
List your target hosts and OSes. Adjust as needed.

- Windows: REAPER, Ableton Live, FL Studio, Studio One
- macOS: Logic Pro, Ableton Live, Bitwig, REAPER

For each host:
- [ ] Open/close editor repeatedly
- [ ] Scale matrix pass (100/125/150/200 on Win; 1×/2× on macOS)
- [ ] Automation write/read pass
- [ ] Preset load/save + session recall

## 12) Launch Ops
- [ ] Rollback plan documented; last‑known‑good build referenced.
- [ ] Known issues labeled with workarounds; support macro ready.
- [ ] Post‑launch watch items assigned (crash, UI regressions, feedback inbox).

---

Sign‑off

| Area | Owner | Date | Notes |
| --- | --- | --- | --- |
| Visual | Hooki (Dev/Design) | 2025‑11‑12 | OLED area honors tokens; chassis teal skin OK for v1.0 |
| Accessibility |  |  | Keyboard order + focus ring to verify |
| Functional | Hooki | 2025‑11‑12 | APVTS bindings; double‑click reset; tooltip values |
| Performance |  |  | Measure with Inspector; target 8–16 ms frame |
| Packaging |  |  | BUNDLE_ID com.museaudio.muse; formats VST3/AU/AUv3/Standalone; CLAP id set |
| Release |  |  | Host matrix smoke pass; screenshots archived |
