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
  std::printf("\n%s (%d checks, %d failures)\n", failures == 0 ? "ALL PASSED" : "FAILURES", checks,
              failures);
  return failures == 0 ? 0 : 1;
}
