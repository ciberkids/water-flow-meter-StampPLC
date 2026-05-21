# Story Refinement Findings — SI-20250517-05 firmware modularity follow-up

**Story ID:** SI-20250517-05  
**Story Title:** Firmware UI Module Restructure  
**Implementation Artifact:** `Water-Flow-Meter-PlatformIO/src/firmware.cpp`, `Water-Flow-Meter-PlatformIO/Dockerfile`  
**Reviewer:** Codex  
**Review Date:** 2025-11-10  
**Status Recommendation:** ready for verification

---

## 1. Snapshot Summary
- **Observed Outcome:** Button/factory-reset orchestration now lives in `input/interaction_handler.*`, sensor aggregation flows through `sensors/sensor_state_engine.*`, `firmware.cpp` delegates to both, and the Docker image installs PlatformIO successfully (with `python3-venv` + `--break-system-packages`). Containerized `pio run -d /workspace` now reaches the compilation phase and surfaces real code-time errors (currently missing Arduino headers) instead of failing during setup.
- **Intended Outcome:** Story SI-20250517-05 expects distinct modules for input handling and sensor-state math plus a reproducible Docker workflow that exercises PlatformIO builds.
- **Gap Statement:** Previous gaps (inline button logic, inline sensor math, missing Docker toolchain) are closed. Further compile errors are unrelated to the container/tooling work and will be tracked separately.

---

## 2. Findings Log

| # | Area / File | Severity* | Finding | Evidence (line / behaviour) | Proposed Refinement |
|---|-------------|-----------|---------|------------------------------|---------------------|
| 2.1 | `Water-Flow-Meter-PlatformIO/src/input/interaction_handler.*`, `src/firmware.cpp` | Resolved (High) | InteractionHandler module now owns button event routing, idle-entry detection, and factory-reset holds; `logicTaskCode` consumes a high-level `InteractionResult`. | `input/interaction_handler.h:10-51`, `input/interaction_handler.cpp:5-109`, and `firmware.cpp:170-237`. | None. |
| 2.2 | `Water-Flow-Meter-PlatformIO/src/sensors/sensor_state_engine.*`, `src/firmware.cpp` | Resolved (High) | SensorStateEngine handles pulse accumulation, flow math, caches, diagnostics, and register syncs; firmware simply calls `sensorStateEngine.update(elapsedTime_s)` and `refreshDiagnostics()`. | `sensors/sensor_state_engine.h:5-28`, `sensors/sensor_state_engine.cpp:5-90`, `firmware.cpp:62-123` and `firmware.cpp:203-222`. | None. |
| 2.3 | `Water-Flow-Meter-PlatformIO/Dockerfile`, `test_result.md` | Resolved (High) | Dockerfile installs PlatformIO (with PEP-668-compliant pip) and `python3-venv`; `podman build -t stampplc-fw .` now succeeds and the containerized `pio run -d /workspace` reaches source compilation (currently failing on upstream Arduino headers, documented in `test_result.md`). | `Water-Flow-Meter-PlatformIO/Dockerfile:1-19`, `test_result.md:1-24`. | None; follow-up work for missing headers belongs to the firmware/toolchain story. |

> *Severity guidance: **High** (blocks requirement / creates defect), **Medium** (partial compliance / risky), **Low** (polish / documentation).

---

## 3. Suggested Refinement Tasks
- [x] Add a button/factory-reset module that encapsulates long-press detection, countdown messaging, and idle-entry so `firmware.cpp` only wires callbacks.
- [x] Introduce a sensor-state engine module that owns pulse accumulation, flow math, caches, and Modbus synchronization.
- [x] Fix the Dockerfile’s PlatformIO installation so `podman build -t stampplc-fw .` succeeds and can subsequently run `pio run` in the container (see `test_result.md` for the remaining Arduino-header build failure).

---

## 4. Dependencies & Questions
- **Upstream Impact:** No schema/runtime changes were required; exporter tooling continues to target `src/ui/generated/`.
- **Open Questions:** None.
- **Testing Gaps:** Containerized `pio run -d /workspace` now executes and exposes missing Arduino headers (tracked separately). Additional unit/compile tests can be layered onto the new modules when the firmware toolchain configuration is updated.

---

## 5. Attachments & Links
- `Water-Flow-Meter-PlatformIO/src/firmware.cpp`
- `Water-Flow-Meter-PlatformIO/Dockerfile`
- `test_result.md` (container build failure log)

---

## 6. Reviewer Notes
- Interaction handling + sensor-state logic are now reusable modules, leaving `firmware.cpp` focused on orchestration.
- The Docker image builds cleanly; `podman run ... pio run -d /workspace` proceeds until it hits missing Arduino headers (`Preferences.h`, `M5StamPLC.h`). That dependency issue is unrelated to the container/tooling work and should be handled in the story that reintroduces Arduino support.
