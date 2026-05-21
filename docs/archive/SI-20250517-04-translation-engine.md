# Story: SI-20250517-04-translation-engine — Node.js Export & JSON Schema

> **Naming Convention**  
> Store active story specs under `docs/missing implementation/` using the identifier format above, for example `SI-20250517-04-translation-engine.md`.  
> Reference the story from `docs/stories to implement/missing_implementation_stories.md` with the same ID so incremental progress stays traceable.

**Status:** completed  
**Author:** Matteo  
**Last Updated:** 2025-11-05  
**Linked Requirements:** docs/Requirements/feature addition/Display_Web_Mockup_and_Translator.md#34-translation-engine-export  
**Related Features:** StampPLC UI redesign

---

## 1. Summary

Build the Node.js translation engine that consumes the mockup JSON + theme descriptors and emits optimized C++ drawing code, with pre-export validation.

---

## 2. Acceptance Criteria

- [x] Defined JSON schema covering screens, layers, animations, and theme tokens with automated validation (shared Ajv schemas consumed by exporter and React workspace).
- [x] CLI command (`npm run export:firmware`) produces C++ assets under `src/ui/generated` (also invokable via mockup “Export to Firmware” control).
- [x] Export process backs up prior generated assets into `backups/ui/<timestamp>/` before writing new files.
- [x] `npm run test:visual` completes without snapshot regressions (includes IR-vs-UI parity assertion).

---

## 3. Implementation Notes

- Use TypeScript for the Node.js exporter to share types with the React app.
- Consider generating intermediate IR before rendering final C++ for easier testing.
- Optimize emitted code paths (batch drawing operations, reuse constants) to meet firmware performance constraints.

---

## 4. Tasks & Checklist

- [x] Define JSON schema (Ajv) and validation pipeline.
- [x] Implement Node.js CLI that loads mockup data, validates, and generates IR.
- [x] Write C++ emitter producing header/source files compatible with existing `UiRenderer`.
- [x] Add backup routine with timestamped directories and compression if needed.
- [x] Document exporter usage and integration steps in `docs/`.

---

## 5. Verification Plan

1. Unit tests for schema validation and IR generation. ✅ (`npm run test:exporter`)
2. Snapshot tests comparing emitted C++ against golden files. ✅ (`npm run test:visual` — now also verifies IR parity)
3. Run firmware build (PlatformIO) using generated assets to ensure compilation succeeds. ☐

### Regression Evidence (2025-11-05)
- `npm run test:visual` → pass (Playwright + IR parity check).  
- `npm run test:cypress` → pass (Cypress e2e verifying exporter alignment).
- `npm run export:firmware` → pass (backup created: see `backups/ui/20251105_105928`).

---

## 6. Backout / Fallback Strategy

- Restore previous generated assets from the timestamped backup folder.
- Keep manual C++ UI assets available until the exporter is proven stable.

---

## 7. Notes & Open Questions

- Investigate leveraging WASM (e.g., Rust) if further performance tuning becomes necessary.
- Plan for incremental export (only changed screens) in future iterations.
- CLI entry point lives in `web/mockup/tools/exporter/cli.ts`; `npm run export:firmware` now rebuilds the translator, validates JSON, writes IR snapshots (`ui_export_ir.json`), and regenerates `src/ui/generated/GeneratedUi.*`.
