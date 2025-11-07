# Requirement: RGB Status LED Behaviour

**Version:** 0.1  
**Date:** TBD

---

## 1. Purpose

Define the behaviour, configuration, and firmware/UI integration for the StampPLC’s on-board RGB LED that is routed through the PI4IOE5V6408 expander (pins `P6/P5/P4`). The LED provides at-a-glance feedback on flow activity, configuration health, and cumulative production totals.

---

## 2. Hardware Mapping

| Channel | Expander Pin | Electrical Notes |
| --- | --- | --- |
| Red   | PI4IOE5V6408 `P6` | Sinks through status LED anode; active-high logic. |
| Green | PI4IOE5V6408 `P5` | Active-high; shares expander interrupt line on GPIO14. |
| Blue  | PI4IOE5V6408 `P4` | Active-high; PWM-capable via expander software bit-banging if needed. |

The expander is addressed at `0x43` over I²C (SCL = GPIO15, SDA = GPIO13). All LED updates must be non-blocking and coexist with button scanning on the same device.

---

## 3. Behaviour Specification

### 3.1. Red Channel — Production Pulse

- **Intent:** Provide a tactile indicator proportional to total volume processed across all sensors.
- **Trigger:** Totalized session counter (`Σ sessionLiters`) crossing configurable volume quanta.
- **Configuration:**
  - `Global Holding Register 31` – **LED Red Volume Step** (`uint16_t`, R/W)  
    | Value | Meaning | Default |
    | --- | --- | --- |
    | `1` | Pulse every 1 liter | ✓ |
    | `10` | Pulse every 10 liters |  |
    | `100` | Pulse every 100 liters |  |
  - `Global Holding Register 32` – **LED Red Pulse Period (ms)** (`uint16_t`, R/W)  
    Defines the full on/off cycle time while pulsing (minimum 100 ms, default 500 ms). The LED is on for 40 % of the cycle, off for the remainder.
- **UI Integration:** Configuration Mode → Device Settings → “LED Pulse” page exposes both registers with wrap-around selection.

### 3.2. Green Channel — Configuration Health

- **Intent:** Convey whether all sensors are valid and ready.
- **Behaviour:** Solid ON when every active sensor has `isReady == true` and no pending configuration errors; OFF otherwise.
- **Edge Cases:** During a configuration save countdown the LED is dimmed (optional PWM at 25 % duty) until validation completes.

### 3.3. Blue Channel — Live Flow Activity

- **Intent:** Highlight real-time flow detection.
- **Behaviour:** Blinks (250 ms on / 250 ms off) whenever the aggregated instantaneous flow across all ready sensors exceeds 0.0 L/s; remains OFF during idle.
- **Debounce:** A 500 ms hold-off avoids flicker when flow is intermittent; the blue LED remains on for at least one full cycle after the last pulse.

---

## 4. Modbus & Firmware Requirements

1. New global holding registers (31 & 32) must be exposed for Modbus masters and persisted via `Preferences` alongside other configuration.
2. Register writes to 31/32 should mirror immediately into the LED task and update UI state without requiring a reboot.
3. Firmware must track cumulative session liters across all sensors to drive red pulses; pulses are independent of individual sensor resets (only the “master reset all session values” command clears the accumulator).
4. LED updates must run on the application/core-1 task loop without starving Modbus I/O. Reuse the expander write batching mechanism to avoid I²C contention with button polling.

---

## 5. UI Requirements

- **Info Mode:** Display a small legend describing current LED semantics (e.g., “Red pulses per X L • Green=Ready • Blue=Flow”).
- **Configuration Mode:** Add a device-level settings page that allows selecting the red volume step (1/10/100) and editing the pulse period using the coarse increment rules.
- **Factory Reset:** The UP+DOWN 30 s factory reset returns registers 31/32 to defaults and turns all LEDs off during the countdown.

---

## 6. Test Considerations

- Verify LED transitions under edge cases (sensor enable/disable, configuration invalidation, flow spike, session reset).
- Confirm Modbus writes to 31/32 are reflected both in LED behaviour and the on-device UI.
- Ensure long-running pulses do not introduce noticeable latency in button responsiveness.

