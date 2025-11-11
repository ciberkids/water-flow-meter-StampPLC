# Water Flow Meter Platform

Multi-workspace project that delivers the complete StampPLC water-flow meter experience: embedded firmware, a React/Vite mockup that simulates the display, exporter tooling that bridges the two, and the documentation/rules that keep every contribution aligned.

## Overview

- **Firmware** (`Water-Flow-Meter-PlatformIO/`) hosts the PlatformIO workspace that drives hardware inputs, LEDs, Modbus, and the runtime UI renderer. It consumes generated assets emitted by the mockup exporter.
- **UI Mockup + Exporter** (`web/mockup/`) renders a pixel-accurate StampPLC simulator, captures visual tests, and translates datasets into firmware-ready C++ (`src/ui/generated/` in the firmware repo).
- **Process & Requirements** (`docs/`) contain the operating rules, backlog, requirements, and hardware references that govern development.
- **Support assets** (`graphics/`, `web/backups/`, `water-flow-meter-StampPLC_Legacy/`, etc.) provide design artifacts, exporter backups, and historical firmware snapshots.

See `docs/Project definitions/Folder structure description.md` for the canonical folder-by-folder breakdown.

## Repository Layout

- `Water-Flow-Meter-PlatformIO/` – Main PlatformIO project (see its README for detailed layout, build/test commands, and Docker recipe).
  - `src/input`, `src/led`, `src/modbus`, `src/ui` mirror the runtime subsystems documented in `docs/Requirements`.
  - `platformio.ini` defines environments used by both local CLI builds (`pio run`) and Podman/Docker builds.
- `web/mockup/` – React workspace, Playwright/Cypress suites, and the `npm run export:firmware` translator.
  - `web/backups/` stores timestamped exporter backups, matching the strategy described in the mockup README.
  - `web/Water Flow Meter PlatformIO/` keeps a minimal firmware mirror for UI-side experiments.
- `docs/` – Project rules (`Project definitions`), functional requirements, mermaid diagrams, feature proposals, templates, backlog (“missing implementation” + “stories to implement”), hardware references, and archived stories.
- `graphics/` – SVG assets and rendered mockup images used across documentation and tests.
- `water-flow-meter-StampPLC_Legacy/` – Read-only legacy firmware kept for reference (single-file firmware plus license).
- `docker-compose.yml` – Spins up the mockup dev server container pointing at `web/mockup` (hosted on `5173`).
- `.codex/`, `.vscode/`, and other dotfiles – Editor/agent configuration shared by the team.

## Key Workflows

### Firmware development (`Water-Flow-Meter-PlatformIO/`)

- Build locally with PlatformIO:

  ```bash
  cd Water-Flow-Meter-PlatformIO
  pio run                 # default environment build
  pio test -d tests/build # compile-only smoke tests
  ```

- Containerized build (mirrors CI):

  ```bash
  cd Water-Flow-Meter-PlatformIO
  podman build -t stampplc-fw .
  podman run --rm -v $(pwd):/workspace stampplc-fw
  ```

- The UI runtime pulls assets from `src/ui/generated/` (overwritten by the exporter). Keep commits synchronized with the dataset used to produce them.

### UI mockup, testing, and exporter (`web/mockup/`)

- Local dev server:

  ```bash
  cd web/mockup
  npm install
  npm run dev -- --host 0.0.0.0 --port 5173
  ```

  Keyboard shortcuts emulate hardware buttons, and the design/help panels expose schema documentation alongside the live JSON snapshot.

- Visual regression & schema tests:

  ```bash
  npm run test:visual          # Playwright snapshots
  npm run test:visual:update   # Update baselines
  npm run test:exporter        # AJV + translator unit tests
  ```

- Exporter pipeline:

  ```bash
  npm run export:firmware
  ```

  This validates `screens.json`, backs up the previous firmware assets into `web/backups/ui/<timestamp>/`, and writes new headers/implementation files to `Water-Flow-Meter-PlatformIO/src/ui/generated/`. Use `--dry-run` or custom paths via the CLI in `tools/exporter/cli.ts` when needed.

- `docker-compose up web` (from repository root) launches the same dev server in a container with hot reload enabled.

### Documentation & governance (`docs/`)

- **Project definitions** – Coding conventions, tooling mandates, MCP rules, and VS Code workspace settings.
- **Requirements** – Functional specs (with `feature addition/` for incremental work and `UI_Firmware_Interface.md` for the renderer contract).
- **Missing implementation / stories to implement** – Active backlog items that feed sprint planning.
- **New feature proposal / templates** – Drafts plus reusable templates for future specs.
- **Hardware docs & diagrams** – Reference schematics and mermaid diagrams to keep firmware and UI in sync.

Keep these folders authoritative—link to them in issues/PRs instead of duplicating content elsewhere.

### Assets & legacy

- `graphics/svg` and `graphics/mockupimages` provide UI art and captured previews for documentation and automated tests.
- `water-flow-meter-StampPLC_Legacy/src/firmware.cpp` is preserved for comparison when porting or auditing behaviour.

## Getting Started (new contributors)

1. **Review the rules** – Read `docs/Project definitions/Folder structure description.md` plus any relevant requirements/stories before touching code.
2. **Install prerequisites**
   - PlatformIO CLI (or VS Code extension) and a working `pio` in your PATH.
   - Node.js 18+ with npm (needed for the mockup/exporter).
   - Docker/Podman (optional, but required for container builds or `docker-compose` workflows).
3. **Clone & inspect**
   ```bash
   git clone <repo-url> water-flow-meter
   cd water-flow-meter
   ```
   Use `ls` and the repository layout above to get oriented.
4. **Bring up the mockup workspace**
   ```bash
   cd web/mockup
   npm install
   npm run dev
   ```
   Visit `http://localhost:5173/` (or the port mapped by `docker-compose`) to exercise UI flows and tweak layouts/design tokens.
5. **Export UI assets into firmware**
   - Update `src/data/screens.json` or use the in-app editors.
   - Run `npm run export:firmware` to sync `Water-Flow-Meter-PlatformIO/src/ui/generated/`.
   - Commit both the dataset and regenerated assets together.
6. **Build/test the firmware**
   ```bash
   cd ../../Water-Flow-Meter-PlatformIO
   pio run
   pio test -d tests/build
   ```
   Alternatively, run the provided Podman/Docker workflow for a clean environment.
7. **Consult documentation while coding**
   - Requirements/specs live under `docs/Requirements`.
   - Active work items are tracked in `docs/missing implementation` and `docs/stories to implement`.
   - Hardware notes, diagrams, and templates live nearby.

Following these steps ensures new developers understand the governance docs, keep firmware/UI assets in sync, and can iterate with confidence across the entire platform.
