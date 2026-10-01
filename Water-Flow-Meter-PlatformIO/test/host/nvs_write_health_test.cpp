// DF25 — whether this device can tell that it has stopped keeping what it was told to keep.
//
// The defect: every write to non-volatile storage discarded its result, so a device whose store had
// failed kept publishing correct totals from RAM and lost them at the next power cycle with nothing
// reporting it. The decision, 2026-08-30: N consecutive failures on the same key, N = 3.
//
// Three of these checks exist because of a specific trap rather than for symmetry:
//
//   * `putString` returns `strlen(value)`, so a SUCCESSFUL write of an empty string returns 0 and is
//     indistinguishable from failure by the obvious test. `net_settings_nvs.h` writes nine text fields
//     on every save and an unset MQTT password is an empty string on a healthy device — so testing
//     `returned != 0` would raise a warning triangle on most devices in the field.
//   * a success on ANOTHER key must not clear this key's run, or eight channels writing once a minute
//     would keep each other's alarms permanently reset;
//   * the published code must not change while the fault persists, or an operator cannot write it down
//     and look it up.
//
// The wire-contract numbers are pinned here on purpose. They are append-only from the moment this ships,
// like every other code space in this firmware, and a test is the only thing that makes a silent
// renumbering fail.
#include "storage/nvs_write_health.h"

#include "modbus/sensor_config_nvs.h"
#include "sensors/sensor_topology_nvs.h"

#include <cstdio>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const char* what) {
  ++checks;
  std::printf("  %-78s %s\n", what, condition ? "ok" : "FAIL");
  if (!condition) ++failures;
}

using plc::NvsWriteHealth;
using plc::StorageFault;

/**
 * A store that can be told to fail a NAMED key a given number of times.
 *
 * Per-key rather than a global "fail the next N writes", because the decided policy is per-key: the
 * suite has to be able to say "cml_3 fails three times while cfg_q0 succeeds throughout", which a global
 * countdown cannot express. `mqtt_publisher_test.cpp`'s `failNext` is the global shape, and it is the
 * right one for what it tests.
 *
 * Returns the bytes written on success and 0 on failure — the real `Preferences` contract.
 */
class FailingStore {
 public:
  void failKey(const char* key, int times) { remaining_[key] = times; }

  std::size_t putUChar(const char* key, std::uint8_t value) { return take(key, sizeof(value)); }
  std::size_t putUShort(const char* key, std::uint16_t value) { return take(key, sizeof(value)); }
  std::size_t putShort(const char* key, std::int16_t value) { return take(key, sizeof(value)); }

  std::uint8_t getUChar(const char*, std::uint8_t d) const { return d; }
  std::uint16_t getUShort(const char*, std::uint16_t d) const { return d; }
  std::int16_t getShort(const char*, std::int16_t d) const { return d; }

 private:
  std::size_t take(const char* key, std::size_t width) {
    const auto it = remaining_.find(key);
    if (it != remaining_.end() && it->second > 0) {
      --it->second;
      return 0;  // the real store's failure return
    }
    return width;
  }
  std::map<std::string, int> remaining_;
};

// ---------------------------------------------------------------------------------------------------

void resultPredicateTests() {
  std::printf("\nDid the write land? — the two predicates\n");

  check(plc::nvsPutOk(2, sizeof(std::uint16_t)), "a two-byte put returning 2 landed");
  check(!plc::nvsPutOk(0, sizeof(std::uint16_t)), "returning 0 is the store's failure signal");
  check(!plc::nvsPutOk(1, sizeof(std::uint16_t)),
        "and a SHORT write is a failure too — comparing against 0 alone would miss it");
  check(plc::nvsPutOk(8, sizeof(double)), "a putDouble returning 8 landed");

  check(plc::nvsPutStringOk(0, ""),
        "an EMPTY string writing 0 bytes SUCCEEDED — putString returns strlen, and this is the trap");
  check(!plc::nvsPutStringOk(0, "secret"),
        "while a six-character string returning 0 did not");
  check(plc::nvsPutStringOk(6, "secret"), "and returning its length did");
  check(!plc::nvsPutStringOk(3, "secret"), "a truncated string write is a failure");
  check(plc::nvsPutStringOk(0, nullptr), "a null value is treated as the empty string, not as a crash");
}

