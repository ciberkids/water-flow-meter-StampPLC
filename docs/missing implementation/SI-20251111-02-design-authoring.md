# Story: SI-20251111-02-design-authoring — Design Tab Authoring & Hierarchy Tools

> **Naming Convention**  
> Store active story specs under `docs/missing implementation/` using the identifier format above, for example `SI-20250517-01-led-diagnostics.md`.  
> Reference the story from `docs/stories to implement/missing_implementation_stories.md` with the same ID so incremental progress stays traceable.

**Status:** completed  
**Author:** Matteo  
**Last Updated:** 2025-11-11  
**Linked Requirements:** docs/Requirements/feature addition/Display_Web_UI_Workspace_Improvements.md#32-design-editor-authoring  
**Related Features:** Display Web UI workspace

---

## 1. Summary

Transform the Design tab into a full layout editor with element insertion (text, box, value placeholder, animation box, scroll bar), screen CRUD, nested submenu hierarchy, event binding UI, and a live JSON editor with lint-style validation. Designers should navigate screen trees, drag-drop elements, and immediately see JSON updates/errors.

---

## 2. Acceptance Criteria

- [x] Toolbox allows inserting/deleting text, box, value placeholder, animation box, scroll bar, and SVG asset references.
- [x] Screen manager supports add/remove/duplicate plus a hierarchical tree view that reflects menu/submenu nesting; breadcrumbs update when entering submenus.
- [x] Event editor lists available events per screen and lets users bind them to firmware functions from the manifest.
- [x] Live JSON editor stays synchronized bidirectionally; syntax/validation errors are highlighted inline with hints.
- [x] Animation inspector lets designers upload/select SVG frames, order them, and preview sequences.

---

## 3. Implementation Notes

- Introduce a tree data model for screens (with IDs referencing parent/child). Update JSON schema accordingly (see SI-20251111-03).
- Build a reusable element palette component and align styling with existing UI conventions. ✅ Implemented via `DesignToolbox` with inline editing/removal plus support for animation/scrollbar/svg reference elements.
- Integrate Monaco (or similar) JSON editor with schema-backed validation to provide inline diagnostics. ✅ Implemented with lightweight live JSON editor that validates against the shared schema and surfaces inline status messages.
- Scroll bar widget should auto-compute numbering per clarified requirement (e.g., `5-2`). ✅ Screen hierarchy now derives path codes and the simulator renders scroll-bars with those computed indices.

---

## 4. Tasks & Checklist

- [x] Implement screen hierarchy data model + tree view UI with move/duplicate/delete affordances.
- [x] Extend dataset schema handling to support nested screen collections, scroll bar metadata, and animation assets.
- [x] Build element insertion toolbox + inspector panels for properties.
- [x] Create event binding UI tied to firmware manifest selection.
- [x] Wire live JSON editor with validation + error highlighting.
- [x] Update Playwright tests for element insertion, hierarchy navigation, animation uploads, and JSON editing.

---

## 5. Verification Plan

1. Playwright coverage for inserting each element type and ensuring dataset JSON updates.
2. Manual test creating submenus and confirming tree view breadcrumbs + scroll bar numbering.
3. JSON editor negative tests ensuring malformed input blocks export and surfaces hints.

---

## 6. Backout / Fallback Strategy

- Maintain ability to collapse the tree view and revert to the previous flat list if unexpected regressions occur.
- Provide dataset backup/export actions so users can restore earlier JSON if the live editor misbehaves.

---

## 7. Notes & Open Questions

- Depends on new schema definitions, manifest availability, and asset storage pipeline (story SI-20251111-03).
- Coordinate with Simulation story to ensure event bindings/function names stay consistent.
