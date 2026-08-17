#pragma once

#include <cstdio>
#include <cstddef>
#include <cstdint>

#include "modbus/sensor_types.h"

namespace plc {

/**
 * A channel's calibration, to and from NVS — the WHOLE struct, which it was not.
 *
 * `saveSensorConfig` in firmware.cpp wrote three keys: `cfg_q`, `cfg_f` and `cfg_a`. `SensorCharacteristics`
 * has FIVE fields. `calibration` and `pulses_per_litre` were never written and never read, so a master
 * that wrote registers 123 and 124 — the pulses-per-litre form and its K figure — got them accepted,
 * saw them read back correctly for as long as the device stayed up, and lost both at the next power
 * cycle. The channel came back as `Formula` with `pulses_per_litre = 0`, which fails `configIsValid`,
 * so it reported `SET?` on the panel and published 0.0 for its instantaneous flow, its session volume
 * and its lifetime total.
 *
 * THE LITRES WERE NEVER LOST, which is why this took a while to see: `cml_%u` round-trips and the
 * cumulative total is intact in NVS and in RAM. The zero a master reads at register 103 after the
 * reboot is DERIVED — `syncSensorToHolding` gates the six measured fields on
 * `inUse && configIsValid(configs[i])` and publishes 0.0 in the else arm. So the total reappears the
 * moment the calibration is re-entered. That gate is correct and deliberately left alone: deriving
 * readiness instead of caching it is what fixed the `SensorData::isReady` bug, and re-caching it here
 * would be that bug's third life.
 *
 * WHY THIS IS A HEADER, and templated on the store rather than calling Preferences directly:
 * firmware.cpp is in no link set in `test/host/run.sh`, so nothing in it can be host-tested — which is
 * how a serializer missing two of five fields shipped in the first place. `src/sensors/pulse_counter.h`
 * was carved out of firmware.cpp for exactly this reason, and this follows it. The store is a template
 * parameter so the real `Preferences` and a fake one satisfy the same four calls with no virtual
 * dispatch on the device:
 *
 *     void     putUShort(const char* key, uint16_t value)
 *     uint16_t getUShort(const char* key, uint16_t defaultValue)
 *     void     putShort (const char* key, int16_t  value)
 *     int16_t  getShort (const char* key, int16_t  defaultValue)
 *
 * (`Preferences::put*` return `size_t`; the return is deliberately ignored, as it was before.)
 */

/**
 * THE TRIPWIRE FOR THE NEXT FIELD.
 *
 * The defect this file fixes was not a typo — it was a struct that grew two fields while its serializer
 * did not, and nothing anywhere related the two. Five `uint16_t`-sized members are 10 bytes; a sixth
 * field makes this fail, which is the moment somebody has to decide whether it belongs in NVS. Say no
 * and widen the number, deliberately, with a reason.
 *
 * Crude on purpose. A compile error naming this line is worth more than an elegant scheme, because the
 * failure mode it guards is silent for as long as nobody power-cycles a freshly configured channel.
 */
static_assert(sizeof(SensorCharacteristics) == 10,
              "SensorCharacteristics changed size: add the new field to saveSensorConfigTo and "
              "loadSensorConfigFrom below, or record why it must not persist");

/** The five fields, so no call site spells a key prefix twice. */
enum class SensorConfigField {
  QMax,
  FMultiplier,
  Adjust,
  Calibration,
  PulsesPerLitre
};

/**
 * The NVS key prefix per field.
 *
 * `cfg_q`, `cfg_f` and `cfg_a` are the three that already exist and MUST NOT CHANGE — a device
 * upgrading to this firmware has them in flash, and renaming one would silently discard a calibrated
 * channel's q_max. `cfg_c` and `cfg_p` are new, and their absence on such a device is what makes the
 * upgrade safe: `getUShort` returns the default, which is `Formula` with no K figure — precisely the
 * state the old three-key firmware reconstructed, so nothing changes for a channel that was working.
 */
constexpr const char* sensorConfigKeyPrefix(SensorConfigField field) {
  switch (field) {
    case SensorConfigField::QMax:           return "cfg_q";
    case SensorConfigField::FMultiplier:    return "cfg_f";
    case SensorConfigField::Adjust:         return "cfg_a";
    case SensorConfigField::Calibration:    return "cfg_c";
    case SensorConfigField::PulsesPerLitre: return "cfg_p";
  }
  return "";
}

/**
 * Bytes a key buffer needs: five for the prefix, up to three for the index, one for the NUL.
 *
 * Sized generously on purpose. The old buffer was `char[10]`, which fits — but a truncating `snprintf`
 * would make two channels share one key and silently merge their calibrations, which is a worse bug
 * than the one this file fixes. NVS itself caps a key at 15 characters, so this stays well inside.
 */
inline constexpr std::size_t kSensorConfigKeyBytes = 12;

/** Writes `<prefix><index>` into `out`. */
inline void formatSensorConfigKey(char* out, std::size_t size, SensorConfigField field,
                                  std::size_t index) {
  std::snprintf(out, size, "%s%u", sensorConfigKeyPrefix(field), static_cast<unsigned>(index));
}

/** Persists all five fields of one channel's calibration. */
template <typename Store>
void saveSensorConfigTo(Store& store, std::size_t index, const SensorCharacteristics& cfg) {
  char key[kSensorConfigKeyBytes];
  formatSensorConfigKey(key, sizeof(key), SensorConfigField::QMax, index);
  store.putUShort(key, cfg.q_max);
  formatSensorConfigKey(key, sizeof(key), SensorConfigField::FMultiplier, index);
  store.putShort(key, cfg.f_multiplier);
  formatSensorConfigKey(key, sizeof(key), SensorConfigField::Adjust, index);
  store.putShort(key, cfg.adjust);
  formatSensorConfigKey(key, sizeof(key), SensorConfigField::Calibration, index);
  store.putUShort(key, static_cast<uint16_t>(cfg.calibration));
  formatSensorConfigKey(key, sizeof(key), SensorConfigField::PulsesPerLitre, index);
  store.putUShort(key, cfg.pulses_per_litre);
}

/**
 * Restores all five fields of one channel's calibration.
 *
 * An unrecognised stored `calibration` becomes `Formula` rather than being cast blindly into the enum.
 * The value comes out of flash, so it can be anything a corrupted page or a future firmware wrote, and
 * a `CalibrationType` holding 7 would fall through every `if (calibration == PulsesPerLitre)` in the
 * codebase and be treated as the formula form anyway — this just makes that explicit instead of
 * relying on it, and keeps the struct's invariant true for `operator==` and the dirty check.
 */
template <typename Store>
SensorCharacteristics loadSensorConfigFrom(Store& store, std::size_t index) {
  SensorCharacteristics cfg;
  char key[kSensorConfigKeyBytes];
  formatSensorConfigKey(key, sizeof(key), SensorConfigField::QMax, index);
  cfg.q_max = store.getUShort(key, 0);
  formatSensorConfigKey(key, sizeof(key), SensorConfigField::FMultiplier, index);
  cfg.f_multiplier = store.getShort(key, 0);
  formatSensorConfigKey(key, sizeof(key), SensorConfigField::Adjust, index);
  cfg.adjust = store.getShort(key, 0);
  formatSensorConfigKey(key, sizeof(key), SensorConfigField::Calibration, index);
  const uint16_t calibration = store.getUShort(key, static_cast<uint16_t>(CalibrationType::Formula));
  cfg.calibration = calibration == static_cast<uint16_t>(CalibrationType::PulsesPerLitre)
                        ? CalibrationType::PulsesPerLitre
                        : CalibrationType::Formula;
  formatSensorConfigKey(key, sizeof(key), SensorConfigField::PulsesPerLitre, index);
  cfg.pulses_per_litre = store.getUShort(key, 0);
  return cfg;
}

}  // namespace plc
