# StampPLC UI Mockup Workspace

This React + Vite workspace renders a pixel-accurate simulation of the StampPLC display so UI flows can be inspected without hardware.

## Quick start

```bash
cd web/mockup
npm install
npm run dev
```

The dev server launches at `http://localhost:5173/`. Use the screen selector to preview different templates and the zoom slider to enlarge the 135×240 display.

- Keyboard shortcuts mirror the StampPLC buttons: `Arrow Up`, `Arrow Down`, and `Enter` trigger the same long/short press behaviours as the on-screen controls.
- Design panel lets you adjust palette, typography scales, and easing presets in real time, writing changes straight into the dataset's `theme` block.
- Simulation view includes a live JSON snapshot for the active screen, while the **Help & Documentation** tab hosts the full schema reference and dataset summary.

## Visual regression snapshots

Automated Playwright tests capture the StampPLC viewport to guard against unintended visual changes.

```bash
npm run test:visual          # Build + run snapshot comparisons
npm run test:visual:update   # Rebuild and refresh the stored baselines
```

Baseline images live in `tests/visual/mockup.spec.ts-snapshots/`. Update them only when designs intentionally change.

## Firmware export pipeline

The translation engine converts the mockup JSON (including embedded design tokens) into firmware-ready C++ assets.

```bash
npm run export:firmware   # Validate data, back up existing assets, emit C++
npm run test:exporter     # Schema + IR unit tests (AJV + translator)
```

- Generated files land in `Water Flow Meter PlatformIO/src/ui/generated/` (header, implementation, IR JSON, metadata).
- Previous assets are copied into `backups/ui/<timestamp>/` before new files are written.
- CLI options allow overriding source/target paths or enabling `--dry-run` (see `tools/exporter/cli.ts` for supported flags).
- The Import & Export tab exposes an **Export to Firmware** button that calls the same translator endpoint and surfaces backup metadata inline.

## Dataset workflow

Manage the mockup dataset directly inside the UI:

- **Simulation** — load screens through the selector, inspect live JSON, and exercise the simulated hardware controls.
- **Design** — adjust layout tokens visually or via JSON, with the same screen selector/context cards embedded beside the editor.
- **Import & Export** — load or validate `screens.json`, download the current dataset, and run the firmware translator once validation succeeds.
- **Help & Documentation** — reference the schema, learn how the tool works, and review dataset metadata.
- Design tokens travel with `screens.json`; the translator reads them directly from the dataset so firmware mirrors what you preview.

## Project structure

```
web/mockup
├── package.json         # Dependencies and scripts
├── tsconfig*.json       # TypeScript configuration
├── vite.config.ts       # Vite setup (React + TS)
├── index.html           # Root HTML shell
└── src
    ├── App.tsx          # Workspace shell
    ├── main.tsx         # React bootstrap
    ├── components
    │   ├── DisplayViewport.tsx
    │   ├── HelpPanel.tsx
    │   ├── ThemeEditor.tsx   # Design panel (palette, typography, preview)
    │   └── ScreenSelector.tsx
    ├── data
    │   └── screens.json # Screen layout definitions
    ├── theme            # Theme provider, defaults, and types
    └── types.ts         # Shared TypeScript types
```

## Screen layout JSON

Layouts are defined in `src/data/screens.json` using simple primitives (text, rectangle, icon placeholders). When you edit the JSON, the viewport hot-reloads to display the new layout.

## Next steps

- Extend the JSON schema to cover animations and richer components. ✅
- Integrate exporter outputs into the firmware build target (`src/ui/core` refactor). (Story SI-20250517-05)
- Add Playwright/Cypress tests to exercise button interactions (story SI-20250517-02).
