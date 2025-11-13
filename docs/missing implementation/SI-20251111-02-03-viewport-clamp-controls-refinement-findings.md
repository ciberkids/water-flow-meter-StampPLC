# Story Refinement Findings — SI-20251111-02 viewport recovery & toolbox alignment

> **File naming:** store each report as `SI-<story-id>-<slug>-refinement-findings.md` (example: `SI-20250517-01-01-web-mockup-refinement-findings.md`).

**Story ID:** SI-20251111-02-03  
**Story Title:** Viewport clamp controls & nudge affordances  
**Implementation Artifact:** `web/mockup/src/App.tsx`, `web/mockup/src/components/design/DesignToolbox.tsx`, `web/mockup/src/App.css`, `web/mockup/src/data/screens.json`  
**Reviewer:** Codex  
**Review Date:** 2025-11-13  
**Status Recommendation:** needs refinement

---

## 1. Snapshot Summary
- **Observed Outcome:** The default layout loads with multiple boxes already spilling beyond the 135×240 canvas, arrow buttons sit outside the element cards and don’t mirror keyboard placement, the keyboard/pad arrows move elements along the wrong axes when the viewport is in landscape, and two different “Live screen JSON” panes render simultaneously (only one edits).
- **Intended Outcome:** Designers should immediately see an in-bounds mockup (or a one-click recovery control), have arrow controls that match physical input both in layout and axis behaviour, and work from a single editable JSON surface without redundant read-only dumps.
- **Gap Statement:** Core editing ergonomics regress—users can’t snap layouts back inside the display, directional nudging feels inverted, the arrow UI doesn’t live with each element, and the duplicated JSON panels crowd the Design column.

---

## 2. Findings Log

| # | Area / File | Severity* | Finding | Evidence (line / behaviour) | Proposed Refinement |
|---|-------------|-----------|---------|------------------------------|---------------------|
| 2.1 | `web/mockup/src/App.tsx`, `web/mockup/src/components/design/DesignToolbox.tsx` | High | The initial dataset renders with overflow items and the UI only lists them; there is no control to clamp everything back inside the physical display, nor a per-element “fix” action. | Layout diagnostics (`web/mockup/src/App.tsx:1364-1376`) lists each overflow ID but exposes zero buttons, and the toolbox rows (`DesignToolbox.tsx:70-187`) only allow manual typing or delete. | Add two remediation buttons: (a) a global “Clamp all to display” button colocated with the arrow controls that normalizes every out-of-bounds element, and (b) a per-row “Clamp to display” action that appears when `layoutReport.overflow` contains that element. Both should reuse the existing clamp logic so the dataset updates, not just the render. |
| 2.2 | `web/mockup/src/App.tsx`, `web/mockup/src/utils/layout.ts` | High | Arrow key assignments feel wrong because the nudge logic mutates portrait-space coordinates while the viewport defaults to landscape rotation, so pressing ↑ moves the element to the right, ↓ moves left, etc. | The editor initializes orientation to `"landscape"` (`web/mockup/src/App.tsx:254-257`) while `handleNudgeSelectedElement` simply increments `x/y` (`App.tsx:509-533`) and `computeLayout` rotates those coordinates (`web/mockup/src/utils/layout.ts:142-150`), producing perceived axis swaps. | Either lock editing orientation to portrait, or transform arrow deltas through the current orientation matrix so ↑ actually reduces the on-screen Y regardless of rotation. Apply the same mapping to on-screen arrow buttons to keep parity between keyboard and UI. |
| 2.3 | `web/mockup/src/components/design/DesignToolbox.tsx`, `web/mockup/src/App.css` | Medium | The “Move selected element” arrow pad is visually disconnected and does not mimic a real arrow cluster; buttons sit in two linear rows beneath the element list, so the muscle memory from a keyboard/gamepad doesn’t translate. | Markup places up, left/right, down in separate flex rows (`DesignToolbox.tsx:188-229`), and the styling uses a 2-row grid with a spanning middle block (`App.css:980-1004`), yielding a strip rather than a cross. | Restructure the pad into a 3×3 grid (blank center) that matches keyboard arrow layout, and embed it directly inside each element row (or at least within the selected card) so context is obvious. |
| 2.4 | `web/mockup/src/App.tsx` | Low | The Design column shows two “Live screen JSON” sections: a read-only preview inside `ThemeEditor` (`App.tsx:1428-1435`) and the editable `LiveJsonEditorPanel` (`App.tsx:1464-1468`). The duplication wastes vertical space and confuses which surface is authoritative. | Both sections render concurrently whenever the Design tab is active. | Remove the read-only preview (or replace it with a link to the editor) so only the editable JSON panel remains visible. |

> *Severity guidance: **High** (blocks requirement / creates defect), **Medium** (partial compliance / risky), **Low** (polish / documentation).

---

## 3. Suggested Refinement Tasks
- [ ] Add a global “Clamp all to display” control near the arrow pad that rewrites any out-of-bounds elements back inside the 135×240 canvas.
- [ ] Surface a per-element clamp button whenever that element appears in `layoutReport.overflow`, wiring it to the same normalization routine.
- [ ] Align keyboard and on-screen arrow deltas with the active orientation so ↑/↓/←/→ always move visually up/down/left/right, and reorganize the arrow pad into a 3×3 cluster embedded in the element toolbox.
- [ ] Remove the duplicate read-only “Live screen JSON” snippet, keeping only the editable JSON editor.

---

## 4. Dependencies & Questions
- **Upstream Impact:** Clamp actions should reuse `normalizeElementUpdate`/layout clamp helpers to avoid duplicate math; ensure JSON editor and undo history stay in sync after bulk normalization.
- **Open Questions:** Should per-element clamp buttons auto-select the corrected element and highlight it in the viewport? Do we need a confirmation modal before globally clamping many elements?
- **Testing Gaps:** No automated coverage exercises the new clamp buttons or orientation-aware movement; add unit tests for the math and a Playwright spec that presses ↑/↓ in both portrait and landscape.

---

## 5. Attachments & Links
- `web/mockup/src/App.tsx`
- `web/mockup/src/components/design/DesignToolbox.tsx`
- `web/mockup/src/App.css`
- `web/mockup/src/data/screens.json`

---

## 6. Reviewer Notes
- Consider surfacing a toast after each clamp action summarizing how many elements were corrected so designers know the operation succeeded.
