// Host tests for SensorStateEngine's readiness aggregation.
//
// The green LED is the operator's only at-a-glance signal that the device is configured and
// working. It was unreachable on any installation using fewer than eight channels, which is
// every realistic installation, and nothing caught it because this engine had no test.
#include "sensors/sensor_state_engine.h"

#include <cstdio>

#include "modbus/modbus_manager.h"

// The engine calls three ModbusManager methods behind a null-pointer guard, so the harness
// only needs them to LINK. Defined here rather than by compiling modbus_manager.cpp, which
// would drag eModbus into a build that promises no PlatformIO.
void ModbusManager::syncSensorToHolding(std::size_t) {}
void ModbusManager::syncGlobalRegisters() {}
void ModbusManager::evaluateSensorDiagnostics() {}

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const char* what) {
  ++checks;
  std::printf("  %-66s %s\n", what, condition ? "ok" : "FAIL");
  if (!condition) ++failures;
}

/** Runs the engine over a given enable/ready pattern and reports the aggregated readiness. */
bool readinessFor(std::size_t count, const bool* inUse, const bool* isReady) {
  SensorData sensors[plc::kNumSensors] = {};
  SensorCharacteristics configs[plc::kNumSensors] = {};
  double totalLiters = 0.0;
  double aggFlow = 0.0;
  bool allReady = false;
  uint16_t undersampling = 0;

  for (std::size_t i = 0; i < count; ++i) {
    sensors[i].inUse = inUse[i];
    sensors[i].isReady = isReady[i];
    configs[i].q_max = 100;
    configs[i].f_multiplier = 1;
  }

  plc::SensorStateEngine::Dependencies deps;
  deps.sensors = sensors;
  deps.configs = configs;
  deps.sensorCount = count;
  deps.registerBank = nullptr;
  deps.modbusManager = nullptr;  // guarded by a null check in update()
  deps.totalSessionLitersCache = &totalLiters;
  deps.aggregateFlowLpsCache = &aggFlow;
  deps.allSensorsReadyCache = &allReady;
  deps.undersamplingFlags = &undersampling;

  plc::SensorStateEngine engine(deps);
  engine.update(1.0f);
  return allReady;
}

/**
 * A disconnected sensor must contribute nothing to anything reported (owner request, 2026-08-05).
 *
 * The engine gates on `sensor.inUse`, and the point of these checks is that the gate covers the
 * AGGREGATES too — not just the per-sensor numbers. A disabled channel carrying stale volume that
 * still summed into totalSessionLiters would be the worst kind of wrong: a plausible total, with no
 * per-sensor row to trace it back to.
 *
 * Breaks if: the `if (sensor.inUse)` guard is removed, or the aggregate sums move outside it.
 */
