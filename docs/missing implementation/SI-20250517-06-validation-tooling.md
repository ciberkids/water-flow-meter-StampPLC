# Story: SI-20250517-06-validation-tooling — Export Validation & Test Automation

> **Naming Convention**  
> Store active story specs under `docs/missing implementation/` using the identifier format above, for example `SI-20250517-06-validation-tooling.md`.  
> Reference the story from `docs/stories to implement/missing_implementation_stories.md` with the same ID so incremental progress stays traceable.

**Status:** pending  
**Author:** Matteo  
**Last Updated:** 2025-05-17  
**Linked Requirements:** docs/Requirements/feature addition/Display_Web_Mockup_and_Translator.md#36-requirement-conformance-validation  
**Related Features:** StampPLC UI redesign

---

## 1. Summary

Implement automated validation that runs after each export to ensure generated firmware assets respect documented behaviours, and provide clear feedback within the web UI.

---

## 2. Acceptance Criteria

- [ ] Post-export validation script checks for required UI elements (LED legend, countdown overlays, diagnostics banners).
- [ ] Web UI surfaces validation results with actionable messages and links to offending elements.
- [ ] Validation pipeline verifies backups exist for the previous export and that generated files compile.
- [ ] `npm run test:visual` completes without snapshot regressions.

---

## 3. Implementation Notes

- Combine static analysis of generated JSON/C++ with targeted unit tests (e.g., verifying component registration).
- Hook validation into the Node.js exporter and expose results via API/JSON to the React app.
- Ensure failure states prevent overwriting firmware assets unless user explicitly overrides.

---

## 4. Tasks & Checklist

- [ ] Implement validation scripts (Node.js) covering structural checks and presence of critical UI elements.
- [ ] Add compilation smoke test (PlatformIO or mocked build) to exporter pipeline.
- [ ] Update web UI to display validation summaries and allow download of detailed logs.
- [ ] Provide documentation on interpreting validation warnings/errors.

---

## 5. Verification Plan

1. Unit tests for validation rules with known-good and known-bad fixtures.
2. Integration tests simulating export flows, ensuring failures block firmware overwrite.
3. Manual review of UI messaging for clarity and accessibility.

---

## 6. Backout / Fallback Strategy

- Allow users to bypass automated checks only after explicit confirmation and backup creation.
- Maintain previous validation scripts to restore if new rules prove too restrictive.

---

## 7. Notes & Open Questions

- Future enhancement: integrate hardware-in-the-loop regression once mockups are validated virtually.
- Consider adding Git hooks or CI jobs to run validation on pull requests automatically.
