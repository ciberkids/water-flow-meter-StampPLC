#include "sensors/sensor_state_engine.h"

namespace plc {

SensorStateEngine::SensorStateEngine(const Dependencies& deps) : deps_(deps) {}

void SensorStateEngine::update(float elapsedSeconds) {
  if (!deps_.sensors || !deps_.configs || elapsedSeconds <= 0.0f) {
    return;
  }

  double totalSessionLiters = 0.0;
  double aggregateFlowLps = 0.0;
  bool allReady = true;
  std::size_t activeSensors = 0;

  for (std::size_t i = 0; i < deps_.sensorCount; ++i) {
    auto& sensor = deps_.sensors[i];
    const auto& config = deps_.configs[i];

    if (sensor.inUse) {
      ++activeSensors;
      const uint32_t pulses = sensor.pulseCount;
      sensor.pulseCount = 0;

      if (sensor.isReady && config.f_multiplier != 0.0f) {
        const float frequency = static_cast<float>(pulses) / elapsedSeconds;
        float flowRateLpm = (frequency - config.adjust) / config.f_multiplier;
        if (flowRateLpm < 0.0f) {
          flowRateLpm = 0.0f;
        }
        if (flowRateLpm > config.q_max) {
          flowRateLpm = config.q_max;
        }

        sensor.instantFlow_L_s = flowRateLpm / 60.0f;
        if (sensor.instantFlow_L_s > sensor.maxFlowSinceReset) {
          sensor.maxFlowSinceReset = sensor.instantFlow_L_s;
        }

        const double litersInterval = sensor.instantFlow_L_s * elapsedSeconds;
        sensor.sessionLiters += litersInterval;
        sensor.cumulativeLiters += litersInterval;
      } else {
        sensor.instantFlow_L_s = 0.0f;
      }

      totalSessionLiters += sensor.sessionLiters;
      aggregateFlowLps += sensor.instantFlow_L_s;
      if (!sensor.isReady) {
        allReady = false;
      }
    } else {
      sensor.instantFlow_L_s = 0.0f;
      // A DISABLED channel must not clear allReady. RGB_LED_Behavior.md §3.2 defines green
      // as "solid ON when every ACTIVE sensor has isReady == true" — active, not all eight.
      // Clearing it here meant a two-sensor installation could never show green, because the
      // six unused channels each falsified it. The "nothing enabled at all" case is already
      // handled by the activeSensors == 0 guard below, which is where it belongs.
    }

    if (deps_.modbusManager) {
      deps_.modbusManager->syncSensorToHolding(i);
    }
  }

  if (activeSensors == 0) {
    allReady = false;
  }

  if (deps_.totalSessionLitersCache) {
    *deps_.totalSessionLitersCache = totalSessionLiters;
  }
  if (deps_.aggregateFlowLpsCache) {
    *deps_.aggregateFlowLpsCache = aggregateFlowLps;
  }
  if (deps_.allSensorsReadyCache) {
    *deps_.allSensorsReadyCache = allReady;
  }

  refreshDiagnostics();
  if (deps_.modbusManager) {
    deps_.modbusManager->syncGlobalRegisters();
  }
}

void SensorStateEngine::refreshDiagnostics() {
  if (!deps_.modbusManager || !deps_.registerBank || !deps_.undersamplingFlags) {
    return;
  }
  deps_.modbusManager->evaluateSensorDiagnostics();
  *deps_.undersamplingFlags = deps_.registerBank->at(REG_UNDERSAMPLING_FLAGS);
}

}  // namespace plc
