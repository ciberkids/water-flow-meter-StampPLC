# Story: SI-20250517-03-theming-engine — Theme & Animation Editor

> **Naming Convention**  
> Store active story specs under `docs/missing implementation/` using the identifier format above, for example `SI-20250517-03-theming-engine.md`.  
> Reference the story from `docs/stories to implement/missing_implementation_stories.md` with the same ID so incremental progress stays traceable.

**Status:** completed  
**Author:** Matteo  
**Last Updated:** 2025-05-17  
**Linked Requirements:** docs/Requirements/feature addition/Display_Web_Mockup_and_Translator.md#33-ui-styling--theming  
**Related Features:** StampPLC UI redesign

---

## 1. Summary

Introduce a theming workspace that lets users adjust colour schemes, typography tokens, and animation curves, with immediate preview across all mockup screens.

---

## 2. Acceptance Criteria

- [x] Theme editor exposes primary/secondary colours, typography scales, and animation presets.
- [x] Changes propagate live to all screens through CSS variables or design tokens.
- [x] Export action writes theme settings into a JSON descriptor reusable by the translation engine.
- [x] `npm run test:visual` completes without snapshot regressions.

---

## 3. Implementation Notes

- Theme tokens live in a dedicated provider (`ThemeProvider`) that hydrates from/saves to `localStorage`, keeping React components and future tooling in sync.
- Palette controls convert hex input into RGBA when needed (e.g., grid overlays) so transparency is preserved while remaining editable in native colour inputs.
- Animation easing presets drive global transition timing via CSS variables, ensuring future animation work can hook into the same token.
- Workspace now surfaces a live JSON snapshot of the active screen while a Help tab retains the full schema reference and dataset summary.

---

## 4. Tasks & Checklist

- [x] Build theme editor panel with colour pickers, typography sliders, and easing preset selector.
- [x] Implement token propagation using a shared React context and runtime-applied CSS variables.
- [x] Add persistence via browser `localStorage`, including reset-to-default controls.
- [x] Serialize active theme to `theme.json` for downstream tooling.

---

## 5. Verification Plan

1. Playwright visual regression (`npm run test:visual`) confirms default theme snapshots remain stable.
2. Playwright keyboard + theme interactions assert palette/typography updates propagate to the viewport and can be reset.
3. Manual inspection of exported `theme.json` verifies token structure ahead of translator integration.
4. Playwright snapshots cover theme editor layout at multiple breakpoints and validate live JSON rendering across workspace/help tabs.

---

## 6. Backout / Fallback Strategy

- Keep default theme hard-coded; if the editor fails, fallback to default tokens with a reset button.
- Version theme JSON files to allow rollback to previous presets.

---

## 7. Notes & Open Questions

- Future work: allow animation timelines to preview at variable speeds.
- Determine whether to expose granular control over LED legend styling within the editor.
