# UI Firmware Integration Guide

**Scope:** Explains how the generated UI bundle (`web/mockup` exporter) is consumed by the firmware after the SI-20250517-05 refactor. Use this document when wiring new exporter fields or adjusting the embedded renderer.

## 1. Asset Pipeline Overview

> **`screens.json` is GENERATED.** This section used to open "Designers edit `screens.json`", which
> stopped being true when the dataset became derived from the per-screen requirement files plus the
> firmware catalogue. Editing it directly works until the generator next runs, and then CI reports a
> diff nobody expected. See **UI_Dataset_Contract.md** for what each JSON is, who owns it, and which
> gate enforces each edge — and edit the requirement file or `generate.mjs`, not the dataset.

1. Geometry is authored per screen in `docs/Requirements/feature addition/screens/*.json`;
   `web/mockup/tools/skeleton/generate.mjs` combines it with `actionManifest.json` (itself generated
   from the firmware catalogues) to produce `screens.json`. Then **Export to Firmware**.
2. The Node.js translator emits:
   - `src/ui/generated/GeneratedUi.{h,cpp}` — strongly typed tables (`ui_exporter::Screen`, `Theme`, etc.).
   - `src/ui/generated/ui_export_ir.json` — debugging IR describing the same content.
   - `src/ui/generated/ui_export_metadata.json` — provenance for audits.
3. Firmware consumes these assets via the `ui::UiAssets` loader (see `src/ui/core/ui_module.{h,cpp}`).

## 2. Runtime Interfaces

| Layer | Responsibilities | Public API |
| --- | --- | --- |
| `ui::UiAssets` | Binds exporter output to runtime | `loadGeneratedAssets()` returns pointers to `ui_exporter::Screen[]`, `Theme`, and `Metadata`, plus a bound `ThemePalette`. |
| `ThemePalette` (`src/ui/theme/theme_palette.*`) | Resolves ARGB tokens + typography | `color(key)`, `typographyBase/Value/Badge`, `animationEasing()`. Renderer converts ARGB to RGB565 via `UiRenderer::toRgb565`. |
| `UiController` (`src/ui/core/ui_controller.*`) | Maps sensor/Modbus data into a `UiRenderContext` consumed by the renderer. Inputs: `LedController`, Modbus register structs, countdown state. |
| `UiScreenRouter` (`src/ui/core/ui_screen_router.*`) | Maps logical modes (Info / Config / overlays) to exporter-defined `ui_exporter::Screen` IDs. Keeps the renderer + interaction handler decoupled from concrete screen names. |
| `UiBindingResolver` (`src/ui/core/ui_bindings.*`) | Converts element `bindingId` strings into live strings derived from `UiRenderContext` (telemetry, sensor summaries, countdown text, etc.). |
| `UiRenderer` (`src/ui/core/ui_renderer.*`) | Draws context to the LCD using the generated `ui_exporter::Screen` definitions. `applyTheme()` must be called before `begin()` to feed palette colours. |
| `UiActionRegistry` (`src/ui/core/ui_actions.*`) | Maps `actionId` strings from `ui_exporter::Flow` entries to firmware callbacks (navigation, saves, resets). InteractionHandler delegates button events through this registry. |

`firmware.cpp` keeps a single `const ui::UiAssets kUiAssets` instance:

```cpp
const ui::UiAssets kUiAssets = ui::loadGeneratedAssets();
...
void setup() {
  M5StamPLC.begin();
  uiRenderer.applyTheme(kUiAssets.palette);
  uiRenderer.begin();
}
```

Future exporters can evolve schema as long as `UiAssets` remains the boundary. Add new helpers under `src/ui/theme/` (e.g., easing curves) and extend `UiRenderer::applyTheme()` to read them.

### 2.1. Binding & Action Hooks

- **Data bindings (`binding` field):** every UI element can declare a `binding` identifier in `screens.json`. The exporter copies those identifiers into `ui_exporter::Element::bindingId`. At runtime, `UiBindingResolver` turns the binding ID into live strings by reading the current `UiRenderContext`. Examples:
  - `page.title`, `telemetry.total`, `legend.warning`
  - `sensor.1` … `sensor.8` map to per-channel summaries, respecting the active `UiPage`.
  - `countdown.value`, `countdown.message` render the currently active hold-to-confirm overlay.
