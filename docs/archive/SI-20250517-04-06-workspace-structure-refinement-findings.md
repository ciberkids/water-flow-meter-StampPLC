# Story Refinement Findings — SI-20250517-04 workspace structure

**Story ID:** SI-20250517-04  
**Story Title:** Node.js translation engine & JSON schema  
**Implementation Artifact:** `web/mockup/src/App.tsx`  
**Reviewer:** Codex  
**Review Date:** 2025-11-07  
**Status Recommendation:** needs refinement

---

## 1. Snapshot Summary
- **Observed Outcome:** Workspace navigation still exposes the original “Workspace/Design/Export/Help & JSON” tabs with dataset tooling hard-coded into the left sidebar.
- **Intended Outcome:** The application should surface four explicit sections — Simulation, Design, Import/Export, Help & Documentation — with dataset management consolidated inside the Import/Export experience and screen selectors visible inside both Simulation and Design contexts.
- **Gap Statement:** Structural expectations for the story’s workspace layout are unmet, so designers cannot evaluate the updated IA nor validate the new Import/Export workflow requirements.

---

## 2. Findings Log

| # | Area / File | Severity* | Finding | Evidence (line / behaviour) | Proposed Refinement |
|---|-------------|-----------|---------|------------------------------|---------------------|
| 2.1 | `web/mockup/src/App.tsx:397-426` | Medium | Tab copy/layout still exposes “Workspace” and “Export” instead of “Simulation” and “Import/Export”, so the four-section mental model isn’t represented. | `workspace-tabs` buttons render literal labels “Workspace”, “Design”, “Export”, “Help & JSON”. | Rename the first tab to “Simulation”, rename the export tab to “Import/Export”, and ensure the corresponding `activePanel` keys and any copy/tests use the new vocabulary. |
| 2.2 | `web/mockup/src/App.tsx:332-377` | Medium | Dataset tooling (import/validate/export buttons, validation feedback) lives permanently in the sidebar rather than inside the Import/Export section. | The sidebar renders “Dataset tools” regardless of which tab is active. | Relocate dataset controls and feedback into the Import/Export view so that import/export responsibilities are co-located; update layout spacing in the sidebar once the block moves. |
| 2.3 | `web/mockup/src/App.tsx:379-394` | Medium | The “Screens” list and “Screen details” only exist in the sidebar, meaning Design view doesn’t get the dedicated context called for in the story (Simulation and Design should both host these blocks). | Sidebar-only sections render screen selector/details; no mirrored panels inside Simulation/Design containers. | Duplicate/move the selector and details UI into both Simulation and Design panels so that each view surfaces screen context alongside its tools, adjusting layout to avoid duplication elsewhere. |

> *Severity guidance: **High** (blocks requirement / creates defect), **Medium** (partial compliance / risky), **Low** (polish / documentation).

---

## 3. Suggested Refinement Tasks
- [ ] Update tab routing to “Simulation / Design / Import & Export / Help & Documentation” and align `activePanel` identifiers plus tests/copy.
- [ ] Move the dataset tool stack (import, validate, export, validation feedback) from the sidebar into the Import/Export panel layout.
- [ ] Embed the screen selector + screen details modules inside both Simulation and Design panels, ensuring responsive layout parity and updating CSS/tests accordingly.
- [ ] Refresh Playwright specs and snapshots to cover the renamed tabs and relocated UI blocks.

---

## 4. Dependencies & Questions
- **Upstream Impact:** Playwright regression snapshots (`mockup.spec.ts-snapshots`) and any Cypress/export automation will need updated tab labels and selectors.
- **Open Questions:** None — requirements clearly describe the four-section structure.
- **Testing Gaps:** No automated coverage currently ensures the dataset tools render inside the Import/Export panel or that Simulation/Design each host the screen context blocks.

---

## 5. Attachments & Links
- `web/mockup/src/App.tsx`
- `web/mockup/tests/visual/mockup.spec.ts`

---

## 6. Reviewer Notes
- Rename-related changes will ripple through localization copy, aria labels, and Playwright locators — plan for snapshot updates plus string replacements within documentation (`README.md`) to maintain a consistent IA description.
