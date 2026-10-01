#pragma once

#include <cstddef>
#include <cstdint>

#include "modbus/register_map.h"  // kNumSensors

namespace plc {

/**
 * The sensor cascade topology: which channel feeds which (`Sensor_Cascade_Topology.md` §3.1, T1).
 *
 * A channel stores ONE parent, and the forest that makes is the whole of the topology. Multiple roots
 * mean parallel branches, which is every installation that exists today and the default this module
 * must reduce to.
 *
 * WHY A MODULE RATHER THAN A FIELD ON `SensorData`. Three separate things need this arithmetic — the
 * netted aggregate (T2), the skew verification (T3) and the Modbus staged apply (T4) — and the rule
 * that matters most is a REFUSAL: §3.6 requires a write that would break the forest to change nothing.
 * A refusal spread across three call sites is a refusal that will eventually be half-applied, and a
 * half-applied topology is a total that is silently wrong. So the invariant lives with the data.
 *
 * ARDUINO-FREE, deliberately, and tested by `test/host/sensor_topology_test.cpp`. Cycle rejection and
 * out-of-service skipping are pure arithmetic on eight numbers; needing a board to find out whether a
 * two-cycle is refused is the blind spot `verification-blind-spots` keeps naming.
 *
 * NOTHING CALLS THIS YET. T1 is the module and its tests; the boot-time load lands with T2 (which is
 * the first consumer of an effective root) and the save with T4 (which is the first thing that can
 * change a parent). Stated here rather than left to be discovered, because a setting with no author is
 * exactly the shape of DF22, DF23 and DF24.
 */

/**
 * A parent is stored 1-BASED, with `0` meaning "this channel is a root".
 *
 * The zero is load-bearing in three places at once: it is R1.1's NVS default, so a device upgrading
 * from firmware without this feature reads the parallel topology it already had; it is the value a
 * factory reset leaves behind; and it is what an unwritten Modbus register holds. A 0-based parent
 * would make "channel 0 feeds me" and "I am a root" the same byte.
 *
 * So indices and parent values are two different vocabularies in this file. Channel INDICES are
 * 0-based everywhere (they index arrays); parent VALUES are 1-based. `hasParent`/`parentIndex` exist
 * so no caller has to do that conversion itself.
 */
inline constexpr std::uint8_t kTopologyRoot = 0;

/** Why a candidate topology was refused. Reported, never guessed at — §3.6 wants a reason on the bus. */
enum class TopologyError : std::uint8_t {
  None = 0,
  /** A parent naming a channel this device does not have. */
  ParentOutOfRange = 1,
  /** A channel named as its own parent — the likeliest operator slip, so it gets its own code. */
  SelfParent = 2,
  /** A closed loop of two or more channels. Nobody in the loop has a root, so nobody has a total. */
  Cycle = 3,
};

/**
 * The verdict on a candidate topology, with the channel that caused it.
 *
 * `index` is a 0-BASED channel index and is meaningful only when `error != None`. It is here because
 * "refused" alone is not actionable at a panel or on a register: the operator needs to know which row
 * to fix, and T4 publishes this pair.
 */
struct TopologyValidation {
  TopologyError error = TopologyError::None;
  std::size_t index = 0;

  bool ok() const { return error == TopologyError::None; }
};

/**
 * Is `parents[0..count)` a forest? — R1.3.
 *
 * Free function rather than a method so a test can exercise a three-channel forest without
 * constructing an eight-channel device, and so T4 can validate a STAGED set before committing it.
 *
 * R1.2's "depth is limited to 7 edges" is deliberately NOT a check of its own. An acyclic forest of
 * eight nodes cannot exceed seven edges, so a depth rule here could only ever reject a shape this
 * function has already accepted as legal — the redundant check nobody can trigger. The bound is
 * asserted from the other side, in the tests: a legal eight-channel chain measures exactly 7.
 */
inline TopologyValidation validateForest(const std::uint8_t* parents, std::size_t count) {
  TopologyValidation result;
  for (std::size_t i = 0; i < count; ++i) {
    const std::uint8_t parent = parents[i];
    if (parent == kTopologyRoot) {
      continue;
    }
    if (parent > count) {
      return TopologyValidation{TopologyError::ParentOutOfRange, i};
    }
    if (static_cast<std::size_t>(parent - 1) == i) {
      return TopologyValidation{TopologyError::SelfParent, i};
    }
  }
  // Walk up from every channel. A walk that has not reached a root after `count` steps is inside a
  // loop, because a forest of `count` nodes has no path longer than `count - 1` edges. Bounding the
  // walk is what makes this terminate on the very input it exists to reject.
  for (std::size_t i = 0; i < count; ++i) {
    std::size_t node = i;
    for (std::size_t step = 0; step <= count; ++step) {
      const std::uint8_t parent = parents[node];
      if (parent == kTopologyRoot) {
        break;
      }
      node = static_cast<std::size_t>(parent - 1);
      if (step == count) {
        return TopologyValidation{TopologyError::Cycle, i};
      }
    }
  }
  return result;
}

/**
 * The stored topology, which is only ever a forest.
 *
 * Default-constructs to all roots — the parallel topology — so a device that has never been told
 * otherwise behaves exactly as it did before this feature existed (R2.2 makes that bit-identity a
 * requirement rather than an aspiration, and T2 asserts it).
 */
class SensorTopology {
 public:
  static constexpr std::size_t kChannels = kNumSensors;

