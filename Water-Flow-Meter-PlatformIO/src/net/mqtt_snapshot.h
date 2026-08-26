#pragma once

#include <cstddef>
#include <cstdint>

#include "modbus/sensor_types.h"
#include "net/mqtt_publisher.h"
#include "units.h"

namespace plc {

/**
 * Assembling the MQTT telemetry snapshot — moved out of `firmware.cpp` so it can be tested at all.
 *
 * `MqttPublisher` owns the cadence, the queue and the JSON. It does NOT own what goes into the
 * snapshot, and until 2026-08-26 that assembly lived in the middle of `logicTaskCode` — a function in
 * a file no host link set includes. The consequence was `DF24`: `MqttTotalTelemetry` has four fields
 * and the assembly filled two, so `<base>/total/state` published `"total":0.000000,"sensors":0` for
 * the entire life of the topic.
 *
 * **The serializer was correctly tested throughout.** `mqtt_publisher_test.cpp` sets all four fields by
 * hand and asserts the JSON carries them, and it passed, because what was broken was the code that
 * decides what the fields contain. That is the shape this repository keeps finding — a value with a
 * home, a publisher and no author — and `verification-blind-spots.md` already prescribed the fix:
 * prefer MOVING code into a link set over asserting around it. `modbus/sensor_config_nvs.h` was the
 * first instance, extracted for the same reason after the NVS serializer silently dropped two of five
 * calibration fields. This is the second.
 *
 * Arduino-free and header-only, so a host binary drives it with no link-set change.
 */

/** Everything the snapshot needs that is not per-channel state. */
struct MqttSnapshotInputs {
  /** The GROSS aggregates the engine maintains — `aggregateFlowLpmCache` and `totalSessionLitersCache`. */
  double aggregateFlowLpm = 0.0;
  double totalSessionLiters = 0.0;

  float pollingRateKhz = 0.0f;
  uint16_t undersamplingFlags = 0;
  uint32_t uptimeSeconds = 0;
  int8_t wifiRssiDbm = 0;
  MqttCommandResult lastCommandResult = MqttCommandResult::Idle;
};

/**
 * Fills `out` from the live channel state and `inputs`.
 *
 * Overwrites every field it owns, so a caller reusing a snapshot cannot leak a stale reading. The one
 * field deliberately NOT set here is `MqttDiagnosticsTelemetry::baselineRateKhz`, which is `DF23` and
 * carries an unanswered decision of its own — folding it into this move would bury that decision
 * inside a refactor.
 */
inline void fillMqttSnapshot(MqttSnapshot& out,
                             const SensorData* sensors,
                             const SensorCharacteristics* configs,
                             std::size_t count,
                             const MqttSnapshotInputs& inputs) {
  out = MqttSnapshot{};
  if (!sensors || !configs) {
    return;
  }
  if (count > kNumSensors) {
    count = kNumSensors;
  }

  uint16_t uncalibratedFlags = 0;
  uint8_t activeSensors = 0;
  double cumulativeLitres = 0.0;

  for (std::size_t i = 0; i < count; ++i) {
    auto& sensor = out.sensors[i];
    sensor.present = sensors[i].inUse;
    if (!sensor.present) {
      // A disconnected channel publishes no topic at all: zeros would be indistinguishable from a
      // fitted channel with no flow, which is the confusion §4.5 exists to prevent.
      continue;
    }
    ++activeSensors;
    // Inside the present gate, deliberately and as it always was: an out-of-service channel is not a
    // commissioning gap, it is a channel nobody is using. Raising its bit would put `SET?` on the
    // panel for a meter that was removed on purpose.
    if (!configIsValid(configs[i])) {
      uncalibratedFlags |= static_cast<uint16_t>(1u << i);
    }
    // §2a: storage IS L/min, so there is no ×60 here.
    sensor.flowLPerMin = sensors[i].instantFlow_L_min;
    sensor.sessionLiters = sensors[i].sessionLiters;
    sensor.totalCubicMeters = units::litresToCubicMeters(sensors[i].cumulativeLiters);
    sensor.maxFlowLPerMin = sensors[i].maxFlowSinceReset;
    sensor.pulses = sensors[i].pulseCount;
    cumulativeLitres += sensors[i].cumulativeLiters;
  }

  out.total.flowLPerMin = static_cast<float>(inputs.aggregateFlowLpm);
  out.total.sessionLiters = static_cast<float>(inputs.totalSessionLiters);

  /**
   * THE TWO FIELDS `DF24` WAS ABOUT, and the first is a semantic choice rather than a transcription.
   *
   * `totalCubicMeters` has no precedent to copy — nothing has ever computed an aggregate lifetime
   * volume, which is why it was never filled. It is the sum over IN-SERVICE channels, matching
   * `sessionLiters` beside it and the engine's own `if (sensor.inUse)` gate. That is a real decision
   * and it can be argued the other way: an out-of-service meter's litres are still litres that flowed,
   * and "lifetime total" reads as all-time-all-channels. The gate is chosen for consistency with every
   * other aggregate on this payload, so a subscriber comparing `total` against the sum of the per-
   * sensor topics it receives gets an answer that agrees. Changing it later is a wire-visible change.
   *
   * Summed as LITRES and converted once, not as eight already-divided values: `litresToCubicMeters` is
   * a division, and dividing then adding differs from adding then dividing in the last places. Adding
   * first also matches how the engine accumulates.
   */
  out.total.totalCubicMeters = units::litresToCubicMeters(cumulativeLitres);

  /**
   * `activeSensors` is the count of IN-SERVICE channels — the same predicate as `present`, so it can
   * never disagree with the number of per-sensor topics a subscriber actually receives. That invariant
   * is the whole value of the field and it is asserted in the host test.
   *
   * When the cascade lands (`N-e`), "how many meters contribute to the total" stops being the same
   * question as "how many are in service", because a downstream channel contributes nothing directly.
   * That will want its OWN field rather than a redefinition of this one — the register and topic layout
   * should leave room, and `N-e` §7 Q1a is where it is being decided.
   */
  out.total.activeSensors = activeSensors;

  out.diagnostics.pollingRateKhz = inputs.pollingRateKhz;
  out.diagnostics.undersamplingFlags = inputs.undersamplingFlags;
  out.diagnostics.uncalibratedFlags = uncalibratedFlags;
  out.diagnostics.uptimeSeconds = inputs.uptimeSeconds;
  out.diagnostics.wifiRssiDbm = inputs.wifiRssiDbm;
  // R4.4.2d's remote half. Sticky, so it describes the last command rather than the last tick.
  out.diagnostics.lastCommandResult = inputs.lastCommandResult;
}

}  // namespace plc