void disconnectedSensorTests() {
  std::printf("\n[a disconnected sensor contributes nothing — owner request]\n");

  SensorData sensors[plc::kNumSensors] = {};
  SensorCharacteristics configs[plc::kNumSensors] = {};
  double totalLiters = 0.0;
  double aggFlow = 0.0;
  bool allReady = false;
  uint16_t undersampling = 0;

  // Channel 0 is live and flowing. Channel 1 is DISABLED but deliberately loaded with stale state —
  // pulses pending, volume on the clock — exactly what a sensor that was just switched off looks
  // like before anything clears it.
  for (std::size_t i = 0; i < plc::kNumSensors; ++i) {
    configs[i].q_max = 100;
    configs[i].f_multiplier = 1;
  }
  sensors[0].inUse = true;
  sensors[0].isReady = true;
  sensors[0].pulseCount = 60;  // 60 pulses in 1 s, F=1 -> 60 L/min -> 1 L/s

  sensors[1].inUse = false;         // disconnected
  sensors[1].isReady = true;        // but its flags were never cleared
  sensors[1].pulseCount = 6000;     // and a large backlog is sitting there
  sensors[1].sessionLiters = 999.0f;
  sensors[1].cumulativeLiters = 12345.0;
  sensors[1].instantFlow_L_s = 42.0f;

  plc::SensorStateEngine::Dependencies deps;
  deps.sensors = sensors;
  deps.configs = configs;
  deps.sensorCount = plc::kNumSensors;
  deps.registerBank = nullptr;
  deps.modbusManager = nullptr;
  deps.totalSessionLitersCache = &totalLiters;
  deps.aggregateFlowLpsCache = &aggFlow;
  deps.allSensorsReadyCache = &allReady;
  deps.undersamplingFlags = &undersampling;

  plc::SensorStateEngine engine(deps);
  engine.update(1.0f);

  std::printf("      ch0 flow=%.2f L/s session=%.2f L | totals: flow=%.2f L/s volume=%.2f L\n",
              static_cast<double>(sensors[0].instantFlow_L_s),
              static_cast<double>(sensors[0].sessionLiters), aggFlow, totalLiters);

  check(sensors[0].instantFlow_L_s > 0.9f && sensors[0].instantFlow_L_s < 1.1f,
        "the live sensor converted its pulses (60 pulses, F=1 -> about 1 L/s)");

  // The aggregates are the part worth asserting: 999 L and 42 L/s are sitting in the array.
  check(aggFlow < 1.1,
        "aggregate flow EXCLUDES the disconnected sensor's stale 42 L/s");
  check(totalLiters < 1.1,
        "and aggregate volume excludes its stale 999 L — a plausible wrong total is the worst kind");

  check(sensors[1].pulseCount == 6000,
        "the disabled channel's pulses are neither converted nor cleared by the engine, which is "
        "exactly why the polling mask must come from inUse and not a second copy of it");
  check(sensors[1].sessionLiters == 999.0f,
        "and its stale volume is left untouched rather than silently folded in");

  // Readiness must ignore it too, or one disconnected channel would hold the green LED off forever.
  check(allReady, "readiness ignores the disconnected channel (RGB_LED_Behavior.md §3.2)");
}

void readinessTests() {
  std::printf("[readiness — RGB_LED_Behavior.md §3.2: every ACTIVE sensor ready]\n");

  {
    // The case that was broken: a realistic two-sensor installation.
    bool inUse[8] = {true, true, false, false, false, false, false, false};
    bool ready[8] = {true, true, false, false, false, false, false, false};
    check(readinessFor(8, inUse, ready),
          "two enabled sensors, both ready -> green (six disabled channels are irrelevant)");
  }
  {
    bool inUse[8] = {true, true, false, false, false, false, false, false};
    bool ready[8] = {true, false, false, false, false, false, false, false};
    check(!readinessFor(8, inUse, ready),
          "two enabled, one not ready -> not green");
  }
  {
    bool inUse[8] = {false, false, false, false, false, false, false, false};
    bool ready[8] = {false, false, false, false, false, false, false, false};
    check(!readinessFor(8, inUse, ready),
          "nothing enabled -> not green, via the activeSensors guard rather than the loop");
  }
  {
    bool inUse[8] = {true, true, true, true, true, true, true, true};
    bool ready[8] = {true, true, true, true, true, true, true, true};
    check(readinessFor(8, inUse, ready), "all eight enabled and ready -> green");
  }
  {
    bool inUse[8] = {true, true, true, true, true, true, true, true};
    bool ready[8] = {true, true, true, true, true, true, true, false};
    check(!readinessFor(8, inUse, ready), "all eight enabled, the last not ready -> not green");
  }
  {
    // A single enabled channel is a legitimate deployment and must be able to show green.
    bool inUse[8] = {false, false, false, true, false, false, false, false};
    bool ready[8] = {false, false, false, true, false, false, false, false};
    check(readinessFor(8, inUse, ready),
          "one enabled sensor in the middle of the range, ready -> green");
  }
}

}  // namespace

int main() {
  std::printf("SensorStateEngine — readiness aggregation\n\n");
  readinessTests();
  disconnectedSensorTests();
  std::printf("\n%s (%d checks, %d failures)\n", failures == 0 ? "ALL PASSED" : "FAILURES", checks,
              failures);
  return failures == 0 ? 0 : 1;
}