- **Flow triggers (`flows[]`):** each screen can define button/data/time triggers. The exporter emits typed metadata (`FlowTrigger`, `FlowButton`, `FlowGesture`, `timeoutMs`, `actionParams`) that the firmware consumes without hard-coded switch statements.
- **Action registry:** flows reference firmware actions by string ID (e.g., `ui.action.page.next`). `UiActionRegistry` maps those IDs to real callbacks implemented in `src/ui/core/ui_actions.*`. The catalogue below is generated from `src/ui/core/ui_action_catalogue.h`, which `static_assert` already reconciles with the handler table — so an action listed here has a handler, and one with a handler is listed here:

<!-- BEGIN GENERATED ACTION TABLE — node tools/wiki/gen-actions.mjs --write -->

*18 actions, generated from `src/ui/core/ui_action_catalogue.h`. Do not edit by hand:
CI regenerates this table and fails on any difference.*

| Action ID | Designer label | What it does |
| --- | --- | --- |
| `ui.action.page.next` | Next page | Advances to the next UI page |
| `ui.action.page.previous` | Previous page | Returns to the previous UI page |
| `ui.action.mode.idle` | Enter idle | Dims the display and enters idle mode |
| `core.action.save-config` | Save configuration | Persists the current configuration block to NVS and Modbus registers |
| `core.action.reset-session` | Reset session counters | Issues the Reset Session Modbus command for all ready sensors |
| `core.action.reset-all-measured` | Reset all measured totals | Issues the Reset All Measured Modbus command |
| `core.action.reset-max-flow` | Reset peak flow | Issues the Reset Max Flow Modbus command; clears the volatile peak and nothing else |
| `core.action.reset-calibration` | Reset calibration | Returns the SELECTED channel's calibration to defaults so a replacement meter can be entered; cumulative, session and peak readings are kept |
| `core.action.factory-reset` | Factory reset | Wipes NVS, clears Modbus config, and reboots |
| `core.action.reset-portal-login` | Reset portal login | Restores the configuration portal login to admin/admin, leaving totals, calibration and network settings untouched (R8.2a) |
| `ui.action.nav.descend` | Descend one level | Push the flow's target level onto the navigation stack |
| `ui.action.nav.back` | Back one level | Pop one level off the navigation stack |
| `ui.action.nav.escape` | Escape to main screen | Clear the navigation stack back to P0, discarding any uncommitted edit |
| `config.action.value.increment` | Increment value | Raise the pending value by its step, with hold acceleration |
| `config.action.value.decrement` | Decrement value | Lower the pending value by its step, with hold acceleration |
| `config.action.value.commit` | Commit value | Clamp, write the mapped register, validate, then ascend one level |
| `config.action.value.commit-override` | Save despite Nyquist warning | Forces the pending value past a failed Nyquist check and raises bit n of register 30 (§5.5) |
| `config.action.value.discard` | Discard edit | Abandon the pending value and ascend one level |

<!-- END GENERATED ACTION TABLE -->

Adding a new behaviour only requires:
1. Declare the flow in `screens.json` with the desired trigger + `actionId`.
2. Register a matching handler inside `ui_actions.cpp` (or supply a different registry).

`InteractionHandler` now routes button events entirely through those flow definitions, so navigation/selections live alongside the UI description rather than being duplicated in firmware.

## 3. Compatibility Rules

- Generated headers must avoid dynamic allocation; firmware treats them as `constexpr`-friendly tables.
- Any breaking change to `ui_exporter::*` structs must be mirrored in both the translation engine and this document.
- Renderer logic should only rely on abstracted palette + context, never parse JSON files at runtime.
- Keep `UiController` free of exporter-specific logic so future screen sets can plug in without firmware recompilation beyond asset refresh.

## 4. Testing & Validation

- Run `pio test -e build` (see project README) to ensure the reorganized code compiles against PlatformIO.
- Run `npm run test:visual` in `web/mockup` before exporting to verify visual diffs.
- After every firmware export, keep the generated metadata files for traceability; they are attached to release notes.
