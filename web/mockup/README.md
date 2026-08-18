# StampPLC UI Mockup Workspace

This React + Vite workspace simulates the StampPLC display — **240 × 135, landscape** — so UI flows can be
inspected without hardware, and exports the dataset it shows into `constexpr` C++ the firmware renders.

Landscape is not a setting. Decision **D3** fixed the panel at 240 × 135 and the Design tab's orientation
control is a disabled indicator saying so. The ST7789V2's native *portrait* dimensions (135 × 240) survive
in `src/utils/layout.ts` as `DISPLAY_WIDTH`/`DISPLAY_HEIGHT` and are swapped for display — so a `135` in
this codebase is a panel dimension, never a bound on the x axis.

## Quick start

```bash
cd web/mockup
npm ci                       # not npm install — the lockfile is authoritative
npm run dev
```

The dev server launches at `http://localhost:5173/`. **You start on the shipped dataset:**
`src/data/screens.json`, 80 screens plus a `theme` block. It is not a blank canvas — import your own JSON
from the Import & Export tab if you want one.

- **Keyboard.** `ArrowUp`, `ArrowDown` and `Enter` are the three device buttons; short, long and hold come
  from how long you hold them, exactly as on the panel. In the **Design** tab the arrow keys are taken over
  to nudge the selected element by one pixel instead, so drive the simulation from the Simulation tab.
- **The four tabs own different things.** Simulation: the viewport, the device-memory values panel, the
  function trace and layout diagnostics. Design: the element toolbox, theme editor, screen hierarchy, event
  bindings and the live JSON editor. Import & Export: import, validate, download, export to firmware. Help
  & Documentation: the schema reference and dataset summary.
- **The Design panel writes palette and typography** straight into the dataset's `theme` block. It offers
  no easing presets: animation was dropped by decision **C1** in favour of the scrollbar. The theme's
  `animation.easing` token still exists and nothing on the device reads it — tracked as **J2**.

## Visual regression snapshots

```bash
npm run test:visual          # tsc + vite build + Playwright
npm run test:visual:update   # the same, refreshing the stored baselines
```

**Use the npm scripts, not a bare `npx playwright test`.** `vite preview` serves whatever is already in
`dist/` and does not build, so a bare Playwright run tests a stale bundle — that is how a renamed button
once broke every test through `beforeEach` while the suite still looked green.

Baselines live in `tests/visual/mockup.spec.ts-snapshots/`. Refresh them only when a design change is
intended, and never as a way to make failures go away: the suite spent months failing 32 of 46 tests
(**DF17**) and 11 of those failures were assertions about the workspace's own controls, not moved
baselines. **CI does not run this suite** (**DF21**), so it is only as fresh as the last person to run it.

## Firmware export pipeline

```bash
npm run export:firmware   # validate, back up existing assets, emit C++, compile
npm run test:exporter     # schema + IR tests (AJV + translator)
```

- Generated files land in `Water-Flow-Meter-PlatformIO/src/ui/generated/` — header, implementation, IR
  JSON and metadata.
- Previous assets are copied into `backups/ui/<timestamp>/` before anything is written.
- Flags worth knowing, all in `tools/exporter/cli.ts`: `--screens <file>` and `--manifest <file>` override
  the inputs (an explicitly empty `--manifest` is the only way to skip it), `--dry-run` skips the
  compilation step, and `--allow-missing-toolchain` waives it when no container is available.
- `export:firmware` compiles the real firmware in a container, which makes it the strongest gate here. It
  also rewrites three `generated/` timestamps on every run — check `git diff` before committing.
- The Import & Export tab's **Export to Firmware** button calls the same translator and shows the backup
  metadata inline.
- **Commit the dataset and the regenerated assets together.** CI fails if a fresh export disagrees with the
  committed copy.

## Dataset workflow

- **Simulation** — pick a screen in the selector and drive the simulated buttons.
- **Design** — edit layout and tokens visually or as JSON, with the live screen JSON beside the editor.
- **Import & Export** — load or validate a dataset, download the current one, run the translator. A legacy
  dataset for regression tests lives in `tests/fixtures/legacy-screens.json`; today's export gates
  deliberately **refuse** it (screens the firmware requires are missing, and bindings it names are not in
  the manifest), which is what makes it useful for exercising the failure path.
- Design tokens travel inside `screens.json`, so the translator reads what you previewed.

## Project structure

```
web/mockup
├── package.json          # dependencies and scripts
├── playwright.config.ts  # visual suite: chromium, 1280x960, vite preview on :4173
├── vite.config.ts        # Vite (React + TS) and the export endpoint the UI calls
├── shared/               # schema definitions shared by the app and the exporter
├── tools
│   ├── exporter/         # dataset -> IR -> C++ , and the export gates
│   ├── skeleton/         # generates the default dataset from the firmware catalogues
│   └── audit/            # screen-geometry.ts and screen-spec.ts geometry audits
├── tests
│   ├── visual/           # Playwright suite + committed baselines
│   └── fixtures/         # legacy-screens.json, sample-manifest.json, default.uipack
└── src
    ├── App.tsx           # workspace shell: the four tabs and the simulation state
    ├── components
    │   ├── DisplayViewport.tsx      # the emulated 240x135 panel
    │   ├── ScreenSelector.tsx       DeviceGrid.tsx        ButtonPanel.tsx
    │   ├── FirmwareValuesPanel.tsx  FirmwareLoopPanel.tsx SimulationTracePanel.tsx
    │   ├── ThemeEditor.tsx          ExporterPanel.tsx     HelpPanel.tsx
    │   └── design/                  # DesignToolbox, EventBindingPanel,
    │                                # LiveJsonEditorPanel, ScreenHierarchyPanel
    ├── data/screens.json # the shipped dataset — generated, see below
    ├── theme/            # theme provider, defaults, types
    └── utils/            # layout, clamping, sensor config, flow matching
```

## Screen layout JSON

`src/data/screens.json` is **generated**, not hand-written:

```bash
node tools/skeleton/generate.mjs --write
```

It builds the default menu from the firmware's own catalogues and refuses to emit one that leaves any
`category: "setting"` value unreachable. CI compares the result byte-for-byte with the committed file, so
editing the dataset by hand and committing it is a failing build. Elements are simple primitives — text,
value, badge, box, icon, scrollbar — and the viewport hot-reloads as you edit.

## Where the open items live

`docs/active_work/open_decisions.md` is the single source of truth for open work on this project, including
this workspace's. Every item has a stable ID (rule **I3** there): the ones this README mentions are
**DF17**, **DF21**, **J2**, **C1** and **D3**. Cite the ID rather than describing the problem again here —
a second list is how the first one goes stale.
