# Story: SI-20251111-03-exporter-schema-manifest — Firmware Manifest & Exporter Enhancements

> **Naming convention.** Story specs live in `docs/backlog/` using the identifier format above. The
> two directories this note used to name — `docs/missing implementation/` and `docs/stories to
> implement/` — were renamed to `docs/backlog/` and `docs/active_work/`, and there is no
> `missing_implementation_stories.md` index: `docs/active_work/open_decisions.md` is what gets read.

**Status:** mostly delivered — see *Where this actually stands* below (updated 2026-08-12)  
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

---

## Where this actually stands (2026-08-12)

Checked against the code, not against memory. Most of this story shipped under other commits, which is
why every checkbox above is still empty — nobody came back to tick them.

**Delivered:**

- The C++ to JSON generator exists: `Water-Flow-Meter-PlatformIO/tools/manifest_gen` emits
  `actionManifest.json` from the firmware catalogues, and CI fails if the committed manifest drifts
  from a fresh generation.
- Ajv validation of both the dataset and the manifest, in `web/mockup/src/schema/`.
- The exporter emits IR and C++ for value placeholders, scrollbars and hierarchical screens, and
  **fails** on a binding that names a value or action the firmware does not have —
  `manifest-value-coverage` and `manifest-action-coverage` in `tools/exporter/validation.ts`.
- The export panel reports manifest, validation and compile status, including the PlatformIO compile
  check (`platformio-compile`).

**Not delivered, and two of the three no longer should be:**

- *Schemas in `shared/`, versioned.* They live in `web/mockup/src/schema/` and are not versioned.
  Moving them buys nothing today — the firmware does not read them — so this is a real gap only when
  a second consumer appears. See **N-b** in `../active_work/open_decisions.md`, which is the same
  question wearing a different hat.
- *Animation assets.* Dropped by decision C1: the scrollbar shipped and animation stays authorable
  but export-blocked. Not pending — decided against.
- *Backward compatibility via schema versioning and migration utilities.* This is exactly **N-b**.
  Tracked there, not here.

**What is genuinely left:** nothing this story should own. The residue is N-b.
