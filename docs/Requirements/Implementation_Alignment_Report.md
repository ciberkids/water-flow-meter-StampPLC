# Implementation Alignment Report

## Context

This note captures the current state of the firmware in `src/firmware.cpp` versus the requirements defined in `docs/Requirements/Project_document.md` (v1.0, Oct 24 2025). The goal is to highlight what already matches, what is still missing, and which technical risks should be addressed next.

## Confirmed Alignment

1. **Multi-sensor support** – the code defines `NUM_SENSORS 8` and maintains per-channel state (`SensorData`/`SensorCharacteristics`), matching the eight-input requirement.
2. **Dual-core architecture** – `pollingTaskCode` is pinned to core 0 with elevated priority, while `logicTaskCode` (Modbus + processing) runs on core 1, fulfilling the dedicated high-frequency polling mandate.
3. **Flow computation logic** – when a sensor configuration is considered ready, the implementation converts pulses to Hz and then to L/min using the documented formula `Flow = (Freq - Adjust) / F`, with clamping at `Qmax`.
4. **Persistent cumulative totals** – the firmware loads/saves cumulative liters per sensor via ESP32 `Preferences`, guarding the non-volatile storage assumption.
5. **Session management APIs** – reset commands (`session`, `measurement`, `config`) are routed to helper functions that zero the appropriate data structures, reflecting the behaviour described in the Modbus register map.
6. **Polling-rate telemetry** – the polling task tracks loop iterations and publishes `pollingRate_kHz`, satisfying the diagnostics requirement.
7. **Modbus register map** – the firmware now exposes the documented holding-register table through eModbus worker callbacks, supporting FC03/FC06/FC16 traffic for the entire address space.
8. **Word-accurate telemetry** – floats and doubles are written to the register map with explicit MSW-first packing, so Modbus masters can decode instantaneous and accumulated values without ambiguity.
9. **Readiness gating** – when a channel is disabled or fails validation, the exported measurement registers are forced to zero while internal counters pause, matching the specification’s “values remain 0” rule.
10. **Configuration validation** – `isConfigValid` enforces bounds on `Adjust` relative to `Q` and `F`, avoiding runaway results from unrealistic calibration data.

## Gaps & Risks

- Remaining risks are predominantly operational:
  * Modbus handling has not yet been exercised against a real master (timing, multi-register writes, broadcast behaviour).
  * Cross-core access to shared data still relies on cooperative scheduling; heavy Modbus traffic could expose race conditions without further stress testing.
  * Nyquist validation and RGB LED signalling should be verified on hardware to confirm that the new diagnostics map cleanly to physical indicators.

## Next Actions

1. Exercise the new Modbus register map with an integration test (FC03/FC06/FC16) to confirm word ordering and command semantics on hardware.
2. Run a soak test with multiple sensors active to observe any cross-core contention or missed pulses under sustained Modbus load.
3. Validate the `Undersampling Flags` behaviour and RGB LED state machine on-device to ensure they remain responsive under real polling rates.
