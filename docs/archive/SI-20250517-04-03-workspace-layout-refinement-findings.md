# Story Refinement Findings — SI-20250517-04 workspace layout

**Story ID:** SI-20250517-04  
**Story Title:** Node.js translation engine & JSON schema  
**Implementation Artifact:** `web/mockup/src/App.tsx` • `web/mockup/src/App.css`  
**Reviewer:** Codex  
**Review Date:** 2025-11-05  
**Status Recommendation:** ready for verification

---

## 1. Snapshot Summary
- **Observed Outcome:** Core controls (dataset tools, JSON export, translator trigger, theme editor) are clustered in the main workspace sidebar, making the UI feel cramped and hard to navigate.
- **Intended Outcome:** Editing, exporting, and theming flows should be clearly separated so designers can focus on one task at a time.
- **Gap Statement:** Current layout splits attention across one page; UX would benefit from re-organising controls into focused sections/pages.

---

## 2. Findings Log

| # | Area / File | Severity* | Finding | Evidence (line / behaviour) | Proposed Refinement |
|---|-------------|-----------|---------|------------------------------|---------------------|
| 2.1 | `web/mockup/src/App.tsx` | Medium | “Data export” button lives in its own block, duplicating functionality already introduced in Dataset Tools. | Sidebar shows separate “Data export” section while Dataset Tools handles import/validation. | Move the JSON export button into the Dataset Tools card and remove the redundant block. |
| 2.2 | `web/mockup/src/App.tsx` | Medium | Export-to-firmware flow shares the main workspace panel, mixing editing with translation tasks. | Exporter panel and editing components appear on the same tab. | Create a dedicated Export page/tab (similar to Help) to isolate translator actions and backup history. |
| 2.3 | `web/mockup/src/App.tsx` / `.css` | Medium | Theme/typography editor occupies prime space in the main workspace, contributing to clutter. | Theme editor card sits between dataset controls and viewport. | Relocate theme/typography adjustments to their own page to keep design editing focused and spacious. |

> *Severity guidance: **High** (blocks requirement / creates defect), **Medium** (partial compliance / risky), **Low** (polish / documentation).

---

## 3. Suggested Refinement Tasks
- [x] Consolidate JSON import/export controls inside the Dataset Tools card and remove the standalone export block.
- [x] Introduce an “Export” view/tab that houses the translator button, backup summary, and related messaging.
- [x] Split theme/typography editing into its own view so the workspace focuses on layout editing and previewing.

---

## 4. Dependencies & Questions
- **Upstream Impact:** None beyond UI routing adjustments; ensure Playwright/Cypress tests reflect new navigation.
- **Open Questions:** Should the Export view also surface regression logs and backup restore actions? Clarify navigation labels for the additional tab(s).
- **Testing Gaps:** Add coverage for navigating between the new tabs/pages once implemented.

---

## 5. Attachments & Links
- `web/mockup/src/App.tsx`
- `web/mockup/src/App.css`
- `web/mockup/tests/visual/mockup.spec.ts`

---

## 6. Reviewer Notes
Streamlining the workspace into focused tabs (Edit / Theme / Help / Export) will keep the mockup usable as features grow.

## 7. Resolution Summary (2025-11-05)
- Dataset Tools card now handles import, validate, and JSON export actions; the redundant workspace export block is gone.
- Workspace navigation exposes dedicated tabs for Workspace, Theme, Export, and Help & JSON, with ThemeEditor and ExporterPanel relocated accordingly.
- Export view surfaces validation status, dataset summary, and the firmware exporter, while Theme view houses the editor in a spacious layout.
- Playwright snapshots were refreshed and tests updated to navigate between tabs; Cypress exporter parity continues to pass.
