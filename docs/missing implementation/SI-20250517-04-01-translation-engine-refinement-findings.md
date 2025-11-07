# Story Refinement Findings — SI-20250517-04-translation-engine

**Story ID:** SI-20250517-04  
**Story Title:** Node.js translation engine & JSON schema  
**Implementation Artifact:** `web/mockup/tools/exporter` • `src/ui/generated`  
**Reviewer:** Codex  
**Review Date:** 2025-11-05  
**Status Recommendation:** needs refinement

---

## 1. Snapshot Summary
- **Observed Outcome:** CLI exporter validates JSON and emits C++ artifacts, but the web authoring surface lacks parity features or QA hooks to ensure UI ⇄ firmware alignment.
- **Intended Outcome:** Designers trigger firmware exports from the mockup, share a single JSON definition between viewer and exporter, and maintain one-to-one capability mapping validated by automated tests.
- **Gap Statement:** Without UI controls, schema cross-checks, or regression tests, the translation pipeline remains manual and risks diverging from the visualization that designers rely upon.

---

## 2. Findings Log

| # | Area / File | Severity* | Finding | Evidence (line / behaviour) | Proposed Refinement |
|---|-------------|-----------|---------|------------------------------|---------------------|
| 2.1 | `web/mockup/src/App.tsx:1` | High | Web workspace offers no “Export to Firmware” action; the CLI can only be run manually, blocking designer-triggered exports promised in the requirement. | Sidebar and header render screen selectors/editor controls only; no button or handler exists to call the exporter. | Add an export control wired to an API/IPC bridge that invokes the Node.js translator and surfaces status + backup links. |
| 2.2 | `web/mockup/src/data/screens.json:1` vs. `web/mockup/tools/exporter/schema.ts:1` | High | Story references layers/animations/theme tokens, yet the viewer renders only `elements`; no automated check confirms the schema powering Ajv matches what the React UI consumes. | Viewer imports JSON directly without validation, while the schema allows flows/assets/animations not visualized. | Share a single schema contract (runtime validation in the UI) and add parity tests to guarantee any dataset served to designers is translator-valid. |
| 2.3 | `web/mockup/tools/exporter/cppEmitter.ts:1` | High | IR → C++ emitter drops `flows`, `animations`, `assets`, and `submenus` even though JSON supports them, preventing “exact mapping” from mockup to firmware. | Emitted `Screen` struct exposes `elements` only; other collections are ignored. | Extend the generated model and C++ structs to serialise all declared capabilities, then update firmware integration to consume them. |
| 2.4 | `web/mockup/tests/visual/mockup.spec.ts:1` | Medium | Existing Playwright suite covers only UI behaviour; no tests (Playwright or Cypress) assert exporter output matches the React rendering. | Snapshot specs never invoke translation nor diff generated C++/IR. | Add cross-check tests that run the exporter, parse `GeneratedUi.*`, and compare positions/content against the rendered DOM (Playwright for UI, Cypress or Node-based tests for file parity). |
| 2.5 | `docs/missing implementation/SI-20250517-04-translation-engine.md:1` | Medium | Regression documentation for translation exports has not been started despite requirement; no baseline artefacts or reports exist to review output changes. | Story doc lacks a regression log; only acceptance boxes were ticked. | Create an export regression journal (attach IR snapshots, metadata diffs, and Playwright evidence) and update the documentation checklist to keep it current after each run. |

> *Severity guidance: **High** (blocks requirement / creates defect), **Medium** (partial compliance / risky), **Low** (polish / documentation).

---

## 3. Suggested Refinement Tasks
- [x] Surface an “Export to Firmware” control in the mockup UI with progress + backup feedback.
- [x] Introduce shared runtime schema validation so the viewer refuses datasets that the exporter would reject.
- [x] Expand the IR/C++ emitter and firmware model to cover flows, animations, assets, and submenus.
- [x] Implement Playwright + Cypress parity tests that compare rendered layouts with generated C++ metadata.
- [x] Author and maintain an export regression log (linking to snapshots, IR diffs, and test runs).

---

## 4. Dependencies & Questions
- **Upstream Impact:** Firmware integration story SI-20250517-05 depends on richer generated structs; blocking until parity is achieved.
- **Open Questions:** Do we expose the exporter via local API or command-bridge from Vite? Which backup retention policy do we adopt when triggering exports from the browser?
- **Testing Gaps:** No end-to-end comparison between React-rendered positions and firmware coordinates; lack of Cypress coverage for UI-export workflow.

---

## 5. Attachments & Links
- `web/mockup/tools/exporter/cli.ts`
- `src/ui/generated/GeneratedUi.h`
- `docs/missing implementation/SI-20250517-04-translation-engine.md`

---

## 6. Reviewer Notes
- Consider reusing the Ajv schema inside the browser (via Web Worker) so designers see validation errors immediately.
- Explore exposing generated IR to the mockup for live-on-screen diffing between React layout and firmware coordinates.

## 7. Resolution Summary (2025-11-05)
- Export trigger and status surface live inside the mockup, delegating to a Vite API that builds and runs the translator while tracking backups.
- Shared Ajv schemas now gate both exporter and UI; validation failures render inline banners in the workspace.
- Generated C++ assets now include flows, animations, graphic assets, and submenus to preserve the JSON capabilities.
- Playwright parity and Cypress e2e tests compare the React dataset with `ui_export_ir.json`, guarding UI ⇄ firmware drift.
- Regression log captures the latest exporter/test executions within the story document.
