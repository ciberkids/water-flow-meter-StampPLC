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

---

# Refinement Findings #1 (Legacy SI-20251111-02-01)
**Review Date:** 2025-11-12 | **Status:** resolved

## Snapshot Summary
- **Observed:** Design toolbox exposes width/height, enforces bounds, and supports selection/nudge.
- **Intended:** Practical geometry editing with bounded inputs and arrow keys.
- **Gap:** ADDRESSED.

## Findings Log

| # | Area / File | Severity | Finding | Proposed Refinement |
|---|-------------|----------|---------|---------------------|
| 2.1 | `DesignToolbox.tsx` | Medium | Box elements missing width/height editing. | ✅ Width/height inputs now render for geometric elements. |
| 2.2 | `DesignToolbox.tsx` | Medium | Inputs accept long strings, exceeding display. | ✅ Inputs clamped to 4 digits and screen max. |
| 2.3 | `DesignToolbox.tsx` | Medium | No element selection affordance. | ✅ Radio buttons added to select active element. |
| 2.4 | `App.tsx` | Medium | No quick key/button nudge. | ✅ Added directional buttons + keyboard arrows. |

## Suggested Refinement Tasks
- [x] Add width/height controls (with persistence + validation).
- [x] Enforce 4-character max + min/max range.
- [x] Implement element selection via radio buttons.
- [x] Add arrow button cluster + keyboard bindings.

---

# Refinement Findings #2 (Legacy SI-20251111-02-02)
**Review Date:** 2025-11-13 | **Status:** needs refinement

## Snapshot Summary
- **Observed:** Arrow keys do single nudge (no repeat); out-of-bounds elements render without correction.
- **Intended:** Continuous press-and-hold nudge; automatic guardrails for bounds.
- **Gap:** Nudging requires repeated taps; invalid geometry persists.

## Findings Log

| # | Area / File | Severity | Finding | Proposed Refinement |
|---|-------------|----------|---------|---------------------|
| 2.1 | `App.tsx` | Medium | Keydown handler rejects repeats. | Let keyboard repeat drive the nudge loop. |
| 2.2 | `layout.ts`, `DisplayViewport.tsx` | High | Layout tags overflow but doesn't clamp. | Normalize coordinates on ingest/render and highlight corrections. |

## Suggested Refinement Tasks
- [x] Enable continuous keyboard nudging.
- [x] Clamp X/Y/width/height on dataset ingest and layout render.
- [x] Add regressions tests for holding keys and importing out-of-bounds data.

---

# Refinement Findings #3 (Legacy SI-20251111-02-03)
**Review Date:** 2025-11-13 | **Status:** resolved

## Snapshot Summary
- **Observed:** Boxes spill bounds; arrow buttons misplaced; keyboard arrows inverted in landscape; duplicate JSON panels.
- **Intended:** Immediate in-bounds mockup; consistent arrow controls; single JSON editor.
- **Gap:** Ergonomics regress; split attention on JSON.

## Findings Log

| # | Area / File | Severity | Finding | Proposed Refinement |
|---|-------------|----------|---------|---------------------|
| 2.1 | `App.tsx`, `DesignToolbox.tsx` | High | Initial overflow with no fix action. | per-element actions. |
| 2.2 | `App.tsx`, `layout.ts` | High | Arrow keys feel inverted in landscape. | Transform arrow deltas through orientation matrix. |
| 2.3 | `DesignToolbox.tsx`, `App.css` | Medium | Arrow pad linear strip instead of cluster. | Restructure to 3x3 grid embedded in toolbox. |
| 2.4 | `App.tsx` | Low | Duplicate read-only JSON. | Remove read-only preview. |

## Suggested Refinement Tasks
- [x] Add global "Clamp all" and per-element clamp.
- [x] Align keyboard/UI arrows with logical orientation.
- [x] Remove duplicate JSON preview.

---

# Refinement Findings #4 (Legacy SI-20251111-02-04)
**Review Date:** 2025-11-13 | **Status:** resolved

## Snapshot Summary
- **Observed:** Origin is top-left; clamp buttons no-op; missing triggers; long inputs; misplaced arrows; animation flicker.
- **Intended:** Lower-left origin; working clamps; full trigger list; stable UI.
- **Gap:** Broken affordances and bugs.

## Findings Log

| # | Area / File | Severity | Finding | Proposed Refinement |
|---|-------------|----------|---------|---------------------|
| 2.1 | `layout.ts` | Medium | Origin is top-left. | Flip Y-axis to match physical requirement (lower-left). |
| 2.2 | `DesignToolbox.tsx` | High | Clamp buttons don't mutate data. | Fix handler to persist clamped values. |
| 2.3 | `EventBindingPanel.tsx` | Medium | Missing required triggers. | Replace with full list from requirements. |
| 2.4 | `DesignToolbox.tsx` | Low | Inputs too wide. | Resize inputs to match digit count. |
| 2.5 | `App.tsx` | Medium | Arrow pad far from viewport. | Move pad beneath viewport. |
| 2.6 | `App.tsx` | Low | Button event log in Design tab. | Restrict to Simulation tab. |
| 2.7 | `AnimationInspector.tsx` | High | Animation input flicker. | Debounce/control input state. |

## Suggested Refinement Tasks
- [x] Re-map viewport to lower-left origin.
- [x] Fix clamp button mutation.
- [x] Update trigger picker list.
- [x] Resize inputs and relocate arrows.
- [x] Fix animation input flicker.

---

# Refinement Findings #5 (Legacy SI-20251111-02-05)
**Review Date:** 2025-11-13 | **Status:** resolved

## Snapshot Summary
- **Observed:** `x=0` gap; easing flicker; clamps still no-op; wrong triggers; keyboard still inverted.
- **Intended:** Pixel-perfect origin; stable controls; working clamps.
- **Gap:** Persistent regressions.

## Findings Log

| # | Area / File | Severity | Finding | Proposed Refinement |
|---|-------------|----------|---------|---------------------|
| 2.1 | `layout.ts` | High | `x=0` offset/gap. | Enforce true min/max and zero-margin. |
| 2.2 | `ThemeEditor.tsx` | Medium | Easing dropdown flicker. | Manage as controlled input. |
| 2.3 | `DesignToolbox.tsx` | High | Clamps still don't persist. | Wire to global helper + re-render. |
| 2.4 | `EventBindingPanel.tsx` | High | Wrong button list. | Restrict to up/down/enter + short/long. |
| 2.5 | `App.tsx` | Medium | Keyboard arrows X/Y swap. | Fix orientation mapping. |

## Suggested Refinement Tasks
- [x] Clamp coordinates strictly to `[0, display]`.
- [x] Stabilize easing dropdown.
- [x] Fix clamp persistence.
- [x] Correct trigger list.
- [x] Fix keyboard X/Y orientation.

