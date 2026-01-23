# Story: SI-20250517-06-validation-tooling — Export Validation & Test Automation

> **Naming Convention**  
> Store active story specs under `docs/missing implementation/` using the identifier format above, for example `SI-20250517-06-validation-tooling.md`.  
> Reference the story from `docs/stories to implement/missing_implementation_stories.md` with the same ID so incremental progress stays traceable.

**Status:** completed  
**Author:** Matteo  
**Last Updated:** 2025-11-11  
**Linked Requirements:** docs/Requirements/feature addition/Display_Web_Mockup_and_Translator.md#36-requirement-conformance-validation  
**Related Features:** StampPLC UI redesign

---

## 1. Summary

Implement automated validation that runs after each export to ensure generated firmware assets respect documented behaviours, capture backups, run a firmware compile smoke-test, and surface the entire report inside the mockup workspace.

---

## 2. Acceptance Criteria

- [x] Post-export validation script checks for required UI elements (LED legend, countdown overlays, diagnostics banners).
- [x] Web UI surfaces validation results with actionable messages and links to offending elements.
- [x] Validation pipeline verifies backups exist for the previous export and that generated files compile (or fail safely with restored backups).
- [x] `npm run test:visual` completes without snapshot regressions.

---

## 3. Implementation Notes

- Added a dedicated validator (`web/mockup/tools/exporter/validation.ts`) that inspects the dataset/IR for mandatory bindings (LED legend, countdown overlay, diagnostics banner). The validator produces structured `ValidationReport` objects consumed by the CLI and UI.
- The exporter CLI (`tools/exporter/cli.ts`) now orchestrates: dataset/theme validation, backup creation, IR emission, the new UI validation report, a PlatformIO compile check (with log capture), and safe restoration of previous exports on failure.
- Vite's `/api/export` endpoint simply returns the richer CLI payload so the React UI can mirror the automation state. Failure codes distinguish schema errors, validation failures, and automation (compile) failures.
- React `ExporterPanel` renders validation checklists, navigation shortcuts that jump to the relevant screen, backup summaries, and log-download buttons so designers can interpret warnings/errors without leaving the workspace.
- Added unit tests for the validator plus updated Playwright snapshots to include the new UI affordances; `npm run test:visual` now exercises the full export pipeline end-to-end.

---

## 4. Tasks & Checklist

- [x] Implement validation scripts (Node.js) covering structural checks and presence of critical UI elements.
- [x] Add compilation smoke test (PlatformIO) to exporter pipeline (with graceful warning when PlatformIO CLI is absent).
- [x] Update web UI to display validation summaries, navigation shortcuts, and downloadable logs.
- [x] Provide documentation on interpreting validation warnings/errors (see Section 7 and in-app messaging/log downloads).

---

## 5. Verification Plan

1. `npm run test:exporter` — exercises the CLI in node:test, covering schema validation plus the new validator unit tests.
2. `npm run test:visual` — runs the Vite build, executes Playwright against the exporter endpoint, and asserts the automated export payload (including validation + backup summaries) is returned.
3. Manual review via the Import & Export tab to ensure validation statuses, log downloads, and “View screen” shortcuts behave as documented.

---

## 6. Backout / Fallback Strategy

- Allow users to bypass automated checks only after explicit confirmation and backup creation (current CLI refuses to overwrite assets when validation fails and restores from backup after compile errors).
- Maintain previous validation scripts to restore if new rules prove too restrictive.

---

## 7. Notes & Open Questions

- Help panel + story doc document how to interpret validation statuses: `Pass` (safe to export), `Needs attention` (warnings such as missing PlatformIO CLI), `Blocked` (validation/compile failures). Users can download the textual validator log and PlatformIO output directly from the Exporter panel.
- Future enhancement: integrate hardware-in-the-loop regression once mockups are validated virtually.
- Consider adding Git hooks or CI jobs to run validation on pull requests automatically.
