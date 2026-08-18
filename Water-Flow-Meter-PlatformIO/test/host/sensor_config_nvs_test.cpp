// A channel's calibration across a power cycle — the two fields that never reached flash.
//
// `saveSensorConfig` wrote three of `SensorCharacteristics`' five fields. A master writing register
// 123 (the calibration form) and 124 (pulses per litre) had both accepted, read them back correctly
// for as long as the device stayed up, and lost them on the next reboot: the channel returned as
// `Formula` with `pulses_per_litre = 0`, which fails `configIsValid`, so it reported `SET?` and
// published 0.0 for flow, session and lifetime total. RS485 is the source of truth for everything, so
// a master's write surviving a reboot is not optional.
//
// It shipped because firmware.cpp is in no link set in run.sh — the same blind spot
// `pulse_counter.h` was carved out to close. The serializer now lives in a header, so these run.
#include "modbus/sensor_config_nvs.h"

#include "modbus/register_map.h"  // kNumSensors — the key-collision check sweeps every channel

#include <cstdio>
#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <string>

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const char* what) {
  ++checks;
  std::printf("  %-78s %s\n", what, condition ? "ok" : "FAIL");
  if (!condition) ++failures;
}

/**
 * A stand-in for `Preferences`, faithful in the one behaviour that matters here: a key that was never
 * written returns the caller's default. That is what makes the upgrade path safe and what made the
 * defect invisible — the missing keys returned 0, which is a legal-looking configuration.
 *
 * Keys are recorded so a truncating or colliding key name is visible as a count rather than as two
 * channels mysteriously sharing a calibration.
 */
class FakeStore {
 public:
  void putUShort(const char* key, uint16_t value) {
    unsigned_[key] = value;
    written_.insert(key);
  }
  void putShort(const char* key, int16_t value) {
    signed_[key] = value;
    written_.insert(key);
  }
  uint16_t getUShort(const char* key, uint16_t defaultValue) const {
    const auto it = unsigned_.find(key);
    return it == unsigned_.end() ? defaultValue : it->second;
  }
  int16_t getShort(const char* key, int16_t defaultValue) const {
    const auto it = signed_.find(key);
    return it == signed_.end() ? defaultValue : it->second;
  }

  std::size_t keyCount() const { return written_.size(); }
  bool has(const std::string& key) const { return written_.count(key) != 0; }
  /** Simulates the three-key firmware: only q/f/a were ever in flash. */
  void seedLegacy(std::size_t index, uint16_t qMax, int16_t multiplier, int16_t adjust) {
    const std::string suffix = std::to_string(index);
    unsigned_["cfg_q" + suffix] = qMax;
    signed_["cfg_f" + suffix] = multiplier;
    signed_["cfg_a" + suffix] = adjust;
    written_.insert("cfg_q" + suffix);
    written_.insert("cfg_f" + suffix);
    written_.insert("cfg_a" + suffix);
  }
  void forceCalibrationWord(std::size_t index, uint16_t raw) {
    const std::string key = "cfg_c" + std::to_string(index);
    unsigned_[key] = raw;
    written_.insert(key);
  }

 private:
  std::map<std::string, uint16_t> unsigned_;
  std::map<std::string, int16_t> signed_;
  std::set<std::string> written_;
};

void roundTripTests() {
  std::printf("\n[all five fields survive a power cycle — the defect]\n");

  // The exact configuration the defect destroyed: a 450 pulses/L meter, the commonest form there is.
  SensorCharacteristics pulses;
  pulses.q_max = 100;
  pulses.calibration = CalibrationType::PulsesPerLitre;
  pulses.pulses_per_litre = 450;

  FakeStore store;
  plc::saveSensorConfigTo(store, 0, pulses);
  const SensorCharacteristics reloaded = plc::loadSensorConfigFrom(store, 0);

  check(reloaded.calibration == CalibrationType::PulsesPerLitre,
        "the calibration FORM survives — register 123 was previously lost");
  check(reloaded.pulses_per_litre == 450,
        "the K figure survives — register 124 was previously lost");
  check(reloaded.q_max == 100, "and q_max, which always did");
  check(reloaded == pulses, "the whole struct compares equal, so no field was dropped");
  check(configIsValid(reloaded),
        "so the channel comes back READY rather than reporting SET? with a zeroed total");

  // The formula form, with a negative adjust — the sign has to survive the uint16/int16 split.
  SensorCharacteristics formula;
  formula.q_max = 150;
  formula.f_multiplier = 6;
  formula.adjust = -120;
  formula.calibration = CalibrationType::Formula;
  plc::saveSensorConfigTo(store, 1, formula);
  const SensorCharacteristics reloadedFormula = plc::loadSensorConfigFrom(store, 1);
  check(reloadedFormula == formula, "a formula channel round-trips too, negative adjust included");
  check(reloadedFormula.adjust == -120, "and the sign of adjust is not lost through NVS");

  // Channel 0 must not have been disturbed by channel 1's write.
  check(plc::loadSensorConfigFrom(store, 0) == pulses,
        "writing channel 1 left channel 0's calibration alone");
}

