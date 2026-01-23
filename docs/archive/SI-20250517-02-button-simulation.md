# Story: SI-20250517-02-button-simulation — Button Input Simulation Framework

> **Naming Convention**  
> Store active story specs under `docs/missing implementation/` using the identifier format above, for example `SI-20250517-02-button-simulation.md`.  
> Reference the story from `docs/stories to implement/missing_implementation_stories.md` with the same ID so incremental progress stays traceable.

**Status:** completed  
**Author:** Matteo  
**Last Updated:** 2025-05-17  
**Linked Requirements:** docs/Requirements/feature addition/Display_Web_Mockup_and_Translator.md#32-button-interaction-simulation  
**Related Features:** StampPLC UI redesign

---

## 1. Summary

Add virtual button controls (UP, DOWN, ENTER) within the web mockup so users can exercise the UI state machine, including long-press and repeat behaviour.

---

## 2. Acceptance Criteria

- [x] Simulated buttons mirror firmware timing thresholds (short press, 1.5 s long press, 250 ms repeats).
- [x] Keyboard bindings (arrow keys + Enter) trigger the same events as on-screen controls.
- [x] Interaction log captures each event with timestamp and resulting state transition.
- [x] `npm run test:visual` completes without snapshot regressions.

---

## 3. Implementation Notes

- Reuse the firmware timing rules via a shared simulation hook that drives both pointer and keyboard paths.
- Button states and timers live in a central hook so UI components stay declarative and testable.
- Visual feedback reflects pressed/held status for debugging long-press flows.
- End-to-end coverage relies on Playwright keyboard scenarios.

---

## 4. Tasks & Checklist

- [x] Implement button components with press/hold detection and timers.
- [x] Add keyboard shortcuts (Arrow keys + Enter) tied into the same simulation timers.
- [x] Wire button events to update the currently displayed screen via the state machine.
- [x] Render an on-screen event log capturing timestamp and resulting screen ID.

---

## 5. Verification Plan

1. Playwright visual regression suite confirms viewport snapshots remain stable.
2. Playwright keyboard scenarios assert navigation/log output for short and long presses.
3. Manual validation of pointer interactions against long-press countdown behaviour.

---

## 6. Backout / Fallback Strategy

- Keep interaction logic modular so it can be disabled without affecting static mockups.
- Preserve existing static screens to avoid blocking designers if simulation layer regresses.

---

## 7. Notes & Open Questions

- Evaluate need for adjustable timing parameters for experimentation.
- Consider exporting interaction logs for regression documentation.
