# Story Refinement Findings — SI-20251111-01 simulation trace visual polish

> **File naming:** store each report as `SI-<story-id>-<slug>-refinement-findings.md` (example: `SI-20250517-01-01-web-mockup-refinement-findings.md`).

**Story ID:** SI-20251111-01  
**Story Title:** Simulation Function Trace & Editing  
**Implementation Artifact:** `web/mockup/src/App.tsx`, `src/App.css`, `src/components/SimulationTracePanel.tsx`  
**Reviewer:** Codex  
**Review Date:** 2025-11-11  
**Status Recommendation:** resolved

---

## 1. Snapshot Summary
- **Observed Outcome:** The new function-trace column introduces filtering and scrollability, but the clear button spills outside the card, the scroll viewport is cramped, and the Simulation sidebar still consumes limited width because workspace stats/theme snapshot remain stacked in the left sidebar.
- **Intended Outcome:** Trace tools should fit comfortably inside their card, showing entries without clipping; the Simulation area must gain horizontal room by relocating non-essential sidebar widgets to other tabs/panels.
- **Gap Statement:** Current layout impairs readability — controls overflow their containers, trace entries wrap excessively, and the sidebar space is underutilized for Simulation-specific tools.

---

## 2. Findings Log

| # | Area / File | Severity* | Finding | Evidence (line / behaviour) | Proposed Refinement |
|---|-------------|-----------|---------|------------------------------|---------------------|
| 2.1 | `web/mockup/src/components/SimulationTracePanel.tsx`, `src/App.css` | Medium | Clear button sits outside the trace card and overlaps surrounding grid; card width isn’t sufficient for button + filter. | Prior layout cramped header controls. | ✅ Implemented: header now wraps content, filter + clear button sit inside `.trace-actions` with full-width flex container (`SimulationTracePanel.tsx`, `App.css:300-340`). |
| 2.2 | `web/mockup/src/App.css` | Medium | Trace scroll box is too narrow; long IDs/JSON wrap awkwardly and clip. | Old `.panel-grid--simulation` limited third column to 320px. | ✅ Implemented: trace column widened to 380px, scroll container set to `width: 100%`, and JSON blocks allow horizontal scroll (`App.css:600-640`). |
| 2.3 | `web/mockup/src/App.tsx`, `src/App.css` | Low | Theme snapshot still lives in the left sidebar, taking vertical space on Simulation tab; workspace overview remains in sidebar rather than a top banner, limiting room for Simulation tools. | Sidebar previously hosted overview/theme sections. | ✅ Implemented: new `workspace-banner` displays validation/screen stats across the top, sidebar removed; theme snapshot now only appears inside the Design tab context column (`App.tsx:470-520`, `App.css:210-260`). |

> *Severity guidance: **High** (blocks requirement / creates defect), **Medium** (partial compliance / risky), **Low** (polish / documentation).

---

## 3. Suggested Refinement Tasks
- [x] Redesign the trace header to keep filter and clear button contained (stack on small widths, enlarge card padding/margins).
- [x] Widen trace column / increase `.panel-grid--simulation` third column width and adjust scroll container padding so JSON entries fit comfortably.
- [x] Relocate “Theme snapshot” content into the Design tab; convert “Workspace overview” into a top-of-workspace banner with inline stats to free sidebar height.
- [x] Update snapshots/docs once layout polish is applied. *(Visual suite expanded to cover all tabs/resolutions; snapshots refreshed via `npm run test:visual:update` on 2025-11-12.)*

---

## 4. Dependencies & Questions
- **Upstream Impact:** None; layout-only adjustments.
- **Open Questions:** Preferred max width for the trace column? Should the global banner mirror the existing sidebar content or add new metrics?
- **Testing Gaps:** None — Playwright visual suite now captures every tab across 1440×900, 1080p, and 2K viewports.

---

## 5. Attachments & Links
- `docs/missing implementation/SI-20251111-01-simulation-instrumentation.md`
- `docs/Requirements/feature addition/Display_Web_UI_Workspace_Improvements.md`

---

## 6. Reviewer Notes
- Consider sharing layout tokens between tabs to keep future adjustments consistent (e.g., CSS variables for column widths). A top-level banner could also host global alerts/validation states beyond Simulation.