void keyTests() {
  std::printf("\n[keys: five per channel, all distinct]\n");

  FakeStore store;
  SensorCharacteristics cfg;
  cfg.q_max = 10;
  cfg.f_multiplier = 2;
  cfg.pulses_per_litre = 7;
  for (std::size_t i = 0; i < plc::kNumSensors; ++i) {
    plc::saveSensorConfigTo(store, i, cfg);
  }
  check(store.keyCount() == 5 * plc::kNumSensors,
        "eight channels x five fields = 40 distinct keys, so nothing truncated into a collision");
  check(store.has("cfg_q0") && store.has("cfg_f0") && store.has("cfg_a0"),
        "the three EXISTING key names are unchanged, so an upgrade keeps a calibrated channel");
  check(store.has("cfg_c0") && store.has("cfg_p0"), "and the two new ones are written");
  check(store.has("cfg_c7") && store.has("cfg_p7"), "including on the highest channel");

  char key[plc::kSensorConfigKeyBytes];
  plc::formatSensorConfigKey(key, sizeof(key), plc::SensorConfigField::PulsesPerLitre, 7);
  check(std::string(key) == "cfg_p7", "the key format is <prefix><index> with no separator");
}

void upgradeTests() {
  std::printf("\n[the upgrade path: a device whose flash predates the two new keys]\n");

  // What the three-key firmware left behind for a working formula channel.
  FakeStore store;
  store.seedLegacy(3, 150, 6, -120);
  const SensorCharacteristics loaded = plc::loadSensorConfigFrom(store, 3);

  check(loaded.q_max == 150 && loaded.f_multiplier == 6 && loaded.adjust == -120,
        "the three legacy keys still load");
  check(loaded.calibration == CalibrationType::Formula,
        "a missing cfg_c defaults to Formula — exactly what the old firmware reconstructed");
  check(loaded.pulses_per_litre == 0, "and a missing cfg_p defaults to no K figure");

  /**
   * THE LOADER IS UNCHANGED; THE PREDICATE IS NOT (DF14).
   *
   * This asserted `configIsValid(loaded)` — "a channel that worked before the upgrade still works after
   * it" — and the legacy figures it seeds are `q_max = 150, multiplier = 6, adjust = -120`. That negative
   * offset is exactly the case DF14 refused: the engine computes `(F - adjust) / multiplier`, so at zero
   * pulses this channel reported `120 / 6 = 20 L/min` on a dry pipe and accumulated volume from it.
   *
   * So the honest assertion is the one below. The three legacy keys still LOAD — no data is lost, and a
   * re-calibration is all it takes — but the channel is no longer READY, and it renders `SET?` until
   * someone enters a non-negative offset or switches it to pulses-per-litre.
   *
   * This is a migration consequence, stated rather than papered over by editing the seed: a device whose
   * flash holds a negative offset stops measuring across this update. It is affordable because the fleet
   * is empty — nothing has run on hardware yet — which makes this the cheapest moment the rule will ever
   * have to tighten.
   */
  check(!configIsValid(loaded),
        "and a legacy NEGATIVE offset is no longer valid, so the channel reads SET? until re-calibrated");
  SensorCharacteristics repaired = loaded;
  repaired.adjust = 0;
  check(configIsValid(repaired),
        "one field is all it takes: the loaded q_max and multiplier are intact and still valid");
}

void corruptionTests() {
  std::printf("\n[a calibration word out of flash is not blindly cast into the enum]\n");

  FakeStore store;
  SensorCharacteristics cfg;
  cfg.q_max = 100;
  cfg.f_multiplier = 6;
  plc::saveSensorConfigTo(store, 2, cfg);

  store.forceCalibrationWord(2, 7);  // neither 0 nor 1 — a corrupt page, or a future firmware's form
  const SensorCharacteristics loaded = plc::loadSensorConfigFrom(store, 2);
  check(loaded.calibration == CalibrationType::Formula,
        "an unrecognised stored form becomes Formula rather than an out-of-range enum");
  check(static_cast<uint16_t>(loaded.calibration) == 0,
        "so operator== and the 60 s dirty check keep comparing a value the enum can hold");

  store.forceCalibrationWord(2, 1);
  check(plc::loadSensorConfigFrom(store, 2).calibration == CalibrationType::PulsesPerLitre,
        "and 1 still means pulses per litre");
}

}  // namespace

int main() {
  std::printf("Sensor calibration <-> NVS\n");
  roundTripTests();
  keyTests();
  upgradeTests();
  corruptionTests();
  std::printf("\n%s (%d checks, %d failures)\n", failures == 0 ? "ALL PASSED" : "FAILURES",
              checks, failures);
  return failures == 0 ? 0 : 1;
}
