---
name: UI Release Checklist
about: Track UI verification for a release (works without mocks)
title: "UI Release: <version>"
labels: ["release", "ui"]
assignees: []
---

## Owners
- Dev:
- QA:
- Design/PM:

## Links
- Brief: docs/UI_BRIEF_TEMPLATE.md
- Checklist: docs/UI_SHIP_CHECKLIST.md
- Build/Artifacts:

## Checks
- [ ] Visual tokens applied (type, color, spacing)
- [ ] States (idle/hover/focus/drag/disabled/error) reviewed
- [ ] Resizing + scaling: Win 100/125/150/200, macOS 1x/2x
- [ ] Keyboard navigation + focus order
- [ ] Accessibility names/roles set (JUCE AccessibilityHandler)
- [ ] Performance: smooth interaction; idle CPU low
- [ ] Presets: browse/load/save; session recall exact
- [ ] Automation: write/read; parameter text formatting
- [ ] Error/empty states produce clear UI messages
- [ ] Copy/branding/version strings correct
- [ ] Packaging identifiers, icons, signing/notarization

## Host Matrix (check those exercised)
- [ ] REAPER (Win/macOS)
- [ ] Ableton Live (Win/macOS)
- [ ] Logic Pro (macOS)
- [ ] FL Studio (Win)
- [ ] Studio One (Win/macOS)
- [ ] Bitwig (Win/macOS)

## Baselines & Evidence
- Screenshots: `docs/ui-baselines/<version>/`
- Notes:

## Sign‑off
- Visual:
- Accessibility:
- Functional:
- Performance:
- Packaging:
- Release:

