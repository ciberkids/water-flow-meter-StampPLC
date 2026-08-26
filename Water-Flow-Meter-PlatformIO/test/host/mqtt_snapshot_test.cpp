/**
 * The MQTT snapshot ASSEMBLY — the half that had no test, which is why `DF24` shipped.
 *
 * `mqtt_publisher_test.cpp` covers the serializer: it sets all four `MqttTotalTelemetry` fields by hand
 * and asserts the JSON carries them, and it passed for the whole life of the aggregate topic while that
 * topic published `"total":0.000000,"sensors":0`. What was broken was the code deciding what the fields
 * contain, and that code was in `firmware.cpp`, which no host link set includes. Moving it into
 * `net/mqtt_snapshot.h` is what makes this file possible; the assertions below are what make the move
 * worth having.
 *
 * So the emphasis here is deliberately on the fields NOBODY was filling, and on the invariant a
 * subscriber will assume without being told: that `activeSensors` equals the number of per-sensor
 * topics it actually receives.
 */
#include <cmath>
#include <cstdio>
#include <cstring>

#include "net/mqtt_snapshot.h"

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const char* what) {
  ++checks;
  std::printf("  %-72s %s\n", what, condition ? "ok" : "FAIL");
  if (!condition) ++failures;
}

/** A channel in service with a valid Formula calibration, so `configIsValid` accepts it. */
SensorCharacteristics calibrated() {
  SensorCharacteristics cfg{};
  cfg.q_max = 100;
  cfg.f_multiplier = 8;
  cfg.adjust = 0;
  return cfg;
}

void theTwoFieldsNobodyWasFilling() {
  std::printf("\n[DF24 — totalCubicMeters and activeSensors, which were assigned nowhere]\n");

  SensorData sensors[plc::kNumSensors]{};
  SensorCharacteristics configs[plc::kNumSensors]{};

  // Three channels in service with litres on them, chosen so the sum is exact in binary and the
  // assertion cannot pass by rounding: 1000 + 250 + 750 = 2000 L = 2 m3.
  const std::size_t inService[] = {0, 3, 7};
  const double litres[] = {1000.0, 250.0, 750.0};
  for (std::size_t k = 0; k < 3; ++k) {
    const std::size_t i = inService[k];
    sensors[i].inUse = true;
    sensors[i].cumulativeLiters = litres[k];
    sensors[i].sessionLiters = 10.0f;
    sensors[i].instantFlow_L_min = 1.5f;
    configs[i] = calibrated();
  }

  plc::MqttSnapshotInputs inputs;
  inputs.aggregateFlowLpm = 4.5;
  inputs.totalSessionLiters = 30.0;

  plc::MqttSnapshot snapshot;
  plc::fillMqttSnapshot(snapshot, sensors, configs, plc::kNumSensors, inputs);

  check(std::fabs(snapshot.total.totalCubicMeters - 2.0) < 1e-9,
        "the aggregate lifetime volume is the sum of the in-service channels, in m3");
  check(snapshot.total.activeSensors == 3, "and activeSensors counts them");

  // The invariant a subscriber assumes without being told: the count matches the number of per-sensor
  // topics it receives, because both come from the same `present` predicate.
  std::uint8_t present = 0;
  for (std::size_t i = 0; i < plc::kNumSensors; ++i) {
    if (snapshot.sensors[i].present) ++present;
  }
  check(snapshot.total.activeSensors == present,
        "activeSensors equals the number of `present` sensors — the same predicate, always");

  // The two that already worked, so the move did not lose them.
  check(std::fabs(snapshot.total.flowLPerMin - 4.5f) < 1e-6, "the aggregate flow still comes through");
  check(std::fabs(snapshot.total.sessionLiters - 30.0f) < 1e-6, "and the aggregate session volume");
}

