# Story Refinement Findings — Template

**Story ID:** SI-20250517-01  
**Story Title:** Web Mockup Visual Fidelity & Interaction Refinement  
**Implementation Artifact:** `web/mockup/` (React workspace)  
**Reviewer:** Matteo  
**Review Date:** 2025-10-28  
**Status Recommendation:** ready for verification

---

## 1. Snapshot Summary
- **Observed Outcome:** Current mockup renders a stylised viewport that diverges from hardware typography, grid density, and orientation.  
- **Intended Outcome:** Browser mockup should faithfully mirror the StampPLC display capabilities so layout tweaks translate directly to firmware assets.  
- **Gap Statement:** Rendering stack ignores device font/grid constraints, lacks orientation controls, and provides no input simulation, leaving the story incomplete for spec refinement.

---

## 2. Findings Log

| # | Area / File | Severity* | Finding | Evidence (line / behaviour) | Proposed Refinement |
|---|-------------|-----------|---------|------------------------------|---------------------|
| 1 | `web/mockup/src/components/DisplayViewport.tsx` | High | Typography and drawing primitives exceed what the device graphics library can reproduce. | Rendered text uses web fonts and CSS effects absent from firmware canvas. | Analyse StampPLC rendering library (fonts, glyph set, spacing) and rebuild components to match its capabilities exactly. |
| 2 | `web/mockup/src/components/DisplayViewport.tsx` | High | Grid overlay does not align with the pixel cadence used by the firmware graphics engine. | CSS background grid uses fixed spacing unrelated to device pixel painting. | Reimplement grid to mirror the library’s pixel map (same origin, pitch, stroke). |
| 3 | `web/mockup/src/App.tsx` | Medium | Mockup assumes portrait orientation while hardware ships with a landscape mount. | App hardcodes viewport aspect and positioning as portrait. | Add orientation toggle/rotation support so designers can validate both orientations. |
| 4 | `web/mockup/src/App.tsx` | Medium | Display panel overlaps workspace frame, reducing visual clarity. | Zoomed viewport spills outside provided container at default settings. | Adjust layout and styling to maintain proper margins and framing across zoom presets. |
| 5 | `web/mockup/src` | Medium | No UI affordance to simulate StampPLC button interactions. | Workspace lacks any buttons tied to firmware control scheme. | Add virtual BtnA/BtnB/BtnC controls (matching firmware semantics) to drive screen transitions. |

> *Severity guidance: **High** (blocks requirement / creates defect), **Medium** (partial compliance / risky), **Low** (polish / documentation).

---

## 3. Suggested Refinement Tasks
- [x] Audit StampPLC graphics library (font metrics, drawing APIs) and rebuild text/shape components to stay within device limitations.
- [x] Implement grid renderer that reproduces device pixel grid and honours zoom scaling.
- [x] Introduce orientation toggle (portrait/landscape) and update layout constraints to avoid overflow.
- [x] Create virtual control panel for BtnA/BtnB/BtnC interactions to drive screen state changes.
- [x] Run visual regression captures after alignment to confirm fidelity. _(Manual capture performed; automation queued for follow-up story.)_

---

## 4. Dependencies & Questions
- **Upstream Impact:** Alignment work must stay consistent with firmware rendering utilities to avoid drift.  
- **Open Questions:** Need confirmation of final mounting orientation and exact font asset the firmware will ship with.  
- **Testing Gaps:** No automated snapshot/interaction tests exist to guard against future mismatches; add after refactor.

---

## 5. Attachments & Links
- `docs/missing implementation/SI-20250517-01-web-mockup-shell.md` (original story)
- `docs/templates/story_refinement_findings_template.md` (source template)
- `web/mockup/README.md` (workspace setup)

---

## 6. Reviewer Notes
These findings introduce a follow-up refinement story to ensure the web mockup is an accurate proxy for the physical display, including device-accurate rendering and interactive controls mirroring firmware input handling.

---

## 7. Resolution Summary (2025-05-17)
- Display viewport now applies the StampPLC pixel font metrics and palette to mirror firmware drawing primitives.
- Canvas-driven grid overlay draws the exact device pixel cadence with major ticks, remaining crisp across zoom levels.
- Orientation toggle feeds layout rotation and viewport sizing while keeping the mockup framed within the workspace.
- Virtual BtnA/BtnB/BtnC panel simulates short, long, and repeat presses and logs interactions for verification.
