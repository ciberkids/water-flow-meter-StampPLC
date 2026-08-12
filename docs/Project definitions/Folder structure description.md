# Folder structure

Rewritten 2026-08-12, after a consolidation pass. The previous version described Cypress tests that
were replaced by an in-memory parity test, a `.codex` directory that was removed, and none of
`src/net`, `src/sensors`, `src/ui/pack` or `tools/`.

## Where a fact lives, before the folders

The one rule that explains most of the layout: **one fact, one home.** Several artefacts here are
GENERATED and will silently revert if edited — the tree below marks each one, and
`docs/Requirements/feature addition/UI_Dataset_Contract.md` is the contract.

## `docs/`

- **`Requirements/`** — functional specifications for delivered code.
  - `Project_document.md` — the project's own requirements, including the Modbus register map.
  - `Gesture_Reference.md` — every gesture, **as built** rather than as specified. Moved here from
    *Project definitions* because it documents device behaviour, which is what this folder is for.
  - `Implementation_Alignment_Report.md` — where implementation and requirement diverged, and why.
  - **`feature addition/`** — one requirement document per incremental feature, plus:
    - `UI_Dataset_Contract.md` — **read first** before touching any UI JSON.
    - `screens/<id>.json` — authored, one per screen: the agreed geometry.
- **`active_work/`** — `open_decisions.md`, and only what is genuinely open. The 41 closed entries live
  in `archive/`; a register that lists settled questions does not get read.
- **`backlog/`** — outstanding stories. Each carries a status that has been checked against the code,
  not against memory.
- **`diagrams/`** — mermaid. `ui_navigation_tree` and `ui_config_layout` are **generated** by
  `tools/wiki/gen-diagrams.mjs` from the dataset; the other two are hand-drawn and dated.
- **`archive/`** — completed narratives and closed decisions, kept for history. Several record a
  decision being reversed, which is why they are kept rather than deleted.
- **`templates/`** — documentation templates.
- **`hardware docs/`** — datasheets and hardware references.
- **`Project definitions/`** — this file. It held the gesture reference and an IDE workspace file; both
  moved somewhere they belong.

> **Refinement findings** are appended to the original story or bug as a section, never split into a
> separate `*-refinement-findings.md`. The archive still holds a dozen of the old shape.

## `Water-Flow-Meter-PlatformIO/`

The authoritative firmware tree.

- **`src/`** — `firmware.cpp` plus the subsystems:
  - `input/` — button events, gesture combos, the editor's held-repeat ramp.
  - `led/` — the RGB controller and its patterns.
  - `modbus/` — the server, the register bank, sensor types, diagnostics.
  - `sensors/` — the pulse sampler and the per-channel state engine.
  - `net/` — WiFi state machine, MQTT publisher, Home Assistant discovery, the network register block,
    the configuration portal.
  - `bus/` — SPI arbitration between the display and the SD card.
  - `ui/core/` — the UI runtime: navigator, controller, renderer, bindings, settings.
  - `ui/pack/` — the `.uipack` binary format, its reader and the loader.
  - **`ui/generated/`** — **generated** by the exporter. Never edit; CI diffs it against a fresh export.
- **`test/host/`** — the host suite. Arduino-free by construction, which is what lets the navigator,
  the acceleration tiers, the LED patterns and the whole network stack be checked in a second. It
  catches `-Werror=switch` that PlatformIO does not.
- **`tools/manifest_gen/`** — emits `actionManifest.json` from the firmware catalogues.
- **`platformio.ini`** — build configuration.

## `web/mockup/`

React + Vite. The design tool, the simulator, and the exporter that produces the firmware's UI assets.

- `src/` — the app. `src/data/screens.json` and `src/data/actionManifest.json` are both **generated**.
- `tools/skeleton/generate.mjs` — builds `screens.json` from the requirement files and the manifest.
- `tools/exporter/` — dataset → IR → `GeneratedUi.{h,cpp}` + `default.uipack`, behind ten gates.
- `tools/audit/` — geometry, value and gallery audits.
- `tests/` — Vitest units, `node --test` for the exporter, Playwright for visual baselines. **The
  visual baselines are stale and CI runs no visual step**, so nothing reports it.

## `tools/`

Repo-level tooling, not tied to either workspace.

- `wiki/sync.sh` — publishes the wiki's orientation pages. The wiki carries pointers, never content.
- `wiki/gen-registers.mjs` — the Modbus register reference, generated from the firmware headers.
- `wiki/gen-diagrams.mjs` — the navigation diagrams, generated from the dataset.
- `wiki/pages/` — the long wiki pages, as Markdown files so they diff as prose.

## Everything else

- **`graphics/`** — `svg/` assets, and `mockupimages/` rendered during tests (`<test-name>-<date>`).
- **`backups/ui/<YYYYMMDD_HHMMSS>/`** — written by the exporter before it overwrites firmware assets.
- **`water flow meter.code-workspace`** — the VS Code workspace, at the root where an editor can find
  it. It was under *Project definitions* with a `../..` path that only worked from there.
- **`.github/workflows/ci.yml`** — three jobs: web + exporter, firmware host tests, firmware compile.
