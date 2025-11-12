# Backend Shipping Checklist (DSP + Engine + Packaging)

Use this for pre‑release verification of DSP correctness, thread‑safety, performance, and packaging. Dates below reflect current repo state on 2025‑11‑12.

## 0) Build Matrix & IDs
- [x] Standalone build OK → `build/Muse_artefacts/Debug/Standalone/Muse.exe` (Windows)
- [x] VST3 build OK → `build/Muse_artefacts/Debug/VST3/Muse.vst3` (also copied to `C:\Program Files\Common Files\VST3\Muse.vst3`)
- [x] CLAP build OK → `build/Muse_artefacts/Debug/CLAP/Muse.clap`
- [ ] AU/AUv3 (macOS) — not verified on this machine
- [x] Identifiers: `PRODUCT_NAME="Muse"`, `COMPANY_NAME="Muse Audio"`, `BUNDLE_ID="com.museaudio.muse"`, `CLAP_ID=BUNDLE_ID`  
  File: `CMakeLists.txt`

## 1) Parameters & APVTS Wiring
- [x] APVTS exposes `pair`, `morph`, `intensity`, `mix`; UI attachments present.  
  Files: `source/PluginProcessor.h`, `source/PluginEditor.cpp`, `source/MusePluginEditor.cpp`
- [x] UI→DSP sync (no zippering) with internal smoothing in DSP engines.  
  Files: `dsp/ZPlaneFilter_fast.h`, `dsp/ZPlaneFilter.h`, `source/dsp/MuseZPlaneEngine.cpp`
- [x] Visualizer inputs: audio level, pair, morph, intensity — fed each frame to `HalftoneMouth`.

## 2) DSP Correctness & Safety
- [x] Audio callback: no heap allocation; RT‑safe.  
  File: `source/PluginProcessor.cpp`
- [x] Pole/radius clamping and NaN detection; async UI notifications via `AsyncUpdater`.  
  Files: `source/PluginProcessor.h/.cpp`
- [x] Denormals handled (JUCE defaults + smoothing design).  
  Files: `dsp/*`, `modules/zplane-dsp/include/zplane/*`
- [ ] pluginval run (strict) on VST3/CLAP binaries — pending.

## 3) Thread‑Safety & Comms
- [x] Atomics for UI reads: `audioLevel_`, `currentVowelShape_`, psycho features, NaN flag.  
  File: `source/PluginProcessor.h`
- [x] UI polling via `Timer`; async message delivery; no UI objects touched from audio thread.

## 4) Performance Targets
- [ ] Scalar CPU (Debug): < 10% on mid‑range CPU during interaction (baseline to be recorded).  
- [ ] Release CPU: < 5% typical; capture with DAW + 44.1/48/96 kHz.  
- [ ] SIMD path: enable SSE2/AVX variants if available; benchmark vs scalar.  
  Files: `dsp/ZPlaneFilter_fast.h`, `docs/ZPLANE_OPTIMIZATION_GUIDE.md`

## 5) State, Presets, Recall
- [x] Session state round‑trip via APVTS.  
- [ ] Preset save/load: verify path `C:\Muse\MuseAudio\new\*.musepreset` and permissions.  
  Files: `source/PresetManager.h/.cpp`

## 6) Visualizer ↔ DSP Contract
- [x] `HalftoneMouth` inputs: `setPair(int)`, `setMorph(float)`, `setIntensity(float)`, `setAudioLevel(float)`
- [x] LipHalftone mode (no teeth) responds to formant blending; intensity controls lip thickness.  
  File: `source/ui/HalftoneMouth.h`
- [ ] Optional 10 Hz snap gate (hardware stutter) — TODO flag to quantize visual updates without affecting control feel.

## 7) Error Handling & Diagnostics
- [x] NaN detection surfaces to UI; glitch frame trigger on detection.  
  Files: `source/PluginProcessor.h`, `source/MusePluginEditor.cpp`
- [ ] Lightweight diagnostics view (CPU %, xrun count) — optional QA toggle.

## 8) Tests & Benchmarks (Current Issues)
- [ ] Benchmarks target fails to link (duplicate `AuthenticEMUZPlane` symbols between `AuthenticEMUZPlane.obj` and `EMUFilter.obj`).  
  Fix options:
  - Remove the duplicate TU from `Benchmarks` target or mark one as interface‑only; ensure a single definition.
  - Prefer linking against a single compiled library that owns the implementation.
- [ ] Tests target fails: missing include `EMUAuthenticTables_VERIFIED.h`.  
  Fix options:
  - Add include path to the file’s directory or update the test to include `modules/zplane-dsp/include/zplane/EMUAuthenticTables.h` if intended.

## 9) Packaging, Signing, Validation
- [ ] Windows: run pluginval against `Muse.vst3` and `Muse.clap` (Debug + Release).  
  Example: ``pluginval --strict --verbose --validate-in-process --file "C:\\Program Files\\Common Files\\VST3\\Muse.vst3"``
- [ ] macOS: codesign + notarize (if distributing), then pluginval.  
  See: `cmake/README.md` and Pamplejuce docs.

---

Sign‑off

| Area | Owner | Date | Notes |
| --- | --- | --- | --- |
| Build Matrix |  |  |  |
| Params/APVTS |  |  |  |
| DSP Safety |  |  |  |
| Performance |  |  |  |
| Presets/Recall |  |  |  |
| Visualizer Contract |  |  |  |
| Tests/Benchmarks |  |  |  |
| Packaging/Validation |  |  |  |

