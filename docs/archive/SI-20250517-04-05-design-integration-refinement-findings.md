# Story Refinement Findings — SI-20250517-04 design integration

**Story ID:** SI-20250517-04  
**Story Title:** Node.js translation engine & JSON schema  
**Implementation Artifact:** `web/mockup/src/App.tsx` • `web/mockup/src/components/ThemeEditor.tsx`  
**Reviewer:** Codex  
**Review Date:** 2025-11-06  
**Status Recommendation:** ready for review

---

## 1. Snapshot Summary
- **Observed Outcome:** Design controls now surface under a “Design” tab, embed changes directly into the dataset theme block, and show a live viewport preview within the same pane.
- **Intended Outcome:** Designers refine visual tokens in context and export firmware knowing `screens.json` already reflects palette, typography, and easing choices.
- **Gap Statement:** Previous detachment between theme presets and dataset has been addressed; the workflow now matches the documented intent.

---

## 2. Findings Log

| # | Area / File | Severity* | Finding | Evidence (line / behaviour) | Resolution |
|---|-------------|-----------|---------|------------------------------|------------|
| 2.1 | `web/mockup/src/components/ThemeEditor.tsx` | Medium | Theme export/import copy misled designers into thinking styling sat outside the dataset. | Prior buttons labelled “Export theme” and JSON lacking updates. | `ThemeEditor` now writes tokens straight into `dataset.theme`, removes the export button, and surfaces live feedback (see lines 181-318). |
| 2.2 | `web/mockup/src/App.tsx` | Low | Tab labelled “Theme” didn’t reflect broader design responsibilities. | Workspace navigation showed “Theme” label. | Navigation uses “Design”, export copy clarifies embedded tokens, and active panel wiring updated (lines 397-544). |
| 2.3 | `web/mockup/src/components/ThemeEditor.tsx` | Medium | Designers lacked an in-place preview while editing tokens. | Preview existed only on Workspace pane. | Design tab now hosts a viewport preview driven by current zoom/grid/layout (lines 212-224). |

> *Severity guidance: **High** (blocks requirement / creates defect), **Medium** (partial compliance / risky), **Low** (polish / documentation).

---

## 3. Suggested Refinement Tasks
- [x] Embed theme/design adjustments into the JSON definition (or persist alongside it) and remove standalone export/import prompts.
- [x] Rename the Theme tab/menu copy to “Design” (and update documentation accordingly).
- [x] Render the display preview within the Design view so designers see styling changes instantly.

---

## 4. Dependencies & Questions
- **Upstream Impact:** Exporter now expects the dataset theme; CLI retains `--theme` override for backwards compatibility.
- **Open Questions:** None outstanding.
- **Testing Gaps:** None outstanding — Playwright visual suite updated and Cypress exporter parity run green.

---

## 5. Attachments & Links
- `web/mockup/src/App.tsx`
- `web/mockup/src/components/ThemeEditor.tsx`
- `docs/missing implementation/SI-20250517-04-04-theme-alignment-refinement-findings.md`

---

## 6. Reviewer Notes
Finalize how design tokens coexist with screen JSON before exposing the workflow to designers; avoid mixed messages about export behaviour.
