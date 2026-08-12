# Story: Exporter Binding Logic (C++ Generation)

**ID:** SI-20260123-04
**Parent Feature:** NF-20260123-01 (Firmware Binding)
**Dependencies:** SI-20260123-02, SI-20260123-03

## 1. Goal

Update the `npm run export:firmware` script to generate the C++ glue code that connects the JSON bindings to the actual Firmware Functions.

## 2. Requirements

### 2.1 Event Bindings Output

- **Target File:** `ui_events.cpp` (or similar).
- **Logic:** Generate a switch/case or lookup table that:
  - On `SCREEN_LOAD`, registers the button callbacks for that screen.
  - When Callback triggers -> call `ManifestAction()` -> if `nextScreen` defined, call `ScreenManager::load(nextScreen)`.

### 2.2 Value Bindings Output

- **Target File:** `ui_bindings.h` / `ui_bindings.cpp`.
- **Logic:**
  - Generate a helper to fetch data: `updateScreenValues()`.
  - Function maps `ElementID` -> `ManifestValue()`.
  - Called by Firmware Loop to refresh the screen data.

### 2.3 FW-Driven Navigation Support

- **Interface:** Expose `void loadScreen(int screen_id)` in the generated header so Firmware can call it programmatically (Requirement 4 of Feature Proposal).

## 3. Implementation Tasks

- [ ] Analyze current Exporter logic (`tools/exporter/`).
- [ ] Implement `EventGenerator`: Iterates screens, finds events, generates C++ handler callbacks.
- [ ] Implement `ValueGenerator`: Iterates screens, finds bound elements, generates update mapping.
- [ ] Update C++ boilerplate templates to include these new files.

## 4. Verification Plan

- **Manual:** Run exporter on a project with bindings.
- **Verify:** Inspect generated `.cpp` files.
- **Verify:** Code compiles (mocking the firmware functions if needed).
