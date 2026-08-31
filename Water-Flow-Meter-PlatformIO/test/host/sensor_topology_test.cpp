// The sensor cascade topology — T1 of `Sensor_Cascade_Topology.md`.
//
// Everything here is a rule that would otherwise need a board, a second meter and a length of pipe to
// find out about. Three of them are the kind that stay hidden until a total is quietly wrong:
//
//   * a REFUSED write must change nothing (§3.6) — a half-applied topology means the device is
//     metering a shape nobody asked for, and it looks exactly like a shape somebody did;
//   * an out-of-service mid-chain meter must be SKIPPED, not treated as a root (R1.4) — treating it as
//     a root adds its branch to a total that its grandparent already counts, so the number goes UP when
//     a meter is switched off;
//   * a corrupt flash page must fall back to the parallel topology and SAY it did — the failure this
//     project keeps rediscovering is not a wrong value, it is a wrong value nobody can see.
//
// The default case is asserted first and hardest: with every parent at 0 this module must describe the
// installation the firmware already has, because R2.2 makes that reduction bit-identical rather than
// merely equivalent.
#include "sensors/sensor_topology_nvs.h"

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

using plc::SensorTopology;
using plc::TopologyError;
using plc::kTopologyRoot;

constexpr std::size_t kChannels = SensorTopology::kChannels;
/** Every channel in service — the mask the effective-parent rules are interesting against. */
constexpr std::uint16_t kAllInService = 0x00FF;

/** A candidate topology, spelled 1-based exactly as it is stored and as Modbus will carry it. */
struct Candidate {
  std::uint8_t parents[kChannels] = {};
};

/**
 * A stand-in for `Preferences`, faithful in the behaviour that carries R1.1: a key that was never
 * written returns the caller's default. That is the upgrade path — a device with older firmware has no
 * `parent_*` keys at all — so the fake must not pretend the keys exist.
 *
 * Keys are recorded so a truncating or colliding name shows up as a count rather than as two channels
 * mysteriously sharing a parent.
 */
class FakeStore {
 public:
  void putUChar(const char* key, std::uint8_t value) {
    values_[key] = value;
    written_.insert(key);
  }
  std::uint8_t getUChar(const char* key, std::uint8_t defaultValue) const {
    const auto it = values_.find(key);
    return it == values_.end() ? defaultValue : it->second;
  }

  /** Writes a value the API would have refused — a corrupt page, or a future firmware's shape. */
  void forceRaw(std::size_t index, std::uint8_t value) {
    char key[plc::kSensorTopologyKeyBytes];
    plc::formatSensorParentKey(key, sizeof(key), index);
    values_[key] = value;
  }

  std::size_t distinctKeys() const { return written_.size(); }
  bool holds(const char* key) const { return values_.find(key) != values_.end(); }

 private:
  std::map<std::string, std::uint8_t> values_;
  std::set<std::string> written_;
};

// ---------------------------------------------------------------------------------------------------
// The default: what every installation that exists today looks like.
// ---------------------------------------------------------------------------------------------------

void defaultTopologyTests() {
  std::printf("\nDefault topology — the parallel installation (R1.1, R2.2)\n");
  const SensorTopology topology;

  bool allRoots = true;
  bool allZeroDepth = true;
  bool allEffectiveRoots = true;
  for (std::size_t i = 0; i < kChannels; ++i) {
    if (topology.parent(i) != kTopologyRoot || topology.hasParent(i)) allRoots = false;
    if (topology.depth(i) != 0) allZeroDepth = false;
    if (!topology.isEffectiveRoot(i, kAllInService)) allEffectiveRoots = false;
  }
  check(allRoots, "a default-constructed topology has every channel a root");
  check(allZeroDepth, "and every depth is 0 edges");
  check(allEffectiveRoots,
        "and every channel is an effective root, so R2.1's sum is today's index set");

  // The index set R2.1 iterates must be ascending and complete, because R2.2 requires the same
  // IEEE-754 accumulation order, not merely the same total.
  std::size_t expected = 0;
  bool ascendingAndComplete = true;
  for (std::size_t i = 0; i < kChannels; ++i) {
    if (!topology.isEffectiveRoot(i, kAllInService)) continue;
    if (i != expected) ascendingAndComplete = false;
    ++expected;
  }
  check(ascendingAndComplete && expected == kChannels,
        "the effective roots enumerate 0..7 ascending with nothing skipped");

  check(topology.isEffectiveRoot(0, 0),
        "with nothing in service a root is still a root — service state cannot invent a parent");
}