void litresAreSummedBeforeTheyAreDivided() {
  std::printf("\n[precision — add litres, then convert once]\n");

  SensorData sensors[plc::kNumSensors]{};
  SensorCharacteristics configs[plc::kNumSensors]{};
  // Values whose per-channel m3 conversions are each inexact in binary, so dividing-then-adding and
  // adding-then-dividing land on different doubles. 1/3 of a litre, eight times.
  for (std::size_t i = 0; i < plc::kNumSensors; ++i) {
    sensors[i].inUse = true;
    sensors[i].cumulativeLiters = 1.0 / 3.0;
    configs[i] = calibrated();
  }
  plc::MqttSnapshot snapshot;
  plc::fillMqttSnapshot(snapshot, sensors, configs, plc::kNumSensors, plc::MqttSnapshotInputs{});

  double addedThenDivided = 0.0;
  for (std::size_t i = 0; i < plc::kNumSensors; ++i) addedThenDivided += sensors[i].cumulativeLiters;
  addedThenDivided = units::litresToCubicMeters(addedThenDivided);

  double dividedThenAdded = 0.0;
  for (std::size_t i = 0; i < plc::kNumSensors; ++i) {
    dividedThenAdded += units::litresToCubicMeters(sensors[i].cumulativeLiters);
  }

  check(snapshot.total.totalCubicMeters == addedThenDivided,
        "the total is litres summed and converted ONCE, which is how the engine accumulates");
  // Not an assertion about which is 'more correct' in the abstract — only that the two really differ,
  // so the assertion above is discriminating rather than vacuous.
  std::printf("      added-then-divided %.20f\n      divided-then-added %.20f\n", addedThenDivided,
              dividedThenAdded);
  check(addedThenDivided != dividedThenAdded,
        "and the two orders really do give different doubles here, so that check discriminates");
}

void outOfServiceChannelsAreExcludedEverywhere() {
  std::printf("\n[an out-of-service channel publishes nothing, and counts for nothing]\n");

  SensorData sensors[plc::kNumSensors]{};
  SensorCharacteristics configs[plc::kNumSensors]{};
  // Channel 0 in service; channel 1 has litres and a BROKEN calibration but is out of service.
  sensors[0].inUse = true;
  sensors[0].cumulativeLiters = 500.0;
  configs[0] = calibrated();
  sensors[1].inUse = false;
  sensors[1].cumulativeLiters = 9999.0;  // real litres, on a channel nobody is using

  plc::MqttSnapshot snapshot;
  plc::fillMqttSnapshot(snapshot, sensors, configs, plc::kNumSensors, plc::MqttSnapshotInputs{});

  check(snapshot.total.activeSensors == 1, "only the in-service channel is counted");
  check(std::fabs(snapshot.total.totalCubicMeters - 0.5) < 1e-9,
        "and only its litres reach the aggregate — the documented gate, not an accident");
  check(!snapshot.sensors[1].present, "the out-of-service channel is not present");
  check(snapshot.sensors[1].totalCubicMeters == 0.0,
        "and carries no reading at all, so zeros cannot be mistaken for a fitted channel at rest");
  // An out-of-service channel is not a commissioning gap: it is a channel nobody is using, and
  // flagging it would put SET? on the panel for a meter that was removed on purpose.
  check(snapshot.diagnostics.uncalibratedFlags == 0,
        "an out-of-service channel with no calibration raises no uncalibrated bit");
}

void uncalibratedInServiceChannelsAreFlagged() {
  std::printf("\n[the commissioning gap — in service, no valid calibration]\n");

  SensorData sensors[plc::kNumSensors]{};
  SensorCharacteristics configs[plc::kNumSensors]{};
  sensors[2].inUse = true;
  configs[2] = calibrated();
  sensors[5].inUse = true;  // configs[5] left zeroed, so configIsValid refuses it

  plc::MqttSnapshot snapshot;
  plc::fillMqttSnapshot(snapshot, sensors, configs, plc::kNumSensors, plc::MqttSnapshotInputs{});

  check(snapshot.diagnostics.uncalibratedFlags == (1u << 5),
        "bit 5 and only bit 5 — the bitmap names the channel, which a count could not");
  check(snapshot.total.activeSensors == 2,
        "an uncalibrated channel is still in service, so it still counts");
}

