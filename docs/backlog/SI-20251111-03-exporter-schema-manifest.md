# Story: SI-20251111-03-exporter-schema-manifest — Firmware Manifest & Exporter Enhancements

> **Naming Convention**  
> Store active story specs under `docs/missing implementation/` using the identifier format above, for example `SI-20250517-01-led-diagnostics.md`.  
> Reference the story from `docs/stories to implement/missing_implementation_stories.md` with the same ID so incremental progress stays traceable.

**Status:** pending  
**Author:** Matteo  
**Last Updated:** 2025-11-11  
**Linked Requirements:** docs/Requirements/feature addition/Display_Web_UI_Workspace_Improvements.md#33-import--export-enhancements  
**Related Features:** Display Web UI workspace

---

## 1. Summary

Extend the importer/exporter pipeline to support the richer UI schema: ingest firmware function manifests generated from C++, validate datasets against the new schema (value placeholders, scroll bars, animations, events), and emit IR/C++ that reflects the added constructs. The Export panel must confirm manifest status, show validation, and ensure backups remain intact.

---

## 2. Acceptance Criteria

- [ ] Canonical JSON schema for firmware actions + UI dataset stored in `shared/` and versioned.
- [ ] C++ → JSON generator produces the firmware manifest automatically, matching the schema and exposing all callable actions.
- [ ] Import step validates manifest files and makes function list available to Simulation/Design tabs.
- [ ] Exporter emits IR/C++ with new constructs (value placeholders, scroll bars, animation assets, hierarchical screens) and fails with clear messaging if bindings reference missing functions.
- [ ] Export panel surfaces manifest/validation/compile statuses, tying into existing automation checks.

---

## 3. Implementation Notes

- Schema updates must align with existing Ajv validation pipeline and Playwright tests.
- Manifest generator likely lives in firmware repo; coordinate with spike SI-20251111-05 before implementation.
- Exported assets should include SVG references and any additional metadata required to render animations.
- Ensure backward compatibility via schema versioning and migration utilities.

---

## 4. Tasks & Checklist

- [ ] Define/update JSON schemas for dataset + firmware manifest; add documentation.
- [ ] Build/extend Node exporter to consume the firmware manifest and validate event bindings.
- [ ] Implement import UI for manifest upload + status messaging in Import/Export tab.
- [ ] Update C++ build tooling to emit manifest JSON during firmware builds (hook into PlatformIO or dedicated script).
- [ ] Expand exporter unit tests to cover new schema constructs and failure cases.
- [ ] Update Playwright exporter test to verify manifest-dependent actions.

---

## 5. Verification Plan

1. `npm run test:exporter` extended with fixtures covering new schema features.
2. Manual export run with/without manifest to confirm error paths.
3. Firmware manifest generation test ensuring JSON matches schema (CI hook).

---

## 6. Backout / Fallback Strategy

- Keep previous schema definition available; allow exporter to run in compatibility mode if manifest is missing (with warnings) until firmware side is ready.
- Gate manifest-dependent UI behind a feature toggle so existing datasets continue to load.

---

## 7. Notes & Open Questions

- Implementation depends on spike SI-20251111-05 to pick the C++ design pattern for exposing actions.
- Coordinate with documentation story to keep Help content accurate.
