# Story Refinement Findings — SI-20251111-02 design toolbox/editor follow-up

> **File naming:** store each report as `SI-<story-id>-<slug>-refinement-findings.md` (example: `SI-20250517-01-01-web-mockup-refinement-findings.md`).

**Story ID:** SI-20251111-02  
**Story Title:** Design authoring & hierarchy tools  
**Implementation Artifact:** `web/mockup/src/components/design/DesignToolbox.tsx`, `src/App.tsx` (Design tab)  
**Reviewer:** Codex  
**Review Date:** 2025-11-12  
**Status Recommendation:** resolved

---

## 1. Snapshot Summary
- **Observed Outcome:** Design toolbox now exposes width/height fields for geometric elements, enforces coordinate bounds (≤4 digits), lets users select an element via radio button, and provides both UI + keyboard arrow controls for nudging positions.
- **Intended Outcome:** Requirements called for practical geometry editing with bounded inputs plus directional controls tied to keyboard arrows.
- **Gap Statement:** All identified gaps have been addressed in `web/mockup/src/App.tsx` & `src/components/design/DesignToolbox.tsx`.

---

## 2. Findings Log

| # | Area / File | Severity* | Finding | Evidence (line / behaviour) | Proposed Refinement |
|---|-------------|-----------|---------|------------------------------|---------------------|
| 2.1 | `web/mockup/src/components/design/DesignToolbox.tsx` | Medium | Box elements expose only content/X/Y fields; width & height cannot be edited despite requirements for explicit dimensions. | Element rows render inputs for `content`, `x`, and `y` only (lines ~38-86). | ✅ Width/height inputs now render for `box`, `icon`, `animation`, and `scrollbar` kinds and persist through `onUpdateElement`. |
| 2.2 | `web/mockup/src/components/design/DesignToolbox.tsx` | Medium | Coordinate/size fields accept arbitrarily long strings; designers can type “123456” which exceeds the 135×240 display, producing invalid layouts. | Inputs currently use `<input type="number">` with no `maxLength` or min/max attributes. | ✅ Inputs clamp to 4 digits, sanitize non-numeric characters, and enforce screen-bound maxima before updating state. |
| 2.3 | `web/mockup/src/components/design/DesignToolbox.tsx` & keyboard UX | Medium | No element selection affordance: users cannot mark which element subsequent actions apply to. | Toolbox lists elements but lacks radio/checkbox selection. | ✅ Radio buttons select the active element (highlighted row), and state is stored in `App.tsx` for downstream actions. |
| 2.4 | `web/mockup/src/App.tsx` (Design tab) | Medium | There is no quick way to bump coordinates; moving a distant element requires manual typing and scrolling. Requirement asks for arrow buttons (and keyboard arrows) to nudge X/Y of the selected element. | Design toolbox currently only exposes numeric inputs. | ✅ Added directional button cluster plus keyboard arrow interception (when Design tab is active) to nudge the selected element while respecting display bounds. |

> *Severity guidance: **High** (blocks requirement / creates defect), **Medium** (partial compliance / risky), **Low** (polish / documentation).

---

## 3. Suggested Refinement Tasks
- [x] Add width/height controls (with persistence + validation) for geometric elements in `DesignToolbox`, updating schema hints if needed.
- [x] Enforce 4-character max + min/max range on X/Y/width/height inputs to reflect the physical display limits.
- [x] Implement element selection via radio buttons and store the active element ID in design state.
- [x] Add arrow button cluster (plus keyboard bindings) that nudges the selected element’s coordinates; ensure movements respect bounds and update the dataset + JSON editor.

---

## 4. Dependencies & Questions
- **Upstream Impact:** None beyond Design tab; exporter already handles width/height so surfacing them shouldn’t break IR.
- **Open Questions:** Confirm desired step size for arrow-based nudges (1 px vs. grid increments) and whether modifiers (Shift/Ctrl) should accelerate movement.
- **Testing Gaps:** Need Playwright coverage for selecting an element, pressing arrow controls (including keyboard), and verifying dataset JSON updates + bounds enforcement.

---

## 5. Attachments & Links
- `docs/Requirements/feature addition/Display_Web_UI_Workspace_Improvements.md` (Design tab behaviour section)
- `web/mockup/src/components/design/DesignToolbox.tsx`

---

## 6. Reviewer Notes
- Consider highlighting the selected element inside the viewport (e.g., outline) when the radio button is active. This will reinforce the selection context when nudging via arrows.