void everyDiagnosticInputIsCarried() {
  std::printf("\n[the diagnostics pass through, and baselineKhz deliberately does not]\n");

  SensorData sensors[plc::kNumSensors]{};
  SensorCharacteristics configs[plc::kNumSensors]{};
  plc::MqttSnapshotInputs inputs;
  inputs.pollingRateKhz = 3.3f;
  inputs.undersamplingFlags = 0x82;
  inputs.uptimeSeconds = 86400;
  inputs.wifiRssiDbm = -57;
  inputs.lastCommandResult = plc::MqttCommandResult::RateLimited;

  plc::MqttSnapshot snapshot;
  plc::fillMqttSnapshot(snapshot, sensors, configs, plc::kNumSensors, inputs);

  check(std::fabs(snapshot.diagnostics.pollingRateKhz - 3.3f) < 1e-6, "the polling rate is carried");
  check(snapshot.diagnostics.undersamplingFlags == 0x82, "the undersampling bitmap is carried");
  check(snapshot.diagnostics.uptimeSeconds == 86400, "uptime is carried");
  check(snapshot.diagnostics.wifiRssiDbm == -57, "RSSI is carried");
  check(snapshot.diagnostics.lastCommandResult == plc::MqttCommandResult::RateLimited,
        "and R4.4.2d's sticky command result is carried");

  // DF23, and it stays open ON PURPOSE. R2.1.2 wants a MEASURED radio-off baseline recorded once per
  // firmware update, and that carries an unanswered decision about a first boot with WiFi already
  // enabled. Filling it here would bury that decision inside a refactor, so this asserts the gap
  // rather than papering over it.
  check(snapshot.diagnostics.baselineRateKhz == 0.0f,
        "baselineKhz is still 0 — DF23, deliberately not fixed by this move");
}

void badInputCannotProduceGarbage() {
  std::printf("\n[a null or oversized input returns an empty snapshot, not a wild read]\n");

  plc::MqttSnapshot snapshot;
  snapshot.total.activeSensors = 42;  // pre-dirtied, so a no-op would be visible
  plc::fillMqttSnapshot(snapshot, nullptr, nullptr, plc::kNumSensors, plc::MqttSnapshotInputs{});
  check(snapshot.total.activeSensors == 0, "a null channel array yields a zeroed snapshot");

  SensorData sensors[plc::kNumSensors]{};
  SensorCharacteristics configs[plc::kNumSensors]{};
  for (auto& s : sensors) s.inUse = true;
  for (auto& c : configs) c = calibrated();
  // A count beyond the array is clamped rather than trusted: the struct has kNumSensors slots and a
  // caller passing more would otherwise walk off both arrays.
  plc::fillMqttSnapshot(snapshot, sensors, configs, plc::kNumSensors + 5, plc::MqttSnapshotInputs{});
  check(snapshot.total.activeSensors == plc::kNumSensors,
        "an oversized count is clamped to kNumSensors");

  // And a reused snapshot is fully overwritten, so a stale reading cannot survive a pass.
  plc::MqttSnapshot reused;
  reused.sensors[4].sessionLiters = 1234.0f;
  reused.diagnostics.uptimeSeconds = 999;
  SensorData empty[plc::kNumSensors]{};
  SensorCharacteristics emptyCfg[plc::kNumSensors]{};
  plc::fillMqttSnapshot(reused, empty, emptyCfg, plc::kNumSensors, plc::MqttSnapshotInputs{});
  check(reused.sensors[4].sessionLiters == 0.0f && reused.diagnostics.uptimeSeconds == 0,
        "a reused snapshot is overwritten, so last pass's reading cannot leak into this one");
}

}  // namespace

int main() {
  std::printf("mqtt_snapshot — the assembly firmware.cpp used to hide (DF24)\n");
  theTwoFieldsNobodyWasFilling();
  litresAreSummedBeforeTheyAreDivided();
  outOfServiceChannelsAreExcludedEverywhere();
  uncalibratedInServiceChannelsAreFlagged();
  everyDiagnosticInputIsCarried();
  badInputCannotProduceGarbage();
  if (failures > 0) {
    std::printf("\nFAILURES (%d of %d)\n", failures, checks);
    return 1;
  }
  std::printf("\nALL PASSED (%d checks)\n", checks);
  return 0;
}