void thresholdTests() {
  std::printf("\nN consecutive failures on the same key — N = 3\n");
  check(NvsWriteHealth::kConsecutiveFailuresBeforeAlarm == 3,
        "the decided threshold is 3, so the alarm means three minutes of not keeping totals");

  NvsWriteHealth health;
  check(!health.alarmRaised(), "a fresh device is not in alarm");
  check(health.faultCode() == StorageFault::None, "and publishes code 0");

  health.noteResult(StorageFault::CumulativeLitres, false);
  check(!health.alarmRaised(), "one failed write is not news — the next pass may well succeed");
  health.noteResult(StorageFault::CumulativeLitres, false);
  check(!health.alarmRaised(), "two in a row is still not news");
  check(health.consecutiveFailures(StorageFault::CumulativeLitres) == 2, "but the run is counted");
  health.noteResult(StorageFault::CumulativeLitres, false);
  check(health.alarmRaised(), "the THIRD raises the alarm");
  check(health.faultCode() == StorageFault::CumulativeLitres, "and names the litres as the fault");

  health.noteResult(StorageFault::CumulativeLitres, true);
  check(!health.alarmRaised(), "one success clears that key's run and lowers the alarm");
  check(health.consecutiveFailures(StorageFault::CumulativeLitres) == 0, "the count is back to zero");

  // Saturation: 256 failures must not read as zero. On a PERIODIC group — a one-shot group pins at the
  // threshold through raiseNow on its first failure and never counts past it, which the cadence group
  // below asserts.
  for (int i = 0; i < 300; ++i) {
    health.noteResult(StorageFault::CumulativeLitres, false);
  }
  check(health.consecutiveFailures(StorageFault::CumulativeLitres) == 255,
        "the counter saturates rather than wrapping — 256 failures must never read as healthy");
  check(health.inAlarm(StorageFault::CumulativeLitres), "and it is still in alarm at 300");
}

void perKeyIndependenceTests() {
  std::printf("\nOne key's success must not clear another key's run\n");

  NvsWriteHealth health;
  health.noteResult(StorageFault::CumulativeLitres, false);
  health.noteResult(StorageFault::CumulativeLitres, false);
  // The calibration writer succeeds in the same pass — as it would if only the litre keys were affected.
  health.noteResult(StorageFault::SensorCalibration, true);
  check(health.consecutiveFailures(StorageFault::CumulativeLitres) == 2,
        "a success on the calibration keys leaves the litre run intact");
  health.noteResult(StorageFault::CumulativeLitres, false);
  check(health.faultCode() == StorageFault::CumulativeLitres,
        "so the third litre failure still raises — eight writers cannot reset each other forever");

  health.noteResult(StorageFault::None, true);
  check(health.faultCode() == StorageFault::CumulativeLitres,
        "and `None` is not a key: reporting it clears nothing");
  health.noteResult(StorageFault::None, false);
  check(health.consecutiveFailures(StorageFault::None) == 0, "nor does it accumulate");
}

void faultCodeStabilityTests() {
  std::printf("\nThe published code is stable while the fault persists\n");

  NvsWriteHealth health;
  for (int i = 0; i < 3; ++i) {
    health.noteResult(StorageFault::NetworkSettings, false);
  }
  check(health.faultCode() == StorageFault::NetworkSettings, "the network settings raise first");
  for (int i = 0; i < 3; ++i) {
    health.noteResult(StorageFault::CumulativeLitres, false);
  }
  check(health.faultCode() == StorageFault::CumulativeLitres,
        "when the litres fail too the LOWER code wins, so the code an operator wrote down still means "
        "the same thing");
  check(health.alarmCount() == 2, "while the count says two groups are failing at once");

  NvsWriteHealth opened;
  for (int i = 0; i < 3; ++i) {
    opened.noteResult(StorageFault::CumulativeLitres, false);
    opened.noteResult(StorageFault::StoreDidNotOpen, false);
  }
  check(opened.faultCode() == StorageFault::StoreDidNotOpen,
        "a store that never opened outranks everything it caused — the cause, not the symptom");
}

