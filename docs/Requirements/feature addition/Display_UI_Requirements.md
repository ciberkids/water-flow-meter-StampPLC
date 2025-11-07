# Requirement: On-Device UI for Display and Buttons

**Version:** 0.1  
**Date:** TBD

---

## 1. Purpose

Define the functional requirements for leveraging the StampPLC’s integrated display and three hardware buttons to present live system metrics and allow local configuration while staying aligned with the Modbus feature set described in `docs/Requirements/Project_document.md`.

---

## 2. Hardware Context & Assumptions

- 2.4″ (or equivalent) integrated display capable of rendering static SVG frames supplied in `graphics/svg`. (Actual hardware: 1.14″ ST7789V2 @ 135 × 240, see `../../StampPLC specifications.md`).
- Three physical momentary buttons mapped to MCU GPIOs with interrupt support via the PI4IOE5V6408 expander (I²C SCL = GPIO15, SDA = GPIO13, INT = GPIO14, RST = GPIO3; key lines `KEYA/B/C`, see `../../StampPLC pin map.md`).
- One tri-colour status LED (RGB) driven from the same expander (`P6/P5/P4`) following the behaviour defined in `RGB_LED_Behavior.md`.
- Firmware already exposes sensor state, session counters, cumulative totals, polling rate, and configuration via Modbus registers.
- Polling task publishes `pollingRate_kHz`; this value must be used for Nyquist-style validation when saving sensor parameters.

---

## 3. Button Interaction Model

| Button | Short Press | Long Press (≥1.5 s) | Notes |
| :----- | :---------- | :------------------- | :---- |
| UP     | Navigate to previous page / increment field value | Accelerated increment when held (repeat every 250 ms) | Wraps around at list boundaries. |
| DOWN   | Navigate to next page / decrement field value | Accelerated decrement when held (repeat every 250 ms) | Wraps around at list boundaries. |
| ENTER  | Confirm selection / toggle booleans / open sub-menus | Acts as “Back/Exit”; also used to trigger guarded actions that require a countdown confirmation (see §4.3 & §5.4) | When a countdown is active, ENTER must remain held until it reaches zero to execute the action; releasing early cancels it. |
| UP + DOWN | — | Launch 30 s factory-reset countdown | After 3 s a warning overlay appears; holding for the full 30 s wipes NVS, clears Modbus config, and reboots. |

> Rationale: ENTER long press provides a universal escape from nested menus and lets the UI power down the display without extra buttons.

---

## 4. Display Info Mode

### 4.1. State Machine

| State | Entry Condition | Exit Condition | Purpose |
| :---- | :-------------- | :------------- | :------ |
| `Idle` | No button activity for 120 s | Any button press | Turns the display backlight off to prevent burn-in. |
| `Info` | Any button press while idle or at power-up | 3 s long-press ENTER (forces immediate idle) or explicit transition to Configuration Mode | Shows paged telemetry summaries. |

### 4.2. Propeller Indicator

- Right-hand side renders an SVG propeller.
- Animation cycles through pre-rendered frames at 6 fps while any sensor satisfies `isReady == true` AND `instantFlow_L_s > 0.0`.
- When all sensors report zero flow, show static “stopped” frame.

### 4.3. Telemetry Pages

Pages are circular; UP/DOWN step through them with wrap-around. Each page displays the selected metric for all eight sensors concurrently (column layout). Sensors that are disabled (`inUse == false`) render `--` with a footnote icon.

| Page ID | Title | Metric Source | ENTER (short) | ENTER (long) | Notes |
| :------ | :---- | :------------ | :------------- | :------------ | :----- |
| P1 | Instant Flow | Holding registers 101-116 (`instantFlow_L_s`) | No action | Back-to-idle countdown (3 s) | Also shows per-sensor status icons. |
| P2 | Cumulative Liters | Holding registers 103-110 (`cumulativeLiters`) | Start 30 s countdown to “Reset All Measured” command | Cancel (no action) | Countdown text must include “Hold ENTER to reset totals (30 → 0)”. |
| P3 | Cumulative Cubic Meters | Derived from P2 | Same as P2 | Cancel | |
| P4 | Session Liters | Holding registers 111-112 (`sessionLiters`) | Start 3 s countdown to sensor session reset | Cancel | Issues sensor-specific session reset for all ready sensors once timer hits zero. |
| P5 | Session Cubic Meters | Derived from P4 | Same as P4 | Cancel | |
| P6 | Max Flow Since Reset | Holding register 115 (`maxFlowSinceReset`) | Same as P4 | Cancel | |
| P7 | Enter Configuration | Text-only prompt | Start 3 s countdown to enter configuration mode | Cancel | Requires ENTER held throughout countdown. |

Additional requirements:
1. Countdown overlays display remaining seconds centrally; release ENTER before zero aborts.
2. During countdowns, UP/DOWN have no effect.
3. After a reset completes, trigger the appropriate Modbus command (`Reset Session`, `Reset All Measured`) so persisted state stays coherent.
4. Provide per-page helper text (bottom line) summarizing button hints (e.g., “Hold ENTER 3 s to reset session”).
5. Show a small status legend describing the RGB LED meanings (e.g., “Red pulses every X L • Green=Ready • Blue=Flow”).

---

## 5. Configuration Mode

### 5.1. Entry & Navigation

- Entry is granted when the P7 countdown completes while ENTER stays pressed.
- The first configuration page greets the user with “Configuration Mode” and a reminder that ENTER (long) exits back to info mode.
- UP/DOWN move between configuration pages; wrap-around enabled.
- ENTER short press edits the focused field. While editing:
  - UP/DOWN adjust the value according to the field’s step rules (see §5.2 & §5.3).
  - ENTER short press confirms the tentative value.
  - ENTER long press cancels the edit and restores the prior value.