// ---------------------------------------------------------------------------------------------------
// R1.3: what is refused, and that a refusal changes nothing.
// ---------------------------------------------------------------------------------------------------

void validationTests() {
  std::printf("\nValidation — R1.3, and §3.6's refusal changes nothing\n");

  SensorTopology topology;

  Candidate chain;  // 1 <- 2 <- 3 ... a legal eight-channel cascade
  for (std::size_t i = 1; i < kChannels; ++i) {
    chain.parents[i] = static_cast<std::uint8_t>(i);
  }
  check(topology.apply(chain.parents).ok(), "an eight-channel chain is a legal forest");
  check(topology.depth(kChannels - 1) == kChannels - 1,
        "and its deepest channel measures 7 edges — R1.2's bound, reached from the legal side");

  Candidate outOfRange;
  outOfRange.parents[0] = static_cast<std::uint8_t>(kChannels + 1);
  plc::TopologyValidation verdict = topology.apply(outOfRange.parents);
  check(verdict.error == TopologyError::ParentOutOfRange,
        "a parent naming a channel the device does not have is refused");
  check(verdict.index == 0, "and the refusal names the channel at fault");

  Candidate selfParent;
  selfParent.parents[3] = 4;  // channel index 3 is parent value 4
  verdict = topology.apply(selfParent.parents);
  check(verdict.error == TopologyError::SelfParent, "a channel named as its own parent is refused");
  check(verdict.index == 3, "with its own index reported");

  Candidate twoCycle;
  twoCycle.parents[0] = 2;
  twoCycle.parents[1] = 1;
  verdict = topology.apply(twoCycle.parents);
  check(verdict.error == TopologyError::Cycle, "a two-channel loop is refused");

  Candidate longCycle;
  for (std::size_t i = 0; i < kChannels; ++i) {
    longCycle.parents[i] = static_cast<std::uint8_t>(((i + 1) % kChannels) + 1);
  }
  check(topology.apply(longCycle.parents).error == TopologyError::Cycle,
        "and so is a loop that closes only after all eight channels");

  // The whole point of apply() being the only mutator.
  bool chainIntact = topology.parent(0) == kTopologyRoot;
  for (std::size_t i = 1; i < kChannels; ++i) {
    if (topology.parent(i) != static_cast<std::uint8_t>(i)) chainIntact = false;
  }
  check(chainIntact,
        "after four refusals the stored chain is untouched — a refused write changes nothing");

  Candidate forest;  // two independent branches (0->1->2 and 3->4) plus three bare roots
  forest.parents[1] = 1;
  forest.parents[2] = 2;
  forest.parents[4] = 4;
  check(topology.apply(forest.parents).ok(), "several roots with branches under them is a forest");
  check(topology.depth(2) == 2 && topology.depth(4) == 1 && topology.depth(5) == 0,
        "and depth is measured per branch, not per device");
}

// ---------------------------------------------------------------------------------------------------
// R1.4: the rule that makes an out-of-service meter safe.
// ---------------------------------------------------------------------------------------------------

void effectiveParentTests() {
  std::printf("\nEffective parent — R1.4, decision 3 of §2\n");

  SensorTopology topology;
  Candidate chain;
  chain.parents[1] = 1;  // channel 1 feeds channel 2 (indices 0 -> 1 -> 2)
  chain.parents[2] = 2;
  check(topology.apply(chain.parents).ok(), "a three-channel chain applies");

  check(topology.effectiveParent(2, kAllInService) == 2,
        "with everything in service the effective parent is the stored one");
  check(topology.effectiveParent(0, kAllInService) == kTopologyRoot,
        "and the head of the chain is an effective root");

  const std::uint16_t withoutMiddle = static_cast<std::uint16_t>(kAllInService & ~(1u << 1));
  check(topology.effectiveParent(2, withoutMiddle) == 1,
        "an out-of-service mid-chain meter is SKIPPED — the channel below re-parents to its grandparent");
  check(topology.parent(2) == 2,
        "and the STORED parent is untouched, so putting the meter back needs no rewrite");

  const std::uint16_t withoutBoth = static_cast<std::uint16_t>(kAllInService & ~0x0003u);
  check(topology.effectiveParent(2, withoutBoth) == kTopologyRoot,
        "a walk that finds no in-service ancestor makes the channel an effective root");
  check(topology.isEffectiveRoot(2, withoutBoth),
        "so its volume enters the total rather than vanishing from it");

  // The failure this rule prevents, stated as arithmetic: how many channels feed the total.
  std::size_t rootsAllInService = 0;
  std::size_t rootsWithoutMiddle = 0;
  for (std::size_t i = 0; i < 3; ++i) {
    if (topology.isEffectiveRoot(i, kAllInService)) ++rootsAllInService;
    if (topology.isEffectiveRoot(i, withoutMiddle)) ++rootsWithoutMiddle;
  }
  check(rootsAllInService == 1 && rootsWithoutMiddle == 1,
        "taking a mid-chain meter out of service does not add a second root to the total");
}