  // The in-service set is passed as a BITMAP, bit i for channel i, because that is the form the device
  // already keeps it in: `conn_map` in NVS and `connectedSensorsBitmap` in firmware.cpp. Converting to
  // a bool array at every call site would be a second representation of one fact.
  static_assert(kChannels <= 16, "the in-service mask is a uint16_t; widen it before adding channels");


  /** The stored parent of `index`: 1-based, or `kTopologyRoot`. Out-of-range reads as a root. */
  std::uint8_t parent(std::size_t index) const {
    return index < kChannels ? parents_[index] : kTopologyRoot;
  }

  bool hasParent(std::size_t index) const { return parent(index) != kTopologyRoot; }

  /** The 0-based index of `index`'s stored parent. Only meaningful when `hasParent(index)`. */
  std::size_t parentIndex(std::size_t index) const {
    return static_cast<std::size_t>(parent(index)) - 1;
  }

  /**
   * Validate and commit, or refuse and change NOTHING — §3.6.
   *
   * The all-or-nothing is the point. A per-channel setter would let a master write four of eight
   * parents, fail the fifth, and leave the device metering a shape nobody asked for.
   */
  TopologyValidation apply(const std::uint8_t (&candidate)[kChannels]) {
    const TopologyValidation verdict = validateForest(candidate, kChannels);
    if (!verdict.ok()) {
      return verdict;
    }
    for (std::size_t i = 0; i < kChannels; ++i) {
      parents_[i] = candidate[i];
    }
    return verdict;
  }

  /** Every channel a root again. What a factory reset leaves, and what a rejected load falls back to. */
  void clear() {
    for (std::size_t i = 0; i < kChannels; ++i) {
      parents_[i] = kTopologyRoot;
    }
  }

  /**
   * The nearest IN-SERVICE ancestor of `index`, 1-based, or `kTopologyRoot` for an effective root — R1.4.
   *
   * Out-of-service channels are skipped rather than treated as roots, which is decision 3 in §2 and the
   * rule that keeps the total correct when a mid-chain meter is taken out of service: the water still
   * flows through that pipe, so the channel below it is still downstream of the one above it. Promoting
   * it to a root instead would add its volume to the total that its grandparent already counts.
   *
   * `index` itself being out of service does not matter here — this answers "who counts my water",
   * which the caller asks precisely when deciding whether `index` contributes to the total.
   */
  std::uint8_t effectiveParent(std::size_t index, std::uint16_t inServiceMask) const {
    if (index >= kChannels) {
      return kTopologyRoot;
    }
    std::size_t node = index;
    // Bounded by the channel count for the same reason validateForest's walk is: the stored state is a
    // forest, but a corrupt flash page is not a promise, and `loadSensorTopologyFrom` is what turns one
    // into all-roots. This bound means a cycle that somehow reached here reports a root rather than
    // hanging the logic loop.
    for (std::size_t step = 0; step < kChannels; ++step) {
      if (!hasParent(node)) {
        return kTopologyRoot;
      }
      const std::size_t candidate = parentIndex(node);
      if ((inServiceMask & (1u << candidate)) != 0) {
        return static_cast<std::uint8_t>(candidate + 1);
      }
      node = candidate;
    }
    return kTopologyRoot;
  }

  /** Whether `index`'s water is counted by nobody else, and so belongs in the delivered total (R2.1). */
  bool isEffectiveRoot(std::size_t index, std::uint16_t inServiceMask) const {
    return effectiveParent(index, inServiceMask) == kTopologyRoot;
  }

  /**
   * Edges between `index` and its root, walking STORED parents and ignoring service state — R1.2.
   *
   * Stored rather than effective on purpose: this measures the shape an operator wired and is what a
   * "too deep" diagnostic would report. The effective depth of a chain changes every time a meter is
   * taken out of service, which would make the number mean something different every day.
   */
  std::size_t depth(std::size_t index) const {
    std::size_t node = index;
    for (std::size_t step = 0; step < kChannels; ++step) {
      if (!hasParent(node)) {
        return step;
      }
      node = parentIndex(node);
    }
    return kChannels;  // unreachable for a forest; see the bound in effectiveParent
  }

 private:
  std::uint8_t parents_[kChannels] = {};
};

/**
 * THE TRIPWIRE FOR THE NEXT FIELD, and it has a known first customer.
 *
 * `sensor_config_nvs.h:56` carries the same guard on `SensorCharacteristics` for the reason that file
 * spells out: a struct grew two fields while its serializer did not, and nothing related the two. This
 * class is one byte per channel and nothing else, so a ninth byte means somebody added state here —
 * and T3's COMMISSIONED BASELINE is already scheduled to want exactly that (§9's slice row says so).
 * When it does, this line is where the "and does it persist?" decision gets forced, rather than being
 * inherited from whichever file the field was typed into.
 *
 * Crude on purpose, like its model. A compile error naming this line is worth more than an elegant
 * scheme, because the failure it guards is silent until a power cycle.
 */
static_assert(sizeof(SensorTopology) == kNumSensors,
              "SensorTopology changed size: teach saveSensorTopologyTo and loadSensorTopologyFrom in "
              "sensors/sensor_topology_nvs.h about the new field, or record why it must not persist");

}  // namespace plc