### 5.2. Device-Level Settings

| Page | Label | Range / Options | Step / Action | Validation |
| :--- | :---- | :-------------- | :------------ | :--------- |
| C1 | Modbus Slave ID | 1–255 | ±1 (wrap around) | Immediate Modbus re-registration; reject duplicates during runtime if network constraints apply. |
| C2 | Baud Rate | {1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200} | Cycle list | Must restart Modbus server task to apply. |
| C3 | Parity | {None, Even, Odd} | Cycle list | Display summary (e.g., “8E1”). |
| C4 | Stop Bits | {1, 2} | Toggle | Combined with parity to show UART frame settings. |
| C5 | LED Pulse Volume | {1 L, 10 L, 100 L} | Cycle list | Maps to global register 31; updates red LED pulse trigger. |
| C6 | LED Pulse Period | 100–2000 ms | ±1 (accelerates per Rules) | Maps to global register 32; clamps input and previews flashing rate. |
| C7 | Sensor Select | 1–8 | ±1 | ENTER short opens sensor sub-menu (see §5.3). |

ENTER long press on any device-level page exits configuration mode and returns to Info mode (no additional countdown).

### 5.3. Sensor Sub-Menu

Once a sensor is selected from C5, the UI enters a scoped sub-menu with the following pages:

| Page | Label | Data Type | Range / Step | Behaviour |
| :--- | :----- | :-------- | :----------- | :-------- |
| S1 | Connected | Boolean | Toggle | Mirrors `Connected Sensors Bitmap` bit. |
| S2 | Multiplier (F) | Signed 16-bit | ±1 (accelerates per Rules) | Maps to register 121. |
| S3 | Adjust | Signed 16-bit | ±1 (accelerates per Rules) | Maps to register 122. |
| S4 | Max Flow (Q, L/min) | 0–65535 | ±1 (accelerates per Rules) | Maps to register 120. |

Rules:
1. Each short ENTER commits the displayed value to the Modbus register and updates in-memory config.
2. While editing numeric fields, sustained UP/DOWN holds accelerate step size:  
   - 0–700 ms: ±1 every 250 ms (base cadence).  
   - 700 ms–1.5 s: ±5 every 150 ms.  
   - >1.5 s: ±25 every 150 ms until released.
3. Long pressing ENTER anywhere in the sub-menu starts a 3 s “Save & Validate” countdown. If held to zero:
   - Compute theoretical pulse frequency `f_theoretical = max(0, Q * F + Adjust)` (in Hz).  
   - Check Nyquist condition: `pollingRate_kHz * 1000 >= 2 * f_theoretical`.  
   - If the condition fails, show prompt: “Sampling too slow. UP=Edit values, DOWN=Save anyway.”  
     - UP returns to the field without saving.
     - DOWN forces the save and raises a warning flag to be exposed over Modbus diagnostics.
4. Long ENTER release before countdown completion cancels without saving.
5. ENTER long press at S1 from an inactive sensor immediately exits back to C5 to avoid editing unused channels.

### 5.4. Persistence & Integration

- Successful saves (device or sensor-level) must propagate to the existing Modbus holding registers within the same task cycle.
- Configuration changes that impact NVS must reuse the existing `Preferences` namespace to maintain persistence across reboots.
- The UI shall respect `isConfigValid()` rules; invalid entries must be blocked with contextual error messages and cannot be saved unless the user explicitly overrides via the Nyquist warning path.

### 5.5. Exit Behaviour

- ENTER long press from any configuration page (outside active countdown) triggers a confirmation toast (“Hold ENTER to exit”). Releasing before 1.5 s cancels; holding to 3 s exits to Info mode and turns off the display (transition to Idle).

### 5.6. Diagnostic Feedback

- The firmware exposes a new read-only holding register at global address **30** (`Undersampling Flags`). Bit *n* (0–7) mirrors whether sensor *n* failed the Nyquist validation during the most recent save.
- Info pages display a warning badge next to any sensor whose bit is high; configuration pages surface the same state before edits are committed.
- Clearing the flag occurs automatically when a sensor passes validation on the next save, or when the user forces a save via the “IGNORE & SAVE” path.

---

## 6. Visual & UX Guidelines

1. Maintain consistent header/footer layout across modes (header = page title and hints; footer = button legend).
2. Use contrasting highlight color for the focused sensor or editable field.
3. Display units alongside numeric values (e.g., “L/s”, “L”, “m³”).
4. Provide error and success feedback to the user within 1 s of an action (e.g., “Session reset complete”).
5. Ensure the UI scales for all eight sensors without overlap; adopt a two-column layout (sensors 1-4 left, 5-8 right) with a 48 × 48 px reserved propeller window per sensor as documented in `../../StampPLC specifications.md`.
6. Surface LED status legend within info mode so operators understand current colour semantics.

---

## 7. Non-Functional Requirements

- **Responsiveness:** Button events must be acknowledged within 100 ms to feel immediate.
- **Low Power:** When the display is off, backlight current draw must fall below the platform’s documented standby target.
- **Localization-ready:** All user-facing strings should be routed through a central table to enable future translations.
- **Reliability:** Countdown confirmations must survive transient Modbus activity; e.g., configuration saves can’t be interrupted by external requests.

---

## 8. Follow-Up

1. Capture UX copy for the 30 s factory-reset warning and confirmation dialogs (multilingual).
2. Validate coarse-adjust acceleration values with real sensors to confirm they balance speed and precision.
3. Implement automated tests that verify the `Undersampling Flags` register clears when conditions improve.
4. Prototype RGB LED transitions (red pulse, green ready, blue flow) to ensure they remain visible without distracting from the UI.
