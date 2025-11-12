# Story Refinement Findings — SI-20250517-04 theme alignment

**Story ID:** SI-20250517-04  
**Story Title:** Node.js translation engine & JSON schema  
**Implementation Artifact:** `web/mockup/src/App.tsx` • `web/mockup/src/components/ThemeEditor.tsx` • `web/mockup/src/components/HelpPanel.tsx`  
**Reviewer:** Codex  
**Review Date:** 2025-11-05  
**Status Recommendation:** ready for verification

---

## 1. Snapshot Summary
- **Observed Outcome:** Theme controls still surface on the main Workspace tab, theme tweaks do not affect the JSON dataset, and schema documentation omits theming guidance.
- **Intended Outcome:** Workspace should focus strictly on layout editing; theme editing should correlate with JSON/theming schema, with clear documentation.
- **Gap Statement:** Current layout and docs leave the theming story half-integrated and potentially confusing for designers exporting JSON.

---

## 2. Findings Log

| # | Area / File | Severity* | Finding | Evidence (line / behaviour) | Proposed Refinement |
|---|-------------|-----------|---------|------------------------------|---------------------|
| 2.1 | `web/mockup/src/App.tsx` | Medium | Theme adjustments remain visible in the Workspace view after navigation refactor. | Even on the Workspace tab, a ThemeEditor card still renders alongside layout tooling. | Remove ThemeEditor from the Workspace runtime and ensure it only appears on the Theme tab. |
| 2.2 | `web/mockup/src/components/ThemeEditor.tsx`, `App.tsx` | Medium | Theme editor alters UI styling but does not update the dataset JSON; live JSON remains unchanged. | `screens.json` preview shows no theming fields; theme state is stored separately. | Confirm desired behaviour: either document that theming is decoupled or introduce a mechanism to persist theme choices in exported JSON. |
| 2.3 | `web/mockup/src/components/HelpPanel.tsx` | Medium | Schema reference lacks placeholders for theme-related fields despite requirements mentioning future theming export. | Help panel template only references screen/elements, not theme tokens. | Extend schema documentation with notes on theme tokens or link to where theme data resides. |

> *Severity guidance: **High** (blocks requirement / creates defect), **Medium** (partial compliance / risky), **Low** (polish / documentation).

---

## 3. Suggested Refinement Tasks
- [x] Ensure ThemeEditor renders exclusively on the Theme tab and is removed from Workspace content.
- [x] Decide and document how theme changes interact with dataset export (persisted JSON vs. dedicated theme file) and update UI copy accordingly.
- [x] Update help/schema documentation to reflect the current or planned theming structure and reference theme tokens.

---

## 4. Dependencies & Questions
- **Upstream Impact:** Depends on translation engine expectations for theme tokens; clarify whether firmware expects theme data in JSON.
- **Open Questions:** Should theme adjustments export alongside screens? If theme export is deferred, where should designers manage theme presets?
- **Testing Gaps:** None yet—add end-to-end tests only after behaviour is clarified.

---

## 5. Attachments & Links
- `web/mockup/src/App.tsx`
- `web/mockup/src/components/ThemeEditor.tsx`
- `web/mockup/src/components/HelpPanel.tsx`

---

## 6. Reviewer Notes
Clarify theme persistence and documentation before exposing the tooling to designers; otherwise they may assume theme tweaks affect exported assets when they currently do not.

## 7. Resolution Summary (2025-11-05)
- Workspace tab now shows layout tooling only; ThemeEditor resides on the dedicated Theme tab.
- UI copy (Theme tab, Export view, README) clarifies that theme changes do not modify `screens.json` and should be exported separately as `theme.json`.
- Help schema reference mentions theme tokens and points designers to the Theme tab for managing them.
