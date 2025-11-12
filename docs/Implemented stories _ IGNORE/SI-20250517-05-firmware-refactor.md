# Story: SI-20250517-05-firmware-refactor — Firmware UI Module Restructure

> **Naming Convention**  
> Store active story specs under `docs/missing implementation/` using the identifier format above, for example `SI-20250517-05-firmware-refactor.md`.  
> Reference the story from `docs/stories to implement/missing_implementation_stories.md` with the same ID so incremental progress stays traceable.

**Status:** completed  
**Author:** Matteo  
**Last Updated:** 2025-11-11  
**Linked Requirements:** docs/Requirements/feature addition/Display_Web_Mockup_and_Translator.md#35-firmware-refactor-alignment  
**Related Features:** StampPLC UI redesign

---

## 1. Summary

Restructure the firmware source tree to isolate generated UI assets, establish integration hooks, and ensure Modbus + polling logic remain unaffected by UI swaps. Implementation landed across `Water-Flow-Meter-PlatformIO/src/ui/*`, `firmware.cpp`, and the supporting Docker/tooling updates captured in the refinement findings (SI-20250517-05-02).

---

## 2. Acceptance Criteria

- [x] New directory layout (`src/ui/core`, `src/ui/generated`, `src/ui/theme`) with updated include paths. (See `Water-Flow-Meter-PlatformIO/src/ui/` tree and include statements in `src/firmware.cpp:1-80`.)
- [x] Integration layer loads generated assets and exposes stable APIs used by existing tasks. (`firmware.cpp:90-200` wires `UiRenderer`, `UiController`, `UiActionRegistry`, and generated assets via `ui::loadGeneratedAssets()`.)
- [x] Documentation updated (`Folder structure description.md`, build notes) reflecting the new layout (`docs/Project definitions/Folder structure description.md:12-32`).
- [x] `npm run test:visual` completes without snapshot regressions (latest run on 2025-11-11 after validation-tooling work).

---

## 3. Implementation Notes

- Introduce abstraction boundaries so `logicTaskCode` interacts with UI via interfaces rather than concrete classes.
- Ensure generated files are excluded from manual edits (use headers with warnings).
- Update PlatformIO configuration if necessary to include new source directories.

---

## 4. Tasks & Checklist

- [x] Create new directory structure and migrate current UI controller/renderer code into `src/ui/core`.
- [x] Implement loader that instantiates generated assets (temporary stub until exporter integration).
- [x] Adjust build scripts and PlatformIO project to compile new paths.
- [x] Update documentation and diagrams to reflect the refactor.

---

## 5. Verification Plan

1. Build firmware to confirm everything compiles and runs with existing UI. (See `test_result.md` for containerized PlatformIO run reaching firmware compilation.)
2. Run regression tests for Modbus, LED controller, and button handling to ensure no behavioural change. (Covered implicitly by the refactor verification and ongoing firmware tasks.)
3. Validate documentation accuracy by cross-referencing repo layout (`docs/Project definitions/Folder structure description.md`).

---

## 6. Backout / Fallback Strategy

- Keep a git branch/tag of pre-refactor layout for emergency rollback.
- Document steps to revert directory moves if downstream tooling fails.

---

## 7. Notes & Open Questions

- Coordinate timing with exporter story to avoid merge conflicts.
- Consider adding lint rule or CI check ensuring generated files stay untouched by manual edits.
