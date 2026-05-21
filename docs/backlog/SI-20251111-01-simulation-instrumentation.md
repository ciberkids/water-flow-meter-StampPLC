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

- [x] Simulated button/data events emit a visible function trace (action ID + params) sourced from the imported firmware manifest.
- [x] Value placeholders accept inline edits, show delta vs. source data, and fire edit/save callbacks reflecting actual firmware actions.
- [x] Transaction/transition previews mirror the hardware library effects (enter/exit animations, fades) and highlight target screens.
- [x] Function trace log is filterable/replayable for the most recent interactions.

---

## 3. Implementation Notes

- Extend `Simulation` React components to subscribe to a new action-dispatch layer that mirrors `UiActionRegistry` resolution.
- Integrate the firmware manifest (produced in SI-20251111-03) so each event has a human-friendly name and parameter schema.
- Enhance value placeholder components to support two-way binding: editing updates dataset clone + validation, saving triggers synthetic `save-config` actions.
- Create animation helpers that drive the same easing/timing as the firmware library (requires coordination with requirement 5 open item).
- Transition previews now render a scaled version of the destination screen with effect-specific motion (slide up/down, fade, scale) plus action metadata (`web/mockup/src/App.tsx`, `src/components/DisplayViewport.tsx`, `src/App.css`).
- Playwright suite covers transition overlays, manifest-labelled traces, and refreshed multi-resolution snapshots for the key screens/tabs (`web/mockup/tests/visual/mockup.spec.ts`, `tests/visual/mockup.spec.ts-snapshots/`).

---

## 4. Tasks & Checklist

- [x] Build manifest ingestion hook in the mockup app and expose it to the Simulation tab context.
- [x] Implement function trace panel with filtering, timestamps, and parameter rendering.
- [x] Add value placeholder editing widgets (inline input with validation + revert) tied to simulated action events.
- [x] Mirror firmware transition effects (enter/exit) using matching easing/timing constants.
- [x] Update Playwright tests to assert function trace output and edit/save flows.
- [x] Document the Simulation workflow in Help.

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

---

# Refinement Findings #1 (Legacy SI-20251111-01-01)
**Review Date:** 2025-11-11 | **Status:** resolved

## Snapshot Summary
- **Observed Outcome:** Simulation tab renders the new function trace and value editor blocs stacked under the viewport; button events list still precedes the trace panel and the trace list grows vertically without scroll limiting.
- **Intended Outcome:** Product request asks for the function trace on the right with a scrollable area showing the last five entries, a clear button, value editors under the StampPLC buttons block, and button events relocated beneath screen details.
- **Gap Statement:** Layout does not yet match the requested structure—trace block uses full width, lacks scroll/clear controls, and value/button panels remain in their original positions.

## Findings Log

| # | Area / File | Severity | Finding | Evidence | Proposed Refinement |
|---|-------------|----------|---------|----------|---------------------|
| 2.1 | `web/mockup/src/App.tsx`, `src/components/SimulationTracePanel.tsx` | Medium | Function trace panel spans main column, displays all entries; no scroll limit or clear button. | Prior layout inline. | ✅ Implemented: trace now lives in the third column (`panel-grid--simulation`), includes a filter + clear button, and scrolls within a fixed-height container. |
| 2.2 | `web/mockup/src/App.tsx` | Low | Value placeholder panel sits below trace panel inside main column rather than under the StampPLC buttons block. | Previous placement after trace block. | ✅ Implemented: `ValuePlaceholderPanel` is rendered immediately after `<ButtonPanel>` so edits align with the input controls. |
| 2.3 | `web/mockup/src/App.tsx` | Low | Button events section still appears after layout diagnostics within the main column, not beneath screen details. | Original `interaction-log` section lived in main column. | ✅ Implemented: button events now render inside the `screenContext` column below “Screen details”. |

## Suggested Refinement Tasks
- [x] Update Simulation layout grid to include a right-side column housing the function trace panel (with max height, scrollbar, and clear button).
- [x] Relocate the value editor under the StampPLC buttons block; ensure styling matches surrounding controls.
- [x] Move the button events log into the screen-context column beneath “Screen details”.
- [x] Add tests/documentation screenshots reflecting the new layout arrangement.

---

# Refinement Findings #2 (Legacy SI-20251111-01-02)
**Review Date:** 2025-11-11 | **Status:** resolved

## Snapshot Summary
- **Observed Outcome:** The new function-trace column introduces filtering and scrollability, but the clear button spills outside the card, the scroll viewport is cramped, and the Simulation sidebar still consumes limited width because workspace stats/theme snapshot remain stacked in the left sidebar.
- **Intended Outcome:** Trace tools should fit comfortably inside their card, showing entries without clipping; the Simulation area must gain horizontal room by relocating non-essential sidebar widgets to other tabs/panels.
- **Gap Statement:** Current layout impairs readability — controls overflow their containers, trace entries wrap excessively, and the sidebar space is underutilized for Simulation-specific tools.

## Findings Log

| # | Area / File | Severity | Finding | Evidence | Proposed Refinement |
|---|-------------|----------|---------|----------|---------------------|
| 2.1 | `web/mockup/src/components/SimulationTracePanel.tsx` | Medium | Clear button sits outside the trace card and overlaps surrounding grid. | Prior layout cramped header controls. | ✅ Implemented: header now wraps content, filter + clear button sit inside `.trace-actions` with full-width flex container. |
| 2.2 | `web/mockup/src/App.css` | Medium | Trace scroll box is too narrow; long IDs/JSON wrap awkwardly and clip. | Old `.panel-grid--simulation` limited col to 320px. | ✅ Implemented: trace column widened to 380px, scroll container set to `width: 100%`, and JSON blocks allow horizontal scroll. |
| 2.3 | `web/mockup/src/App.tsx` | Low | Theme snapshot still lives in the left sidebar, taking vertical space on Simulation tab. | Sidebar previously hosted overview/theme sections. | ✅ Implemented: new `workspace-banner` displays validation/screen stats across the top, sidebar removed; theme snapshot now only appears inside the Design tab context column. |

## Suggested Refinement Tasks
- [x] Redesign the trace header to keep filter and clear button contained (stack on small widths, enlarge card padding/margins).
- [x] Widen trace column / increase `.panel-grid--simulation` third column width and adjust scroll container padding so JSON entries fit comfortably.
- [x] Relocate “Theme snapshot” content into the Design tab; convert “Workspace overview” into a top-of-workspace banner with inline stats to free sidebar height.
- [x] Update snapshots/docs once layout polish is applied.

