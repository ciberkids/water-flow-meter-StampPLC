# Requirement: Design Tool Firmware Binding & Manifest

**Version:** 0.1
**Date:** 2026-01-23

---

## 1. Purpose

The current design tool allows visual layout of screens but lacks "live" integration with the firmware logic. This feature aims to bridge that gap by introducing a **Firmware Manifest**.
This allows the UI designer to:

1. **Bind Events:** Trigger firmware functions when buttons are pressed (e.g., "Save Config", "Increment Value").
2. **Bind Values:** Link UI elements (text/values) to firmware variables (e.g., "Flow Rate", "Total Volume").
3. **Define Navigation:** Specify screen transitions triggered by these events.

The ultimate goal is to generate C++ code that automatically wires these bindings, treating the UI as a fully integrated component of the firmware.

---

## 2. Hardware / Interfaces

| Component | Connection / Address | Notes |
| --- | --- | --- |
| **Design Tool (Web)** | `localhost` | Needs to load a `manifest.json` describing FW capabilities. |
| **Exporter** | CLI / Node.js | Generates `ui_events.cpp` / `ui_bindings.h`. |
| **Firmware** | ESP32 / PlatformIO | Implements the functions defined in the manifest. |

---

## 3. Behaviour Specification

### 3.1. Firmware Manifest Loading

- **Intent:** The design tool needs to know *what* the firmware can do without parsing C++ code directly.
- **Trigger:** Design Tool Startup.
- **Configuration:** A JSON manifest file (e.g., `firmware_manifest.json`) is loaded, listing:
  - **Actions/Events:** Functions callable by the UI (e.g., `saveConfiguration()`, `resetCounter()`).
  - **Values:** Data sources available for display (e.g., `getFlowRate()`, `getWifiStatus()`).

### 3.2. Event Binding (Input -> Action)

- **Intent:** Assign specific behavior to hardware buttons *per screen*.
- **Trigger:** User selects a Screen in the Design Tool.
- **Configuration:** A "Events" or "Interaction" panel allows mapping:
  - `Button A` -> `Action: Increment Value`
  - `Button B` -> `Action: Save & Exit` -> `Next Screen: Home`

### 3.3. Value Binding (Data -> Display)

- **Intent:** specific UI elements (like "Value" placeholders) should dynamically display firmware data.
- **Trigger:** User selects a "Value" element.
- **Configuration:** A property field "Data Source" allows selecting from the Manifest's Value list (e.g., `System.FlowRate`).

---

## 4. Firmware Requirements

1. **Manifest Definition:** Define a standard JSON schema for the manifest.
    - **MVP:** Manually maintained JSON file loaded by the UI tool.
    - **Future:** Auto-generated from C++ headers.
2. **Value Types:** Manifest must specify data types (`int`, `float`, `string`) for all Values to ensure correct formatting in the UI.
3. **Action Registry:** Firmware must implement a mechanism to register or expose these callbacks so the generated code can call them.
4. **Firmware-Driven Navigation:** The firmware must be able to trigger a screen change programmatically (e.g., "Alert Screen" on error, or "Input Screen" for logic discrimination).
5. **Value Update Loop:** Firmware must provide a way to push updates to the UI or allow the UI to poll specific values.

---

## 5. UI / UX Requirements

- **Import Manifest:** Button to load the manual `firmware_manifest.json` in the Design Tool.
- **Event Panel:** New sidebar panel to manage Button Bindings (Input -> Action -> Next Screen ID).
- **Value Binding:** Element Inspector "Data Source" dropdown, filtering by compatible types from the manifest.
- **Standalone Screens:** Support for specialized interaction screens (e.g., Input/Confirmation) used for logic discrimination.
- **Visual Indicators:** Show small icons/badges on elements that are bound to firmware data.

---

## 6. Test Considerations

- **Manifest Validation:** Ensure the tool handles missing/malformed manifests gracefully.
- **Broken Bindings:** What happens if a bound function is removed from the manifest? (Tool should warn).
- **Round-Trip:** Verify that exported JSON -> C++ -> Compiled Firmware correctly triggers the action.

---

## 7. Implementation Roadmap (Draft Stories)

1. **Manifest Schema & Loader:** Define the JSON structure (Actions, Values, Types) and implement the loader in the Design Tool.
2. **Event Binding UI:** Add ability to bind Button Events to Manifest Actions and define transitions.
3. **Value Binding UI:** Add ability to link Text/Value elements to Manifest Values.
4. **Firmware-Driven Navigation:** Implement logic for the firmware to force a screen render (e.g., for Alerts).
5. **Exporter Updates:** Generate `ui_events.cpp` and `ui_bindings.h` from the new bindings.
