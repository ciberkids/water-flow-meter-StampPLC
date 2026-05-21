# Requirement: Display Web UI Workspace Improvements

**Version:** 0.1  
**Date:** 2025-11-11

---

## 1. Purpose

Elevate the StampPLC web workspace (Simulation, Design, Import/Export, Help tabs) from a visual mockup into a functional UI authoring and validation environment. The updated tool must simulate firmware interactions exactly as encoded in `screens.json`, expose callable firmware functions, allow screen and element editing (including animations, scroll bars, and value placeholders), and propagate those edits back into the JSON/schema and exporter pipeline. Designers should see which firmware actions fire for any event, edit content directly on canvas or via JSON, and trust that the exporter will generate code reflecting the enriched schema.

---

## 2. Hardware / Interfaces

| Component | Connection / Address | Notes |
| --- | --- | --- |
| Web mockup workspace (`web/mockup`) | Browser (React/Vite) | Hosts Simulation, Design, Import/Export, Help tabs. |
| Firmware function registry | `Water-Flow-Meter-PlatformIO/src/ui/core/ui_actions.*` | Export list of callable actions to the web tool for binding events. |
| Exporter CLI (`tools/exporter/cli.ts`) | Node.js process | Must parse enriched JSON schema, generate IR/C++ with new constructs, and surface validation results. |
| Screens JSON (`src/data/screens.json`) | File / schema | Gains new element types (value placeholders, scroll bars, animation assets) plus event/function metadata. |

---

## 3. Behaviour Specification

### 3.1 Simulation Instrumentation

- **Intent:** Make the Simulation tab a faithful emulator of the firmware UI so designers can trace interactions without hardware.
- **Trigger:** User presses simulated buttons, edits a value, or invokes a save/transition defined in the JSON.
- **Configuration:** Simulation reads the active dataset plus imported firmware action catalog. When an event fires, the UI displays the exact firmware function ID and parameters that would be dispatched. Value edits must update bindings and display deltas just like the actual device.

### 3.2 Design Editor Authoring

- **Intent:** Provide full-fledged layout editing inside the Design tab.
- **Trigger:** User inserts or modifies elements (text, box, value placeholder, animation/scroll bars), adds/removes screens, or assigns events.
- **Configuration:** Canvas interactions update the dataset immediately. A live JSON editor (with lint-style inline errors similar to Mermaid) stays in sync; invalid JSON highlights offending sections and blocks export until resolved.

### 3.3 Import / Export Enhancements

- **Intent:** Keep dataset, action registry, and generated firmware artifacts aligned.
- **Trigger:** User imports existing JSON or firmware function manifests, runs export, or downloads backups.
- **Configuration:** Import step accepts a function catalog emitted by the firmware (e.g., from `ui_actions.cpp`). Export step writes all new schema constructs and ensures the code generator emits runnable C++/IR for animations, scroll bars, and enriched events.

### 3.4 Help & Documentation Expansion

- **Intent:** Make the Help tab a self-contained reference.
- **Trigger:** User opens Help or needs clarification on schema fields/events.
- **Configuration:** Help content enumerates every JSON field (including the newly added ones) with examples, diagrams, and links to the function registry and event binding workflow.

---

## 4. Firmware Requirements

1. Provide a machine-readable manifest of firmware actions/handlers (e.g., derived from `UiActionRegistry`) that the web tool can import, including IDs, descriptions, and parameter schemas.
2. Extend the exporter schema so generated IR and C++ capture:
   - Value placeholders with read/write behaviour and default formatting.
   - Scroll bar widgets with configurable steps and current state binding.
   - Animation boxes referencing multi-frame SVG assets with per-frame metadata.
   - Event definitions (button/data/time) mapped to firmware functions plus optional parameters.
3. Ensure exporter validation fails if a referenced firmware function is missing from the imported manifest.
4. Update generated metadata to include event-function mappings so firmware can audit or debug bindings at runtime.
5. Preserve backward compatibility by versioning the schema (e.g., `schemaVersion`) and supporting migrations for existing datasets.

---

## 5. UI / UX Requirements

- **Simulation tab:**
  - Display a live “Function Trace” log listing every event, trigger source, and resolved firmware function (with parameters).
  - Allow inline editing of value placeholders with validation and highlight when edits deviate from source data.
  - Show real-time effect of transitions, including target screen previews and action hints.

- **Design tab:**
  - Toolbox for text, box, value placeholder, animation box, scroll bar, and SVG asset insertion/removal.
  - Screen manager to add/remove or duplicate screens; updates the dataset and scroll bar states automatically.
  - Hierarchical screen explorer showing menus/submenus as a collapsible tree; selecting a node focuses the corresponding screen collection and breadcrumb so designers always know their position.
  - Support authoring nested screen groups (e.g., submenu collections) with drag-and-drop reordering and depth-aware navigation; entering a submenu updates the tree view highlight and canvas context.
  - Event editor listing available events per screen (button, data, timeout) and letting users select firmware functions from the imported manifest.
  - Live JSON editor with immediate syntax/validation feedback (inline markers, message panel, cursor-to-error linking).
  - Animation inspector for selecting SVG frames, ordering them, and previewing sequences. Scroll bar editor defines number of steps, labels, and binding target for current index.

- **Import & Export tab:**
  - Input to upload firmware function manifests (JSON/YAML) with validation.
  - Export summary must confirm that enriched schema fields were emitted and list any downgraded features if firmware manifest lacks required functions.

- **Help tab:**
  - Expand reference section to describe every schema field, including new element/event types, with usage tables and cross-links to Simulation/Design features.

---

## 6. Test Considerations

- **Simulation parity tests:** Automated Playwright scripts verifying that button presses trigger the expected firmware function IDs (derived from imported manifest) and that value edits update traces/logs.
- **Design editor tests:** Visual regression and interaction tests covering element insertion, scroll bar configuration, animation frame selection, and round-trip JSON editing.
- **Schema/export tests:** Unit tests for the Node exporter ensuring new constructs render deterministically and that invalid function bindings fail with clear errors.
- **Import tests:** Validate that malformed firmware manifests are rejected with actionable messages.
- **Help content tests:** Snapshot tests confirming new documentation sections render and include required tables/examples.

---

## 7. Open Questions / Follow-Up

1. **Scroll bar semantics (answered):** Each scroll bar displays the current screen number; nested levels concatenate indices with `-` (e.g., `3` for top-level screen 3, `5-2` for the second screen inside submenu branch 5). Implementation must auto-update the numbering as designers move through the hierarchy.
2. **Firmware function manifest (answered):** Use the generated JSON schema emitted from a C++ helper that gathers exposed setters/getters via a dedicated design pattern. Requirements:
   - Maintain a canonical JSON schema definition checked into the repo.
   - Build a C++→JSON generator that outputs the manifest according to that schema.
   - Schedule a spike story to evaluate the design pattern refactor needed to expose UI/Modbus-accessible functions cleanly before codifying the generator.
3. **Undo behaviour (answered):** Standard browser undo (`Ctrl+Z`/`Cmd+Z`) is sufficient; no bespoke undo stack is needed beyond ensuring text inputs and JSON editor cooperate with native undo.
4. **SVG asset handling (answered):** Designers upload SVG files; the UI stores them alongside other generated assets via path references. During export, the engine reads each referenced SVG and emits the required C++ drawing data, so no base64 embedding is needed in the dataset itself.
5. **Transition effects (answered/pending library audit):** Workspace transitions must mirror the hardware display effects supported by the target library. Action item: investigate the firmware rendering library to document available effects/refresh semantics; follow up if additional references are required.
