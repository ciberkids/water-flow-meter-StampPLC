# Story Refinement Findings — SI-20251111-02 keyboard navigation & bounds guardrails

> **File naming:** store each report as `SI-<story-id>-<slug>-refinement-findings.md` (example: `SI-20250517-01-01-web-mockup-refinement-findings.md`).

**Story ID:** SI-20251111-02-02  
**Story Title:** Continuous arrow nudging & viewport bounds clamping  
**Implementation Artifact:** `web/mockup/src/App.tsx`, `web/mockup/src/utils/layout.ts`, `web/mockup/src/components/DisplayViewport.tsx`, `web/mockup/src/components/design/DesignToolbox.tsx`  
**Reviewer:** Codex  
**Review Date:** 2025-11-13  
**Status Recommendation:** needs refinement

---

## 1. Snapshot Summary
- **Observed Outcome:** Holding arrow keys inside the Design tab performs just a single nudge, and elements with oversized coordinates/sizes render outside the 135×240 canvas with no automatic correction when the dataset loads.
- **Intended Outcome:** Requirements call for continuous press-and-hold keyboard navigation plus automatic guardrails so no element exceeds the physical display; any corrected element should be highlighted immediately, including during load.
- **Gap Statement:** Keyboard nudging currently requires repeated taps and the renderer leaves invalid geometry untouched, so mis-placed items persist silently.

---

## 2. Findings Log

| # | Area / File | Severity* | Finding | Evidence (line / behaviour) | Proposed Refinement |
|---|-------------|-----------|---------|------------------------------|---------------------|
| 2.1 | `web/mockup/src/App.tsx` | Medium | The global `keydown` handler rejects repeat events, so holding an arrow key never issues successive nudges; users must press the key multiple times to move elements. | `handleKeyDown` bails out whenever `event.repeat` is true before calling `handleNudgeSelectedElement` (`web/mockup/src/App.tsx:1135-1149`). | Let keyboard repeat drive the nudge loop (e.g., respond to every repeat or run a `requestAnimationFrame` ticker while the key is held) while still blocking repeats for button-simulation keys; keep existing panel/selection guards. |
| 2.2 | `web/mockup/src/utils/layout.ts`, `web/mockup/src/components/DisplayViewport.tsx` | High | Layout calculations only tag out-of-bounds entries but never clamp them, and the viewport simply adds an `overflow` class, so elements can render beyond the device bezel and remain there after load. Requirements ask to correct values to the physical limit, highlight the correction, and do so even while a dataset is loading. | `computeLayout` pushes `overflow` items without modifying `left/width/height` (`web/mockup/src/utils/layout.ts:117-139`), and `DisplayViewport` just toggles an `overflow` CSS class with no correction (`web/mockup/src/components/DisplayViewport.tsx:122-157`). | Normalize coordinates/sizes on ingest/render (shared clamp helper), persist corrected values or flag them for author action, and surface a clear highlight/toast listing the elements that were clipped during load. Ensure live previews and exported datasets stay within 135×240. |

> *Severity guidance: **High** (blocks requirement / creates defect), **Medium** (partial compliance / risky), **Low** (polish / documentation).

---

## 3. Suggested Refinement Tasks
- [x] Enable continuous keyboard nudging by handling repeated `keydown` events (or driving a controlled interval) while respecting current panel focus.
- [x] Clamp X/Y/width/height on dataset ingest and layout render, and surface a visible highlight/toast when a value was corrected because it exceeded the display.
- [x] Add regression tests (unit + Playwright) that hold an arrow key to move an element and import a dataset containing out-of-bounds elements, verifying both the clamping and highlight.

---

## 4. Dependencies & Questions
- **Upstream Impact:** Coordinate with validation logic in `DesignToolbox` so the single clamp utility is shared between input fields, dataset ingestion, and layout render to avoid drift.
- **Open Questions:** Should auto-corrected values be persisted immediately, or should we block save until a human accepts them? What highlight treatment (outline vs. toast vs. inspector badge) best matches the design guidelines?
- **Testing Gaps:** No automated coverage for press-and-hold keyboard nudging or for dataset load scenarios that contain overflowing coordinates/sizes.

---

## 5. Attachments & Links
- `docs/Requirements/feature addition/Display_Web_UI_Workspace_Improvements.md` (keyboard + bounds requirements)
- `web/mockup/src/data/screens.json` (example dataset that can be edited to reproduce load-time overflow)

---

## 6. Reviewer Notes
- Consider showing a consolidated alert (e.g., “3 elements were auto-clipped to fit the display”) so authors immediately know which items need attention after loading or importing data.
