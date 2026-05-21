# Requirement: Display Web Mockup & UI Translation Pipeline

**Version:** 0.1  
**Date:** 2025-05-17

---

## 1. Purpose

Provide an interactive, code-generating design workflow that mirrors the StampPLC display UI on the web. The goal is to let designers, developers, and stakeholders preview layouts, test button-driven interactions, and export firmware-ready assets without needing hardware access. The feature also introduces a translation engine that converts web-authored animations and styles into optimized C++ artifacts, backed up and slotted cleanly into the firmware source tree.

---

## 2. Hardware / Interfaces

Summarize the physical or logical interfaces this feature touches.

| Component | Connection / Address | Notes |
| --- | --- | --- |
| StampPLC display subsystem | SPI LCD (per StampPLC specs) | Generated C++ output must map 1:1 to device resolution and timing. |
| React web application | Browser (desktop) | Provides simulated UI, button mapping, and export trigger surface. |
| Translation engine runtime | CLI / service invoked by UI | May be implemented in the most suitable language (e.g., Node.js, Python, Rust). |
| Firmware repository | `/src` tree | Receives generated C++ assets; requires structured folder layout for UI modules. |
| Backup storage | `/backups/ui/` (new) | Stores dated copies of superseded UI assets before regeneration. |

---

## 3. Behaviour Specification

### 3.1. Web Display Mockup Workspace

- **Intent:** Offer a faithful, 1:1-scale visualization of every StampPLC screen and transition.
- **Trigger:** Users load the web application and choose a screen or flow from the requirements catalogue.
- **Configuration:** Supports zoom controls while retaining pixel-accurate layout; designers can reposition or tweak elements on any template.

### 3.2. Button Interaction Simulation

- **Intent:** Emulate UP/DOWN/ENTER input to validate state machine behaviour and countdown workflows.
- **Trigger:** Virtual buttons (or keyboard bindings) in the web app trigger navigation events defined in the requirements.
- **Configuration:** Interaction scripts mirror firmware rules (debounce, long-press thresholds, wrap-around navigation).

### 3.3. UI Styling & Theming

- **Intent:** Allow rapid experimentation with colour schemes and animation styles.
- **Trigger:** Designers edit CSS tokens or select theme presets; results update immediately within the mockup.
- **Configuration:** CSS variables and animation descriptors kept modular to support fast restyling.

### 3.4. Translation Engine Export

- **Intent:** Convert the React + CSS definition into firmware-ready C++ assets.
- **Trigger:** User presses an “Export to Firmware” control in the web UI.
- **Configuration:** Engine outputs optimized drawing primitives, state tables, and animation schedules. Before writing new files, it copies the previous firmware UI bundle into a dated backup folder.

### 3.5. Firmware Refactor Alignment

- **Intent:** Isolate generated UI code from core logic so UI swaps do not destabilize polling, Modbus, or diagnostics.
- **Trigger:** Repository restructuring introduces a dedicated UI module (e.g., `/src/ui/generated/`), integration layer, and configuration hooks.
- **Configuration:** Build scripts and includes updated to compile regenerated assets without manual intervention.

### 3.6. Requirement Conformance Validation

- **Intent:** Ensure the new workflow maintains compliance with existing functional requirements.
- **Trigger:** After each export, automated checks confirm the generated UI honours documented behaviour (e.g., LED legend, countdown prompts).
- **Configuration:** Reports surface missing elements or behavioural mismatches for manual review.

---

## 4. Firmware Requirements

1. Restructure the `src` tree to separate core logic, UI integration, and generated assets (e.g., `src/ui/core`, `src/ui/generated`, `src/ui/theme`).
2. Provide an integration layer that loads generated UI descriptions while exposing a stable API (`UiRenderer`, `UiController`) to the rest of the firmware.
3. Create a CLI endpoint (e.g., `tools/ui-export`) invoked by the web app to run the translation engine and drop outputs into `src/ui/generated`.
4. Before writing new assets, copy the prior generated bundle into `backups/ui/<YYYYMMDD_HHMMSS>/`.
5. Ensure the generated code compiles with PlatformIO toolchain, adheres to memory constraints, and keeps the high-frequency polling task unaffected.
6. Maintain or improve existing Modbus-driven UI data bindings (e.g., sensor telemetry, diagnostics flags) after refactor.
7. Uphold LED coordination and factory-reset behaviours—UI refactor must not regress any requirements enumerated in `Project_document.md` §5.
8. Document the new folder structure in `docs/Project definitions/Folder structure description.md` and update build instructions accordingly.

---

## 5. UI / UX Requirements

- Web mockup renders a pixel-accurate (135×240) viewport with optional zoom and grid overlays.
- the Mock up will have a "frame by frame"feature to show the rendered screen frame by frame clearly in order to show case to the user how a screen changes at every redraw
- Provide a library of screen templates (Info pages, Configuration pages, countdown overlays) derived from existing requirements.
- Expose draggable/resizable elements with snap-to-grid alignment to preserve layout fidelity.
- Offer button simulation controls that display current state (short press, long press, repeat) and log transitions.
- Include theme editor panels for primary/secondary colours, typography tokens, and animation curves; support exporting theme presets.
- Present an “Export to Firmware” button with progress feedback, validation results, and links to generated backups.
- Provide documentation within the UI on how to interpret export warnings or validation failures.

---

## 6. Test Considerations

- Unit-test the translator to verify React components map to deterministic C++ drawing primitives and animations.
- Run integration tests ensuring generated firmware assets compile and render without runtime faults on hardware.
- Validate that button simulation timing mirrors firmware thresholds (short vs long press, repeats).
- Regression-test LED legends, countdown overlays, and diagnostics banners after each UI export.
- Stress-test export performance for complex animations; translation engine must prioritise runtime efficiency.
- Confirm backup archives are created reliably and can be restored to reproduce previous UI states.

---

## 7. Open Questions / Follow-Up

1. ✅ Translation engine will be implemented in **Node.js** to keep the pipeline aligned with the React authoring stack while still enabling performance-focused optimizations (worker threads, WASM modules).
2. ✅ The export flow will generate a **JSON intermediate** describing screens, timelines, and assets; a dedicated C++ emitter will consume this schema to produce firmware code, simplifying testing and versioning.
3. ✅ UI automation will run end-to-end in the web layer, while embedded validation focuses on ensuring the generated C++ hooks the correct firmware functions for button events and state transitions.
4. ✅ Firmware refactor can proceed immediately; with no production release yet, we can rearrange the source tree to create the required `src/ui/*` boundaries without migration risk.
5. ✅ Apply UX copy best practices: use concise, action-oriented labels (e.g., “Simulate Button Press”, “Export to Firmware”), progressive disclosure for warnings (“Export failed — view details”), and neutral tone for success states. Provide localisation hooks by storing all strings in a single dictionary, supporting future translations with ICU message format. Include accessibility considerations (ARIA labels, keyboard shortcuts) and document these conventions for consistency. (Owner: self)
6. ✅ Ownership remains with the single developer on the project; no additional handoff planning required.
