# Story: SI-20251111-01-simulation-instrumentation — Simulation Function Trace & Editing

> **Naming Convention**  
> Store active story specs under `docs/missing implementation/` using the identifier format above, for example `SI-20250517-01-led-diagnostics.md`.  
> Reference the story from `docs/stories to implement/missing_implementation_stories.md` with the same ID so incremental progress stays traceable.

**Status:** in-progress  
**Author:** Matteo  
**Last Updated:** 2025-11-11  
**Linked Requirements:** docs/Requirements/feature addition/Display_Web_UI_Workspace_Improvements.md#31-simulation-instrumentation  
**Related Features:** Display Web UI workspace

---

## 1. Summary

Upgrade the Simulation tab so it mirrors firmware behaviour: every simulated event must display which firmware action would run, value placeholders must behave like real bindings (edit, save, revert), and transaction/transition effects need to preview hardware animations. Designers rely on this view to confirm JSON datasets map 1:1 to firmware action IDs.

---

## 2. Acceptance Criteria

- [ ] Simulated button/data events emit a visible function trace (action ID + params) sourced from the imported firmware manifest.
- [ ] Value placeholders accept inline edits, show delta vs. source data, and fire edit/save callbacks reflecting actual firmware actions.
- [ ] Transaction/transition previews mirror the hardware library effects (enter/exit animations, fades) and highlight target screens.
- [ ] Function trace log is filterable/replayable for the most recent interactions.

---

## 3. Implementation Notes

- Extend `Simulation` React components to subscribe to a new action-dispatch layer that mirrors `UiActionRegistry` resolution.
- Integrate the firmware manifest (produced in SI-20251111-03) so each event has a human-friendly name and parameter schema.
- Enhance value placeholder components to support two-way binding: editing updates dataset clone + validation, saving triggers synthetic `save-config` actions.
- Create animation helpers that drive the same easing/timing as the firmware library (requires coordination with requirement 5 open item).

---

## 4. Tasks & Checklist

- [ ] Build manifest ingestion hook in the mockup app and expose it to the Simulation tab context.
- [x] Implement function trace panel with filtering, timestamps, and parameter rendering.
- [x] Add value placeholder editing widgets (inline input with validation + revert) tied to simulated action events.
- [ ] Mirror firmware transition effects (enter/exit) using matching easing/timing constants.
- [ ] Update Playwright tests to assert function trace output and edit/save flows.
- [ ] Document the Simulation workflow in Help.

---

## 5. Verification Plan

1. Automated Playwright scenario pressing buttons and asserting trace entries match expected action IDs.
2. Manual edit/save cycle verifying that deltas appear and traces fire `edit` + `save` actions.
3. Visual confirmation that transition previews align with hardware (compare with firmware video/reference).

---

## 6. Backout / Fallback Strategy

- Keep legacy Simulation logic behind a feature flag so designers can revert to the previous static preview if regressions appear.
- Retain existing dataset schema to avoid corrupting user JSON if the new editing pipeline fails; disable editing when manifest import errors occur.

---

## 7. Notes & Open Questions

- Depends on SI-20251111-03 for the firmware manifest and schema updates.
- Transition effect implementation may require confirmation from firmware maintainers on the final animation library/API.
