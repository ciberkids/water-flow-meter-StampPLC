# Story Refinement Findings — SI-20250517-04 translation UI workflow

**Story ID:** SI-20250517-04  
**Story Title:** Node.js translation engine & JSON schema  
**Implementation Artifact:** `web/mockup/src` (UI workflow) • `web/mockup/tools/exporter`  
**Reviewer:** Codex  
**Review Date:** 2025-11-05  
**Status Recommendation:** ready for verification

---

## 1. Snapshot Summary
- **Observed Outcome:** The mockup UI renders screens but lacks the data-entry and validation workflow needed to drive the translator from the browser.
- **Intended Outcome:** Designers should load JSON, validate it, preview the result, and export firmware-ready C++ without leaving the app.
- **Gap Statement:** Missing import, validation, documentation alignment, and translation controls make the UI an incomplete substitute for firmware preview work.

---

## 2. Findings Log

| # | Area / File | Severity* | Finding | Evidence (line / behaviour) | Proposed Refinement |
|---|-------------|-----------|---------|------------------------------|---------------------|
| 2.1 | `web/mockup/src/App.tsx` | High | No control allows users to import a JSON dataset into the workspace; only the bundled dataset is available. | UI shows static dataset loaded via module import; no upload/input widget present. | Add an import button/modal that accepts JSON, validates it, and updates the dataset state. |
| 2.2 | `web/mockup/src/App.tsx` / `schema/validation.ts` | High | Missing dedicated “Validate JSON” action so designers can check datasets pre-export. Validation only happens implicitly on load. | Schema banner appears only when dataset fails at startup; no user-triggered revalidation. | Provide a validation button that runs Ajv over the currently loaded dataset and surfaces results inline. |
| 2.3 | `web/mockup/src/components/HelpPanel.tsx` | Medium | Help panel examples don’t mirror the canonical schema; documentation drifts from actual JSON expectations. | Sample JSON omits flows/assets/animations introduced in schema; mismatch noted in requirements. | Update help documentation/examples to reflect the shared schema (elements, flows, assets, animations, submenus). |
| 2.4 | `web/mockup/src` (UI workflow) | High | Mockup must serve as a faithful microcontroller proxy, yet missing tooling (import/validate/export buttons) limits usefulness. | UI only visualises static data; designers cannot iterate quickly from within the app. | Implement a streamlined workflow (import → validate → preview → export) to make the mockup practically useful. |
| 2.5 | `web/mockup/src/components/ExporterPanel.tsx` | High | Exporter panel lacks a trigger to run the translator from the UI; designers must rely on CLI. | Panel currently only displays summary text; no button to invoke `/api/export`. | Add a visible “Export to Firmware” action that calls the existing endpoint and reports status/backups. |

> *Severity guidance: **High** (blocks requirement / creates defect), **Medium** (partial compliance / risky), **Low** (polish / documentation).

---

## 3. Suggested Refinement Tasks
- [x] Implement JSON import/upload flow with schema validation and error handling.
- [x] Add a manual “Validate Dataset” button that runs Ajv against the active dataset and surfaces issues.
- [x] Align Help panel documentation and examples with the canonical schema (including flows, assets, animations, submenus).
- [x] Enhance Exporter panel with an actionable translator trigger and status reporting.
- [x] Add integration tests (Playwright/Cypress) covering import, validation, and export workflows to ensure usefulness.

---

## 4. Dependencies & Questions
- **Upstream Impact:** Export tooling relies on the shared schema; UI enhancements must continue using the canonical definitions.
- **Open Questions:** Should imports support drag-and-drop and multiple datasets? Define how backups/versions are surfaced after export.
- **Testing Gaps:** No automated coverage for import/validate/export flows; add tests once controls exist.

---

## 5. Attachments & Links
- `web/mockup/src/App.tsx`
- `web/mockup/src/components/HelpPanel.tsx`
- `web/mockup/tools/exporter/cli.ts`

---

## 6. Reviewer Notes
Ensure the browser tooling stays aligned with firmware needs—designers must be able to iterate without leaving the UI or relying on shell commands.

## 7. Resolution Summary (2025-11-05)
- Dataset sidebar now supports importing arbitrary JSON files, rerunning schema validation on demand, and surfacing inline feedback for issues.
- Export trigger and status reside in the mockup sidebar, delegating to the Vite API that builds and runs the translator while tracking backups.
- Help documentation mirrors the canonical schema (elements, flows, assets, animations, submenus) so authors stay aligned with the exporter.
- Playwright import/validation regression test and Cypress exporter parity guard the end-to-end workflow.