// ---------------------------------------------------------------------------------------------------
// Persistence: the keys, the upgrade path, and a flash page that cannot be trusted.
// ---------------------------------------------------------------------------------------------------

void persistenceTests() {
  std::printf("\nTopology <-> non-volatile storage — R1.1\n");

  FakeStore store;
  const plc::SensorTopologyLoad fresh = plc::loadSensorTopologyFrom(store);
  bool freshAllRoots = true;
  for (std::size_t i = 0; i < kChannels; ++i) {
    if (fresh.topology.hasParent(i)) freshAllRoots = false;
  }
  check(freshAllRoots,
        "a store with no parent_* keys loads as all roots — R1.1's upgrade path, no migration");
  check(fresh.stored.ok(), "and an absent topology is not reported as a corrupt one");
  check(store.distinctKeys() == 0, "a load writes nothing, so an untouched device stays untouched");

  SensorTopology topology;
  Candidate forest;
  forest.parents[1] = 1;
  forest.parents[2] = 2;
  forest.parents[6] = 8;
  check(topology.apply(forest.parents).ok(), "a forest with a deep branch and a back-reference applies");

  plc::saveSensorTopologyTo(store, topology);
  check(store.distinctKeys() == kChannels,
        "saving writes one distinct key per channel — no truncation collision");
  check(store.holds("parent_0") && store.holds("parent_7"),
        "and the keys are spelled parent_0..parent_7, not an abbreviation");

  const plc::SensorTopologyLoad reloaded = plc::loadSensorTopologyFrom(store);
  bool roundTripped = reloaded.stored.ok();
  for (std::size_t i = 0; i < kChannels; ++i) {
    if (reloaded.topology.parent(i) != topology.parent(i)) roundTripped = false;
  }
  check(roundTripped, "and every parent survives the round trip, including the 0s");
  check(reloaded.topology.effectiveParent(2, kAllInService) == 2,
        "so the reloaded topology answers R1.4 the same way it did before the power cycle");
}

void corruptionTests() {
  std::printf("\nA flash page that cannot be trusted\n");

  FakeStore store;
  store.forceRaw(0, 2);
  store.forceRaw(1, 1);  // a two-cycle, which no API call could have produced
  const plc::SensorTopologyLoad cyclic = plc::loadSensorTopologyFrom(store);
  check(!cyclic.stored.ok(), "a stored set that is not a forest is REPORTED, not absorbed");
  check(cyclic.stored.error == TopologyError::Cycle, "with the reason it failed");
  bool fellBackToParallel = true;
  for (std::size_t i = 0; i < kChannels; ++i) {
    if (cyclic.topology.hasParent(i)) fellBackToParallel = false;
  }
  check(fellBackToParallel,
        "and the substitute is the PARALLEL topology — the one shape that cannot over-count water");

  FakeStore ranged;
  ranged.forceRaw(3, 200);
  const plc::SensorTopologyLoad wild = plc::loadSensorTopologyFrom(ranged);
  check(wild.stored.error == TopologyError::ParentOutOfRange && !wild.topology.hasParent(3),
        "a parent value of 200 out of a corrupt page is refused the same way");

  // The whole set is discarded rather than repaired one edge at a time: with 3 corrupt, channel 1's
  // perfectly good parent goes too. That is the deliberate choice — repairing would guess which meter
  // feeds which, and a guessed topology publishes a plausible number nobody authorised.
  FakeStore mixed;
  mixed.forceRaw(1, 1);
  mixed.forceRaw(3, 200);
  const plc::SensorTopologyLoad partial = plc::loadSensorTopologyFrom(mixed);
  check(!partial.topology.hasParent(1),
        "one bad parent discards the whole stored set rather than repairing around it");
}

}  // namespace

int main() {
  std::printf("Sensor cascade topology (T1)\n");
  defaultTopologyTests();
  validationTests();
  effectiveParentTests();
  persistenceTests();
  corruptionTests();
  std::printf("\n%s (%d checks, %d failures)\n", failures == 0 ? "ALL PASSED" : "FAILURES", checks,
              failures);
  return failures == 0 ? 0 : 1;
}
