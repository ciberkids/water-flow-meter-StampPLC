# Requirement: RGB Status LED Behaviour

**Version:** 0.2
**Date:** 2026-07-30

> **0.2** — adds §3.4 (boot progress "snake") and §3.5 (reset countdown ramp, replacing
> "LEDs off during the countdown"). Both are single-LED colour sequences, not multi-LED
> chases: the hardware is one tri-colour LED, so a "snake" cycles the three channels and
> "all three on" is white.

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
- **Behaviour:** Blinks (250 ms on / 250 ms off) whenever the aggregated instantaneous flow across all ready sensors exceeds 0.0 L/min; remains OFF during idle.
- **Debounce:** A 500 ms hold-off avoids flicker when flow is intermittent; the blue LED remains on for at least one full cycle after the last pulse.

### 3.4. Boot Progress — Channel Snake

- **Intent:** Show that the controller is alive and initialising, and mark the exact moment
  it is ready to count pulses. Today boot is silent, so a device that hangs during
  initialisation looks identical to one that is simply idle.
- **Behaviour:** From `setup()` until the polling task reports its first
  `pollingRate_kHz`, cycle the single channels in a fixed order — **Red → Green → Blue →
  (repeat)** — one channel lit at a time, 150 ms per step. Because the hardware is one
  tri-colour LED rather than three in a row, the "snake" is a colour rotation, not a
  travelling dot.
- **Completion:** On the first published polling rate, the snake stops and the LED hands
  over to the normal §3.1–§3.3 behaviour. If initialisation has not completed within 10 s,
  the snake changes to **red only, 500 ms blink**, so a hang is visibly different from a
  slow start.
- **Constraint:** Boot animation must never delay the polling task. It is driven from the
  core-1 logic task, and the transition point is a read of the polling rate rather than a
  handshake, so core 0 is never blocked waiting on the LED.

### 3.5. Reset Countdown — Accelerating Ramp to Solid White

- **Intent:** Make a destructive countdown unmistakable, and confirm acceptance without the
  operator having to read the screen. Applies to every reset confirm screen: factory reset,
  reset totals and reset session (`Display_UI_Requirements.md` §4.3.1).
- **Behaviour while the countdown runs:** All three channels blink **in unison** (white
  flashes), and the blink rate **accelerates in step with the on-screen countdown** so that
  the LED and the displayed seconds tell the same story. The period is derived from the
  fraction of the countdown remaining:

  `periodMs = kMinPeriodMs + (kMaxPeriodMs - kMinPeriodMs) * (remainingMs / totalMs)`

  with `kMaxPeriodMs = 600` at the start and `kMinPeriodMs = 60` as it approaches zero.
  Deriving the period from the *fraction* rather than the absolute time means the same ramp
  works for a 3 s countdown and a 30 s one without tuning.
- **Behaviour on acceptance:** When the countdown reaches zero and the action is executed,
  all three channels go **solid ON (white)** and stay there for the duration of the
  acknowledgement toast (2 s, §4.3.1), then release to normal behaviour. Solid white is the
  signal that the reset was *accepted*, distinct from the flashing that means *pending*.
- **Behaviour on abort:** If the operator releases ENTER before zero, the ramp stops
  immediately and the LED returns to normal §3.1–§3.3 behaviour with no white flash — so an
  aborted reset never looks like a completed one.
- **Precedence:** The reset ramp overrides the red/green/blue channel semantics for its
  duration. This replaces 0.1's "turns all LED channels off during the countdown", which
  made the most dangerous operation in the system the one with the least indication.
- **Factory reset:** after solid white, the device reboots, so the LED naturally continues
  into the §3.4 boot snake — giving a coherent "accepted → restarting → ready" sequence.

---

## 4. Modbus & Firmware Requirements

1. New global holding registers (31 & 32) must be exposed for Modbus masters and persisted via `Preferences` alongside other configuration.
2. Register writes to 31/32 should mirror immediately into the LED task and update UI state without requiring a reboot.
3. Firmware must track cumulative session liters across all sensors to drive red pulses; pulses are independent of individual sensor resets (only the “master reset all session values” command clears the accumulator).
4. LED updates must run on the application/core-1 task loop without starving Modbus I/O. Reuse the expander write batching mechanism to avoid I²C contention with button polling.
5. `LedController` gains two override states that pre-empt the channel semantics of §3.1–§3.3:
   **boot** (§3.4) and **reset ramp** (§3.5). `setSuspended()` already exists for the old
   "all off during factory reset" behaviour and is replaced by the ramp.
6. The reset ramp needs the countdown's remaining and total milliseconds, so
   `UiCountdownState` must carry `totalMs` alongside `secondsRemaining` — a whole-second
   value cannot drive a 60 ms blink period.
7. The boot snake ends on the first published `pollingRate_kHz`, which must therefore be
   readable by the logic task before the first sensor calculation cycle.

---

## 5. UI Requirements

- **Info Mode:** Display a small legend describing current LED semantics (e.g., “Red pulses per X L • Green=Ready • Blue=Flow”).
- **Configuration Mode:** Add a device-level settings page that allows selecting the red volume step (1/10/100) and editing the pulse period using the coarse increment rules.
- **Factory Reset:** Factory reset returns registers 31/32 to defaults. The trigger is the P6 → `Factory reset?` confirm screen (30 s hold); the former blind UP+DOWN 30 s combo was retired — see `Display_UI_Requirements.md` §3.3. During the countdown the LED runs the §3.5 accelerating ramp and goes solid white on acceptance; it no longer goes dark.

---

## 6. Test Considerations

- Verify LED transitions under edge cases (sensor enable/disable, configuration invalidation, flow spike, session reset).
- Confirm Modbus writes to 31/32 are reflected both in LED behaviour and the on-device UI.
- Ensure long-running pulses do not introduce noticeable latency in button responsiveness.
- Verify the §3.5 ramp accelerates smoothly across both a 3 s and a 30 s countdown from the
  same formula, reaches solid white only on acceptance, and shows no white flash on abort.
- Verify the §3.4 boot snake starts at power-on, ends when measurement begins, and degrades
  to a red blink if initialisation exceeds 10 s.
- Confirm the boot animation never delays the core-0 polling task.

