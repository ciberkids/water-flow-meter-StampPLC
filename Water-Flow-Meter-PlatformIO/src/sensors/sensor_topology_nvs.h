#pragma once

#include <cstdio>
#include <cstddef>
#include <cstdint>

#include "sensors/sensor_topology.h"
#include "storage/nvs_write_health.h"  // nvsPutOk — DF25

namespace plc {

/**
 * The cascade topology to and from non-volatile storage — R1.1.
 *
 * Templated on the store for the reason `modbus/sensor_config_nvs.h` spells out at length: firmware.cpp
 * is in no link set in `test/host/run.sh`, so a serializer that lives there is a serializer nothing can
 * test, and that is how a calibration serializer shipped writing three of five fields. The real
 * `Preferences` and a fake satisfy the same two calls with no virtual dispatch on the device:
 *
 *     size_t  putUChar(const char* key, uint8_t value)
 *     uint8_t getUChar(const char* key, uint8_t defaultValue)
 *
 * The `size_t` is the bytes written, 0 on failure, and it is CHECKED — see `saveSensorTopologyTo` and
 * `DF25`.
 *
 * SEPARATE FROM THE CALIBRATION SERIALIZER, deliberately. `SensorCharacteristics` is tripwired at
 * `sensor_config_nvs.h:56` with a `static_assert` on its size so a sixth field cannot be added without
 * someone deciding whether it persists — and for `parent` the answer is "persist it, but not there".
 * Calibration describes a METER; topology describes PLUMBING. A meter swap changes one and not the
 * other, and `SensorCharacteristics::operator==` is used as the 60-second dirty check, so folding
 * topology in would make a re-plumb look like a recalibration.
 */

/**
 * `parent_0` … `parent_7`, and the names are spelled out on purpose.
 *
 * The existing calibration keys are `cfg_q`, `cfg_f`, `cfg_a`, `cfg_c`, `cfg_p` and they are FROZEN —
 * `sensor_config_nvs.h` explains that renaming one silently discards a calibrated channel on upgrade,
 * because the old key is what the device has in flash. That is a migration constraint, not a house
 * style. The platform caps a key at 15 characters, `parent_7` is eight, so a new key has no excuse to
 * be an abbreviation. Do not "tidy" the `cfg_*` keys to match; read the comment there first.
 */
inline constexpr const char* kSensorParentKeyPrefix = "parent_";

/** Seven for the prefix, up to three for the index, one for the NUL, rounded up. */
inline constexpr std::size_t kSensorTopologyKeyBytes = 12;

/** Writes `parent_<index>` into `out`. */
inline void formatSensorParentKey(char* out, std::size_t size, std::size_t index) {
  std::snprintf(out, size, "%s%u", kSensorParentKeyPrefix, static_cast<unsigned>(index));
}

/**
 * What came back from flash, and whether it could be trusted.
 *
 * `stored.ok() == false` means the stored set was NOT a forest and `topology` is the all-roots
 * fallback. That is reported rather than absorbed: it is the same class of event as DF23's unassigned
 * baseline, and a device that quietly re-parented every channel after a corrupt read would publish a
 * plausible total that no operator asked for. The caller decides what to do with it — T4 surfaces it,
 * and until then it is at least visible to a test.
 */
struct SensorTopologyLoad {
  SensorTopology topology;
  TopologyValidation stored;
};

/**
 * Persists all eight parents, and returns HOW MANY FAILED — `DF25`.
 *
 * Writes every channel rather than only the changed ones. A topology commits atomically (§3.6), so the
 * eight keys are one value in eight boxes, and a partial write is the state this whole module exists to
 * make impossible. Eight writes on an operator action is nothing: the wear budget is set by the
 * once-a-minute litre writes, and a re-plumb is not a once-a-minute event.
 *
 * A NON-ZERO RETURN IS EXACTLY THE PARTIAL STATE THIS MODULE REFUSES TO CREATE IN RAM, so the caller
 * cannot treat it as cosmetic: the forest in memory is valid and the one in flash is not, and the next
 * boot will load the difference. `loadSensorTopologyFrom` is what catches it — a partial write that is
 * no longer a forest falls back to all-roots and says so — but a partial write that happens to REMAIN a
 * forest is a different topology, silently. `[[nodiscard]]` for the same reason its neighbour has it.
 */
template <typename Store>
[[nodiscard]] std::size_t saveSensorTopologyTo(Store& store, const SensorTopology& topology) {
  char key[kSensorTopologyKeyBytes];
  std::size_t failed = 0;
  for (std::size_t i = 0; i < SensorTopology::kChannels; ++i) {
    formatSensorParentKey(key, sizeof(key), i);
    failed += nvsPutOk(store.putUChar(key, topology.parent(i)), sizeof(std::uint8_t)) ? 0 : 1;
  }
  return failed;
}

/**
 * Restores the topology, or substitutes the parallel one and says so.
 *
 * A missing key returns `kTopologyRoot`, which is R1.1's upgrade path: a device with firmware that
 * predates this feature has no `parent_*` keys, reads all zeros, and is the parallel topology it
 * already was. Nothing needs migrating and nothing needs writing at boot.
 *
 * A stored set that is not a forest becomes ALL ROOTS, not a best-effort repair. The same judgement
 * `loadSensorConfigFrom` makes about an unrecognised calibration enum: flash can hold anything a
 * corrupt page or a future firmware wrote, and there is exactly one substitute here that cannot
 * over-count water — every channel counting only itself, which is the shape the device shipped with.
 * Repairing a cycle by cutting one edge would guess at which meter feeds which.
 */
template <typename Store>
SensorTopologyLoad loadSensorTopologyFrom(Store& store) {
  std::uint8_t parents[SensorTopology::kChannels] = {};
  char key[kSensorTopologyKeyBytes];
  for (std::size_t i = 0; i < SensorTopology::kChannels; ++i) {
    formatSensorParentKey(key, sizeof(key), i);
    parents[i] = store.getUChar(key, kTopologyRoot);
  }

  SensorTopologyLoad result;
  result.stored = validateForest(parents, SensorTopology::kChannels);
  if (result.stored.ok()) {
    result.topology.apply(parents);  // cannot refuse: the same set just validated
  }
  // Otherwise `result.topology` is left as constructed — every channel a root, which IS the
  // substitution. Spelled out because "the fallback is the default constructor" is the kind of
  // silence that reads as a forgotten else-arm.
  return result;
}

}  // namespace plc
