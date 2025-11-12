# Story Refinement Findings — SI-20251111-01 simulation UI layout follow-up

> **File naming:** store each report as `SI-<story-id>-<slug>-refinement-findings.md` (example: `SI-20250517-01-01-web-mockup-refinement-findings.md`).

**Story ID:** SI-20251111-01  
**Story Title:** Simulation Function Trace & Editing  
**Implementation Artifact:** `web/mockup/src/App.tsx`, `src/components/SimulationTracePanel.tsx`, `src/components/ValuePlaceholderPanel.tsx`  
**Reviewer:** Codex  
**Review Date:** 2025-11-11  
**Status Recommendation:** resolved

---

## 1. Snapshot Summary
- **Observed Outcome:** Simulation tab renders the new function trace and value editor blocs stacked under the viewport; button events list still precedes the trace panel and the trace list grows vertically without scroll limiting.
- **Intended Outcome:** Product request asks for the function trace on the right with a scrollable area showing the last five entries, a clear button, value editors under the StampPLC buttons block, and button events relocated beneath screen details.
- **Gap Statement:** Layout does not yet match the requested structure—trace block uses full width, lacks scroll/clear controls, and value/button panels remain in their original positions.

---

## 2. Findings Log

| # | Area / File | Severity* | Finding | Evidence (line / behaviour) | Proposed Refinement |
|---|-------------|-----------|---------|------------------------------|---------------------|
| 2.1 | `web/mockup/src/App.tsx`, `src/components/SimulationTracePanel.tsx`, `src/App.css` | Medium | Function trace panel spans main column, displays all entries; no scroll limit or clear button. | Prior layout (`App.tsx:604-666`) rendered the trace inline. | ✅ Implemented: trace now lives in the third column (`panel-grid--simulation`), includes a filter + clear button, and scrolls within a fixed-height container (see `App.tsx:604-679`, `SimulationTracePanel.tsx`, `App.css:284-360`). |
| 2.2 | `web/mockup/src/App.tsx` | Low | Value placeholder panel sits below trace panel inside main column rather than under the StampPLC buttons block. | Previous placement after trace block. | ✅ Implemented: `ValuePlaceholderPanel` is rendered immediately after `<ButtonPanel>` so edits align with the input controls (`App.tsx:632-645`). |
| 2.3 | `web/mockup/src/App.tsx` | Low | Button events section still appears after layout diagnostics within the main column, not beneath screen details. | Original `interaction-log` section lived in the main column. | ✅ Implemented: button events now render inside the `screenContext` column below “Screen details” (`App.tsx:477-503`). |

> *Severity guidance: **High** (blocks requirement / creates defect), **Medium** (partial compliance / risky), **Low** (polish / documentation).

---

## 3. Suggested Refinement Tasks
- [x] Update Simulation layout grid to include a right-side column housing the function trace panel (with max height, scrollbar, and clear button).
- [x] Relocate the value editor under the StampPLC buttons block; ensure styling matches surrounding controls.
- [x] Move the button events log into the screen-context column beneath “Screen details”.
- [ ] Add tests/documentation screenshots reflecting the new layout arrangement.

---

## 4. Dependencies & Questions
- **Upstream Impact:** None; changes isolated to web mockup layout/styling.
- **Open Questions:** Confirm preferred width for the right-hand trace panel and whether the clear button should preserve replay history.
- **Testing Gaps:** Need updated Playwright snapshots for Simulation tab after layout adjustments.

---

## 5. Attachments & Links
- `docs/missing implementation/SI-20251111-01-simulation-instrumentation.md`
- `docs/Requirements/feature addition/Display_Web_UI_Workspace_Improvements.md`

---

## 6. Reviewer Notes
- Layout adjustments can reuse existing grid styling; consider grouping Simulation-specific panels in a flex container for easier reordering.
