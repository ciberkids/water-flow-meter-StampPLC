# Story: SI-20250517-01-web-mockup-shell — Web Mockup Workspace Scaffold

> **Naming Convention**  
> Store active story specs under `docs/missing implementation/` using the identifier format above, for example `SI-20250517-01-web-mockup-shell.md`.  
> Reference the story from `docs/stories to implement/missing_implementation_stories.md` with the same ID so incremental progress stays traceable.

**Status:** completed  
**Author:** Matteo  
**Last Updated:** 2025-05-17  
**Linked Requirements:** docs/Requirements/feature addition/Display_Web_Mockup_and_Translator.md#31-web-display-mockup-workspace  
**Related Features:** StampPLC UI redesign

---

## 1. Summary

Bootstrap a React-based web workspace that renders a pixel-accurate (135×240) canvas for every StampPLC screen, enabling designers to visualise flows without hardware.

---

## 2. Acceptance Criteria

- [x] React application presents selectable Info, Configuration, Countdown templates driven from data.
- [x] Canvas renders at 135×240 logical pixels with adjustable zoom controls and optional grid overlay.
- [x] Layout loader reads screen metadata from a JSON definition file to seed initial UI positions.

---

## 3. Implementation Notes

- Use Vite + React + TypeScript for fast development and type safety.
- Represent each screen template as data-driven definitions to align with the later translation pipeline.
- Ensure rendering stack accommodates future animation hooks (e.g., requestAnimationFrame).

---

## 4. Tasks & Checklist

- [x] Scaffold Vite/React project inside `web/mockup` adjacent to firmware sources.
- [x] Implement display viewport component with zoom slider and pixel grid toggle.
- [x] Load baseline screen configurations from JSON and render static elements.
- [x] Document project setup in `README.md` within the web workspace.

---

## 5. Verification Plan

1. Manual QA: open each template in the browser, confirm dimensions and zoom behaviour.
2. Snapshot tests for React components to ensure layout regressions are caught.
3. Confirm JSON layout changes hot-reload correctly.

---

## 6. Backout / Fallback Strategy

- Keep the web app isolated from firmware build system; removal is a matter of deleting the `web/mockup` directory.
- Document cleanup steps in case the scaffold needs to be regenerated.

---

## 7. Notes & Open Questions

- Future enhancement: integrate SVG import for complex graphics.
- Evaluate whether to embed design tokens (colours/typography) at this stage or defer to theming story.
