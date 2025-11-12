# UI Brief (No‑Mock Edition) — Muse v1.0 UI

Attach this to the release PR and link it from `docs/UI_SHIP_CHECKLIST.md`.

## 1) What We're Building
- Feature name: Muse v1.0 UI — OLED Séance (400×600 vertical)
- Primary user / persona: Electronic music producers and sound designers using VST/AU in DAWs; secondary: live performers needing immediate visual feedback.
- Problem statement: Modern plugin UIs are polished but interchangeable; we need a distinctive, “haunted hardware” interface that is felt instantly and recognized at a glance.
- Desired outcome: A minimal UI with a phosphor “mouth” that snaps at 10 FPS and three smooth 60 FPS knobs (MORPH, INTENSITY, MIX). Users describe it as haunted/otherworldly; DAWs see a stable, responsive plugin.

## 2) Constraints & Guardrails
- Supported OS/hosts: Windows 10/11 (x64), macOS 12+ (Intel/Apple). Formats: VST3, AU, AUv3, Standalone; CLAP id wired via CMake (optional for v1.0).
- Must‑use components: `source/ui/HalftoneMouth.h` (LED mouth), `PluginEditor` custom knob drawing (`drawKnob`, `drawOLEDGlowText`), APVTS attachments, Melatonin Inspector in Debug.
- Design tokens source: `design/muse-design-system.json` (locked 2025‑11‑10; see `source/ui/MuseSeanceTokens.h`).
- Non‑goals: Multi‑page UI, complex meters/analysers, alternate themes, long copy/tooltips everywhere, resizable window, more than three primary knobs.

## 3) Core Interactions
- Controls and units: 
  - MORPH (0–1), INTENSITY (0–1), MIX (0–1); Shape Pair index (0–3) via backend/auto; APVTS ids: `morph`, `intensity`, `mix`, `pair`.
- Gestures & modifiers: Vertical drag; double‑click resets to 0.5; velocity‑based fine control; mouse wheel increments; tooltip popup on drag.
- Error/empty states: Missing preset file; invalid preset load; no audio input (mouth dims); backend pair auto‑selection unavailable.

## 4) States to Demonstrate
- Idle (mint on black; subtle mouth breathing; 60 FPS UI, 10 FPS mouth snap).
- Hover / Focus (knob hover glow; focus ring visible; keyboard focus order sane).
- Dragging / Editing (smooth knob response; tooltip value; APVTS updates).
- Disabled / Error / Learn (N/A for v1; preset error toast if file invalid).

## 5) Navigation & Layout
- Entry points: Open editor from host; load/save preset; recall from session.
- Resizing behavior: Fixed 400×600; no internal zoom for v1.0.
- Scaling expectations: Windows 100/125/150/200%; macOS 1×/2×. No blurriness; text and dots remain crisp; mixed‑DPI monitor hop safe.

## 6) Acceptance Criteria
- Visual identity: Only black (#000000) and mint phosphor (#d8f3dc) on the OLED area; no background gradients. Phosphor glow visible but subtle.
- Mouth behaviour: Updates at 10 FPS (100 ms); blends per pair rules; responds to audio level; no stutter in knob interaction.
- Knobs: Vertical drag; velocity mode; double‑click to 0.5; tooltip shown; automation writes reflect movements in supported hosts.
- Performance: Editor open/close leak‑free; idle CPU minimal; UI frame within 8–16 ms during active interaction on typical machines.
- Packaging metadata: PRODUCT_NAME "Muse", COMPANY_NAME "Muse Audio", BUNDLE_ID `com.museaudio.muse`, formats VST3/AU/AUv3/Standalone present; CLAP id set to bundle id.
- Host recall: Session save/load restores parameters and UI state; factory presets load instantly.

## 7) References
- Vision & rules: `NORTH_STAR.md` (locked 2025‑11‑10)
- Roadmap & scope: `docs/ROADMAP.md`
- Implementation notes: `OLED_UI_IMPLEMENTATION.md`, `docs/SCREENSHOT2_UI_IMPLEMENTATION.md`
- Code: `source/PluginEditor.h`, `source/PluginEditor.cpp`, `source/ui/HalftoneMouth.h`, `source/PluginProcessor.h`
- Tokens: `design/muse-design-system.json`, `source/ui/MuseSeanceTokens.h`
- Build/ids: `CMakeLists.txt` (PRODUCT_NAME, COMPANY_NAME, BUNDLE_ID, FORMATS)