void wireContractTests() {
  std::printf("\nThe code numbers are a wire contract — append only, never renumber\n");
  check(static_cast<std::uint16_t>(StorageFault::None) == 0, "0 is healthy, as in every other code space here");
  check(static_cast<std::uint16_t>(StorageFault::StoreDidNotOpen) == 1, "1 = the store did not open");
  check(static_cast<std::uint16_t>(StorageFault::CumulativeLitres) == 2, "2 = lifetime litres");
  check(static_cast<std::uint16_t>(StorageFault::SensorCalibration) == 3, "3 = calibration");
  check(static_cast<std::uint16_t>(StorageFault::Topology) == 4, "4 = cascade topology");
  check(static_cast<std::uint16_t>(StorageFault::ConnectedBitmap) == 5, "5 = in-service bitmap");
  check(static_cast<std::uint16_t>(StorageFault::LinkSettings) == 6, "6 = RS485 link settings");
  check(static_cast<std::uint16_t>(StorageFault::FlowUnit) == 7, "7 = display flow unit");
  check(static_cast<std::uint16_t>(StorageFault::LedSettings) == 8, "8 = LED settings");
  check(static_cast<std::uint16_t>(StorageFault::NetworkSettings) == 9, "9 = network settings");
  check(static_cast<std::uint16_t>(StorageFault::CommandEpoch) == 10, "10 = command rate-limit epochs");
  check(static_cast<std::uint16_t>(StorageFault::PackAttemptCounter) == 11, "11 = menu-pack attempts");
  check(static_cast<std::uint16_t>(StorageFault::FactoryResetErase) == 12, "12 = factory-reset erase");
  check(static_cast<std::uint16_t>(StorageFault::TopologyNotAForest) == 13,
        "13 = a stored topology that read back intact and is not a forest — a READ fault, appended "
        "rather than folded into 4, whose contract is write health");
  check(plc::kStorageFaultCount == 14 && plc::kStorageFaultBandEnd == 32,
        "fourteen codes in a band of thirty-two, so the next subsystem cannot renumber these");
}


void writerCadenceTests() {
  std::printf("\nN=3 counts only where something will retry — the cadence table\n");

  check(plc::storageFaultIsPeriodic(StorageFault::CumulativeLitres) &&
            plc::storageFaultIsPeriodic(StorageFault::SensorCalibration) &&
            plc::storageFaultIsPeriodic(StorageFault::ConnectedBitmap) &&
            plc::storageFaultIsPeriodic(StorageFault::NetworkSettings),
        "the four groups the once-a-minute pass re-attempts are periodic");
  check(!plc::storageFaultIsPeriodic(StorageFault::TopologyNotAForest),
        "a boot-time read that will not be re-read is one-shot, so it raises on the first failure");
  check(!plc::storageFaultIsPeriodic(StorageFault::LinkSettings) &&
            !plc::storageFaultIsPeriodic(StorageFault::FlowUnit) &&
            !plc::storageFaultIsPeriodic(StorageFault::LedSettings) &&
            !plc::storageFaultIsPeriodic(StorageFault::CommandEpoch) &&
            !plc::storageFaultIsPeriodic(StorageFault::Topology) &&
            !plc::storageFaultIsPeriodic(StorageFault::PackAttemptCounter),
        "and the one-shot writers are not — nothing comes back to retry an operator's apply");

  // The hole review found: a one-shot group counted to 1 and stopped, so the failure was recorded and
  // could never be published.
  NvsWriteHealth oneShot;
  oneShot.noteResult(StorageFault::LinkSettings, false);
  check(oneShot.faultCode() == StorageFault::LinkSettings,
        "a one-shot write raises on the FIRST failure — §4.1.1's unattended rollback gets no second try");
  check(oneShot.consecutiveFailures(StorageFault::LinkSettings) ==
            NvsWriteHealth::kConsecutiveFailuresBeforeAlarm,
        "and it is recorded at the threshold rather than as a count of one");

  for (int i = 0; i < 50; ++i) {
    oneShot.noteResult(StorageFault::LinkSettings, false);
  }
  check(oneShot.consecutiveFailures(StorageFault::LinkSettings) ==
            NvsWriteHealth::kConsecutiveFailuresBeforeAlarm,
        "repeated one-shot failures pin at the threshold rather than climbing to 255");

  oneShot.noteResult(StorageFault::LinkSettings, true);
  check(oneShot.faultCode() == StorageFault::None,
        "a later successful write of the same group clears it, so a transient failure is recoverable");

  NvsWriteHealth periodic;
  periodic.noteResult(StorageFault::CumulativeLitres, false);
  check(periodic.faultCode() == StorageFault::None,
        "while a PERIODIC group still takes three — one refused litre write is not news");

  // The two conditions nothing retries at all keep raising immediately, through raiseNow.
  NvsWriteHealth erase;
  erase.raiseNow(StorageFault::FactoryResetErase);
  check(erase.faultCode() == StorageFault::FactoryResetErase,
        "and a factory reset that could not erase raises at once, as it always did");
}

void serializerIntegrationTests() {
  std::printf("\nThe serializers report what failed, and the counter turns it into an alarm\n");

  FailingStore store;
  store.failKey("cfg_q0", 10);  // one of the five keys refuses, the other four land
  SensorCharacteristics cfg;
  cfg.q_max = 100;
  cfg.f_multiplier = 6;
  check(plc::saveSensorConfigTo(store, 0, cfg) == 1,
        "a calibration save with one dead key reports exactly one failed write out of five");

  NvsWriteHealth health;
  for (int pass = 0; pass < 3; ++pass) {
    health.noteResult(StorageFault::SensorCalibration, plc::saveSensorConfigTo(store, 0, cfg) == 0);
  }
  check(health.faultCode() == StorageFault::SensorCalibration,
        "three passes of that same partial failure raise the calibration code");

  FailingStore healthy;
  check(plc::saveSensorConfigTo(healthy, 0, cfg) == 0, "and a healthy store reports nothing failed");

  // The topology case the module's own comment calls out: a partial write that REMAINS a forest is a
  // different topology, silently — which is why the return value is [[nodiscard]] rather than advisory.
  FailingStore partial;
  partial.failKey("parent_2", 10);
  plc::SensorTopology topology;
  std::uint8_t parents[plc::SensorTopology::kChannels] = {};
  parents[1] = 1;
  parents[2] = 2;
  check(topology.apply(parents).ok(), "a two-edge chain applies in RAM");
  check(plc::saveSensorTopologyTo(partial, topology) == 1,
        "and one dead key out of eight is reported rather than absorbed");
  check(plc::loadSensorTopologyFrom(partial).topology.parent(2) == plc::kTopologyRoot,
        "the reloaded topology is a DIFFERENT, still-legal forest — exactly why the caller must look");
}

}  // namespace

int main() {
  std::printf("Non-volatile write health (DF25)\n");
  resultPredicateTests();
  thresholdTests();
  perKeyIndependenceTests();
  faultCodeStabilityTests();
  wireContractTests();
  writerCadenceTests();
  serializerIntegrationTests();
  std::printf("\n%s (%d checks, %d failures)\n", failures == 0 ? "ALL PASSED" : "FAILURES", checks,
              failures);
  return failures == 0 ? 0 : 1;
}
