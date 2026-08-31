// Host tests for SensorStateEngine's readiness aggregation.
//
// The green LED is the operator's only at-a-glance signal that the device is configured and
// working. It was unreachable on any installation using fewer than eight channels, which is
// every realistic installation, and nothing caught it because this engine had no test.
#include "sensors/sensor_state_engine.h"

#include <cstdio>
#include <cstdint>

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

/**
 * Runs the engine over a given enable/calibrated pattern and reports the aggregated readiness.
 *
 * "Calibrated" is expressed the way the device expresses it — through the CONFIGURATION — because
 * readiness is `configIsValid(configs[i])` and no longer a bit a test can set behind the engine's back.
 * Setting that bit by hand is precisely why the suite stayed green while a reboot silently disabled every
 * calibrated channel.
 */
bool readinessFor(std::size_t count, const bool* inUse, const bool* calibrated) {
  SensorData sensors[plc::kNumSensors] = {};
  SensorCharacteristics configs[plc::kNumSensors] = {};
  double totalLiters = 0.0;
  double aggFlow = 0.0;
  bool allReady = false;
  uint16_t undersampling = 0;

  for (std::size_t i = 0; i < count; ++i) {
    sensors[i].inUse = inUse[i];
    // q_max = 0 fails configIsValid, which is exactly what "not calibrated" means on the device.
    configs[i].q_max = calibrated[i] ? 100 : 0;
    configs[i].f_multiplier = 1;
  }

  plc::SensorStateEngine::Dependencies deps;
  deps.sensors = sensors;
  deps.configs = configs;
  deps.sensorCount = count;
  deps.registerBank = nullptr;
  deps.modbusManager = nullptr;  // guarded by a null check in update()
  deps.totalSessionLitersCache = &totalLiters;
  deps.aggregateFlowLpmCache = &aggFlow;
  deps.allSensorsReadyCache = &allReady;
  deps.undersamplingFlags = &undersampling;

  plc::SensorStateEngine engine(deps);
  engine.update(1.0f);
  return allReady;
}

/**
 * A disconnected sensor must contribute nothing to anything reported (owner request, 2026-08-05).
 *
 * The engine gates on `sensor.inUse`, and the point of these checks is that the gate covers the
 * AGGREGATES too — not just the per-sensor numbers. A disabled channel carrying stale volume that
 * still summed into totalSessionLiters would be the worst kind of wrong: a plausible total, with no
 * per-sensor row to trace it back to.
 *
 * Breaks if: the `if (sensor.inUse)` guard is removed, or the aggregate sums move outside it.
 */
void disconnectedSensorTests() {
  std::printf("\n[a disconnected sensor contributes nothing — owner request]\n");

  SensorData sensors[plc::kNumSensors] = {};
  SensorCharacteristics configs[plc::kNumSensors] = {};
  double totalLiters = 0.0;
  double aggFlow = 0.0;
  bool allReady = false;
  uint16_t undersampling = 0;

  // Channel 0 is live and flowing. Channel 1 is DISABLED but deliberately loaded with stale state —
  // pulses pending, volume on the clock — exactly what a sensor that was just switched off looks
  // like before anything clears it.
  for (std::size_t i = 0; i < plc::kNumSensors; ++i) {
    configs[i].q_max = 100;
    configs[i].f_multiplier = 1;
  }
  sensors[0].inUse = true;         // calibrated by the loop above
  sensors[0].pulseCount = 60;  // 60 pulses in 1 s, F=1 -> 60 L/min -> 1 L/s

  sensors[1].inUse = false;         // disconnected, but still calibrated by the loop above
  sensors[1].pulseCount = 6000;     // and a large backlog is sitting there
  sensors[1].sessionLiters = 999.0f;
  sensors[1].cumulativeLiters = 12345.0;
  sensors[1].instantFlow_L_min = 42.0f;

  plc::SensorStateEngine::Dependencies deps;
  deps.sensors = sensors;
  deps.configs = configs;
  deps.sensorCount = plc::kNumSensors;
  deps.registerBank = nullptr;
  deps.modbusManager = nullptr;
  deps.totalSessionLitersCache = &totalLiters;
  deps.aggregateFlowLpmCache = &aggFlow;
  deps.allSensorsReadyCache = &allReady;
  deps.undersamplingFlags = &undersampling;

  plc::SensorStateEngine engine(deps);
  engine.update(1.0f);

  std::printf("      ch0 flow=%.2f L/min session=%.2f L | totals: flow=%.2f L/min volume=%.2f L\n",
              static_cast<double>(sensors[0].instantFlow_L_min),
              static_cast<double>(sensors[0].sessionLiters), aggFlow, totalLiters);

  check(sensors[0].instantFlow_L_min > 59.0f && sensors[0].instantFlow_L_min < 61.0f,
        "the live sensor converted its pulses (60 pulses, F=1 -> about 60 L/min)");

  // The aggregates are the part worth asserting: 999 L and a stale 42 L/min sit in the array. The
  // bound is above the live channel's own 60 L/min and below 60 + 42, so it can only pass by
  // excluding the disconnected one.
  check(aggFlow < 61.0,
        "aggregate flow EXCLUDES the disconnected sensor's stale 42 L/min");
  check(totalLiters < 1.1,
        "and aggregate volume excludes its stale 999 L — a plausible wrong total is the worst kind");

  check(sensors[1].pulseCount == 6000,
        "the disabled channel's pulses are neither converted nor cleared by the engine, which is "
        "exactly why the polling mask must come from inUse and not a second copy of it");
  check(sensors[1].sessionLiters == 999.0f,
        "and its stale volume is left untouched rather than silently folded in");

  // Readiness must ignore it too, or one disconnected channel would hold the green LED off forever.
  check(allReady, "readiness ignores the disconnected channel (RGB_LED_Behavior.md §3.2)");
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

/**
 * The case that did not exist, and whose absence let a reboot lose customer data.
 *
 * Boot restores the calibration and the cumulative total from NVS and writes NO configuration through the
 * Modbus path. Under the old design that left `isReady` false forever: the engine counted pulses, cleared
 * the counter, and discarded them, so a calibrated channel accrued nothing and its lifetime total was
 * published as 0.0. Nothing in the suite noticed, because every other test set the readiness bit by hand.
 *
 * The only state this test creates is what `loadSensorConfig` and `loadCumulativeData` would have restored.
 */
void bootRestoreTests() {
  std::printf("\nAfter a reboot, with configuration restored and no config write\n");

  SensorData sensors[plc::kNumSensors] = {};
  SensorCharacteristics configs[plc::kNumSensors] = {};
  double totalLiters = 0.0;
  double aggFlow = 0.0;
  bool allReady = false;
  uint16_t undersampling = 0;

  // Exactly what boot restores: a calibrated channel, enabled, carrying its persisted lifetime total.
  configs[0].q_max = 100;
  configs[0].f_multiplier = 1;
  sensors[0].inUse = true;
  sensors[0].cumulativeLiters = 4321.0;
  sensors[0].pulseCount = 60;  // 60 pulses in 1 s, F=1 -> 60 L/min -> 1 L/s

  plc::SensorStateEngine::Dependencies deps;
  deps.sensors = sensors;
  deps.configs = configs;
  deps.sensorCount = plc::kNumSensors;
  deps.registerBank = nullptr;
  deps.modbusManager = nullptr;
  deps.totalSessionLitersCache = &totalLiters;
  deps.aggregateFlowLpmCache = &aggFlow;
  deps.allSensorsReadyCache = &allReady;
  deps.undersamplingFlags = &undersampling;

  plc::SensorStateEngine engine(deps);
  engine.update(1.0f);

  check(sensors[0].instantFlow_L_min > 59.0f && sensors[0].instantFlow_L_min < 61.0f,
        "a restored calibrated channel computes flow with no config write");
  check(sensors[0].cumulativeLiters > 4321.0,
        "and its restored lifetime total ADVANCES rather than staying frozen");
  check(sensors[0].pulseCount == 0, "pulses are consumed, not merely discarded");
  check(allReady, "readiness follows the restored configuration");
}


// ===================================================================================================
// T2 — the DELIVERED aggregate (`Sensor_Cascade_Topology.md` R2.1-R2.5)
// ===================================================================================================

/**
 * One pass of the engine over a chosen topology, with the per-channel volumes SEEDED.
 *
 * Seeded rather than metered, and `pulseCount` left at 0, because that makes every oracle in these
 * tests an exact double the test can recompute: with no pulses the interval is 0 L, so `sessionLiters`
 * keeps the seed it was given. The flow case below meters real pulses instead.
 */
struct DeliveredRun {
  double gross = 0.0;
  double grossFlow = 0.0;
  double delivered = 0.0;
  double deliveredFlow = 0.0;
  uint16_t unknownBranches = 0;
};

DeliveredRun runDelivered(const plc::SensorTopology* topology,
                          const bool* inUse,
                          const bool* calibrated,
                          const float* seedLiters,
                          const uint32_t* pulses) {
  static SensorData sensors[plc::kNumSensors];
  static SensorCharacteristics configs[plc::kNumSensors];
  for (std::size_t i = 0; i < plc::kNumSensors; ++i) {
    sensors[i] = SensorData{};
    configs[i] = SensorCharacteristics{};
    sensors[i].inUse = inUse[i];
    sensors[i].sessionLiters = seedLiters[i];
    sensors[i].pulseCount = pulses == nullptr ? 0u : pulses[i];
    configs[i].q_max = calibrated[i] ? 100 : 0;  // q_max = 0 is how the device says "not calibrated"
    configs[i].f_multiplier = 1;
  }

  DeliveredRun out;
  bool allReady = false;
  uint16_t undersampling = 0;
  plc::SensorStateEngine::Dependencies deps;
  deps.sensors = sensors;
  deps.configs = configs;
  deps.sensorCount = plc::kNumSensors;
  deps.totalSessionLitersCache = &out.gross;
  deps.aggregateFlowLpmCache = &out.grossFlow;
  deps.allSensorsReadyCache = &allReady;
  deps.undersamplingFlags = &undersampling;
  deps.topology = topology;
  deps.deliveredSessionLitersCache = &out.delivered;
  deps.deliveredFlowLpmCache = &out.deliveredFlow;
  deps.unknownBranchesCache = &out.unknownBranches;

  plc::SensorStateEngine engine(deps);
  engine.update(1.0f);
  return out;
}

/** Deliberately awkward values: no two equal, none a power of two, all plausible litre figures. */
const float kSeeds[plc::kNumSensors] = {12.5f,   3140.75f, 0.125f,  99999.5f,
                                        7.0625f, 1.0f,     420.25f, 65535.75f};
const bool kAllInUse[plc::kNumSensors] = {true, true, true, true, true, true, true, true};
const bool kAllCalibrated[plc::kNumSensors] = {true, true, true, true, true, true, true, true};

void deliveredReducesToGrossTests() {
  std::printf("\n[T2 — R2.2: with every parent at 0 the delivered total IS today's total]\n");

  const DeliveredRun noTopology = runDelivered(nullptr, kAllInUse, kAllCalibrated, kSeeds, nullptr);
  check(noTopology.delivered == noTopology.gross,
        "a null topology delivers exactly the gross double, bit for bit");
  // With no pulses both flows are 0.0, so comparing them there proves nothing — review was right that
  // the assertion could not fail. The flow half of R2.2 is asserted below on METERED flow instead.
  const uint32_t reductionPulses[plc::kNumSensors] = {60, 30, 15, 0, 0, 0, 0, 0};
  const float noSeedsForFlow[plc::kNumSensors] = {0, 0, 0, 0, 0, 0, 0, 0};
  const DeliveredRun metered =
      runDelivered(nullptr, kAllInUse, kAllCalibrated, noSeedsForFlow, reductionPulses);
  check(metered.grossFlow == 105.0, "three metering channels give a gross flow of 60 + 30 + 15 L/min");
  check(metered.deliveredFlow == metered.grossFlow,
        "and with every parent at 0 the delivered FLOW is that same double — R2.2's other half, on a "
        "value that is not zero");
  check(noTopology.unknownBranches == 0, "with nothing unknown");

  plc::SensorTopology allRoots;  // default-constructed: every parent 0
  const DeliveredRun parallel = runDelivered(&allRoots, kAllInUse, kAllCalibrated, kSeeds, nullptr);
  check(parallel.delivered == parallel.gross,
        "and an explicit all-roots topology delivers the same double — R2.2 is bit-identity, not "
        "approximate equality");
  check(parallel.delivered == noTopology.delivered,
        "so a device that has never been told about topology and one told it is parallel agree exactly");

  // WHY THERE IS NO ORDER-PERMUTATION ASSERTION HERE, though R2.2's wording invites one — and the
  // reason is BOUNDED, because the unbounded version of it is false. The addends are `float` (24-bit
  // mantissa) and the accumulator is `double` (53-bit), so eight of them sum exactly WHILE THEIR
  // MAGNITUDES STAY WITHIN ABOUT SEVEN DECADES: ascending versus descending summation over 200,000
  // random tuples disagrees 0 times across 1e-2..1e5 L, 180 times across 1e-2..1e7, and 13,590 times
  // across 1e-2..1e9. An order assertion would therefore pass against almost any implementation at
  // metering magnitudes while being genuinely falsifiable only on volumes no channel reaches — which
  // makes it a test that looks strong and is not.
  //
  // For DOUBLE addends the same sweep disagrees 51 % of the time, so if the lifetime aggregate is ever
  // netted (§7 Q5, open) that is where an order assertion earns its place.
}

void deliveredExcludesDownstreamTests() {
  std::printf("\n[T2 — R2.1: a downstream channel's water is counted by its root, once]\n");

  plc::SensorTopology chain;
  std::uint8_t parents[plc::SensorTopology::kChannels] = {};
  parents[1] = 1;  // channel 1 is fed by channel 0
  parents[2] = 2;  // channel 2 is fed by channel 1
  check(chain.apply(parents).ok(), "a three-channel cascade applies");

  const DeliveredRun cascade = runDelivered(&chain, kAllInUse, kAllCalibrated, kSeeds, nullptr);

  // The ORACLE, recomputed here in ascending index order over the roots only. What IT catches is a
  // wrong index set that is visible with everything in service and calibrated: summing
  // `cumulativeLiters` instead of `sessionLiters`, or a mis-built in-service mask. The other mutations
  // are caught elsewhere and the attribution is worth getting right — `configIsValid` in the predicate
  // fails the uncalibrated-child case in `unknownBranchTests`, the stored-parent-instead-of-effective
  // mutation fails `deliveredFollowsServiceStateTests`, and dropping the NaN rule fails
  // `unknownBranchTests` — because a comment that credits the wrong assertion is how a test's real
  // coverage gets over-estimated.
  //
  // WHAT IT DOES NOT CATCH, stated because the first version of this comment claimed it did:
  // `delivered = gross - downstream` passes every assertion in this file. Measured over four million
  // plausible tuples the two forms are bit-identical; they diverge by one ulp only when one channel's
  // volume sits ~9 decades below another's. The reason to sum the roots is structural — see the
  // comment at the accumulation site — and no test at realistic magnitudes can enforce it.
  double expected = 0.0;
  for (std::size_t i = 0; i < plc::kNumSensors; ++i) {
    if (i == 1 || i == 2) continue;  // downstream of channel 0
    expected += kSeeds[i];
  }
  check(cascade.delivered == expected,
        "the delivered total is exactly the sum of the ROOTS, to the last bit");
  check(cascade.delivered != cascade.gross,
        "and it differs from the gross total, which still double-counts the cascade");
  check(cascade.gross == runDelivered(nullptr, kAllInUse, kAllCalibrated, kSeeds, nullptr).gross,
        "while the GROSS total is unchanged by the topology — R2.3, and what keeps the red LED's "
        "baseline and the blue LED's liveness honest");
}

void deliveredFollowsServiceStateTests() {
  std::printf("\n[T2 — R1.4: an out-of-service mid-chain meter does not promote its child]\n");

  plc::SensorTopology chain;
  std::uint8_t parents[plc::SensorTopology::kChannels] = {};
  parents[1] = 1;
  parents[2] = 2;
  check(chain.apply(parents).ok(), "the cascade applies");

  bool withoutMiddle[plc::kNumSensors] = {true, true, true, true, true, true, true, true};
  withoutMiddle[1] = false;  // the mid-chain meter is taken out of service
  const DeliveredRun run = runDelivered(&chain, withoutMiddle, kAllCalibrated, kSeeds, nullptr);

  double expected = 0.0;
  for (std::size_t i = 0; i < plc::kNumSensors; ++i) {
    if (i == 1 || i == 2) continue;  // 1 is out of service; 2 re-parents to 0 and stays downstream
    expected += kSeeds[i];
  }
  check(run.delivered == expected,
        "channel 2 re-parents to its grandparent and stays downstream — the total does not GROW when a "
        "meter is switched off");

  bool headGone[plc::kNumSensors] = {true, true, true, true, true, true, true, true};
  headGone[0] = false;
  headGone[1] = false;
  const DeliveredRun promoted = runDelivered(&chain, headGone, kAllCalibrated, kSeeds, nullptr);
  double expectedPromoted = kSeeds[2];  // channel 2's walk now reaches root: it IS the delivery point
  for (std::size_t i = 3; i < plc::kNumSensors; ++i) expectedPromoted += kSeeds[i];
  check(promoted.delivered == expectedPromoted,
        "but with every ancestor out of service it becomes an effective root, so its water is counted "
        "rather than lost");
}

void deliveredFlowTests() {
  std::printf("\n[T2 — the same rule for FLOW, metered rather than seeded]\n");

  plc::SensorTopology chain;
  std::uint8_t parents[plc::SensorTopology::kChannels] = {};
  parents[1] = 1;
  check(chain.apply(parents).ok(), "a two-channel cascade applies");

  uint32_t pulses[plc::kNumSensors] = {60, 30, 0, 0, 0, 0, 0, 0};
  const float noSeeds[plc::kNumSensors] = {0, 0, 0, 0, 0, 0, 0, 0};
  const DeliveredRun run = runDelivered(&chain, kAllInUse, kAllCalibrated, noSeeds, pulses);

  check(run.grossFlow == 90.0, "the gross flow adds both meters: 60 + 30 L/min");
  check(run.deliveredFlow == 60.0,
        "the delivered flow is the ROOT's 60 alone — the child's 30 already passed through it");
  check(run.grossFlow != run.deliveredFlow,
        "and liveness reading the gross value still sees water (R2.4): a root at 0 with a flowing child "
        "must not report the device as idle");
}

void unknownBranchTests() {
  std::printf("\n[T2 — R2.5: a total the device cannot support says so, and does not guess]\n");

  plc::SensorTopology chain;
  std::uint8_t parents[plc::SensorTopology::kChannels] = {};
  parents[1] = 1;  // channel 1 is downstream of channel 0
  check(chain.apply(parents).ok(), "a two-channel cascade applies");

  bool rootUncalibrated[plc::kNumSensors] = {true, true, true, true, true, true, true, true};
  rootUncalibrated[0] = false;  // the ROOT has no valid calibration — a meter swap does this
  const DeliveredRun unknown = runDelivered(&chain, kAllInUse, rootUncalibrated, kSeeds, nullptr);

  check(unknown.unknownBranches == 0x0001,
        "an in-service root with no valid calibration marks ITS branch unknown, by bit");
  check(unknown.delivered != unknown.delivered,
        "and the delivered volume is NaN — a device refuses to state a total it cannot support rather "
        "than state one that is wrong by a whole branch");
  check(unknown.deliveredFlow != unknown.deliveredFlow, "the delivered flow too");
  check(unknown.gross == unknown.gross,
        "while the GROSS total stays a NUMBER — R2.3, and now a hard constraint: one NaN through "
        "LedController::update poisons its litre baseline for the rest of the boot");

  bool childUncalibrated[plc::kNumSensors] = {true, true, true, true, true, true, true, true};
  childUncalibrated[1] = false;  // a DOWNSTREAM channel is uncalibrated
  const DeliveredRun childCase = runDelivered(&chain, kAllInUse, childUncalibrated, kSeeds, nullptr);
  check(childCase.unknownBranches == 0,
        "an uncalibrated channel that is NOT a delivery point marks nothing unknown — its water is its "
        "root's business and the root can still state the branch");
  check(childCase.delivered == childCase.delivered, "so the total is still a number");

  // NOTHING IN SERVICE, and every channel UNCALIBRATED — which is the fixture this assertion needs.
  // It used to pass `kAllCalibrated`, so no channel could have been unknown whatever the code did and
  // the check passed for the wrong reason. Review called it correctly: the assertion's own text claims
  // to test that the root must be IN SERVICE, and only this fixture makes that claim testable.
  bool nothingInUse[plc::kNumSensors] = {false, false, false, false, false, false, false, false};
  const bool noneCalibrated[plc::kNumSensors] = {false, false, false, false,
                                                 false, false, false, false};
  const DeliveredRun idle = runDelivered(&chain, nothingInUse, noneCalibrated, kSeeds, nullptr);
  check(idle.delivered == 0.0 && idle.unknownBranches == 0,
        "an OUT-OF-SERVICE uncalibrated channel marks nothing unknown and delivers 0.0 — not NaN, "
        "because there is no branch anybody is asking about");

  // FINDING 6 — nothing pinned the bitmap to any bit but 0, so an index-to-bit mis-mapping survived.
  // Channel 2 becomes an effective root by skipping an out-of-service ancestor, so this fixture pins
  // the mapping AND R1.4's interaction with R2.5 at the same time.
  plc::SensorTopology deep;
  std::uint8_t deepParents[plc::SensorTopology::kChannels] = {};
  deepParents[2] = 1;  // channel 2 is fed by channel 0
  check(deep.apply(deepParents).ok(), "a topology where channel 2 hangs off channel 0 applies");
  bool headOut[plc::kNumSensors] = {true, true, true, true, true, true, true, true};
  headOut[0] = false;  // its only ancestor is out of service, so channel 2 IS the delivery point
  bool twoUncalibrated[plc::kNumSensors] = {true, true, true, true, true, true, true, true};
  twoUncalibrated[2] = false;
  const DeliveredRun bitTwo = runDelivered(&deep, headOut, twoUncalibrated, kSeeds, nullptr);
  check(bitTwo.unknownBranches == 0x0004,
        "the unknown bit is the CHANNEL's own bit — 2 sets 0x0004, not its parent's bit and not bit 0");
}

}  // namespace

int main() {
  std::printf("SensorStateEngine — readiness aggregation\n\n");
  readinessTests();
  disconnectedSensorTests();
  bootRestoreTests();
  deliveredReducesToGrossTests();
  deliveredExcludesDownstreamTests();
  deliveredFollowsServiceStateTests();
  deliveredFlowTests();
  unknownBranchTests();
  std::printf("\n%s (%d checks, %d failures)\n", failures == 0 ? "ALL PASSED" : "FAILURES", checks,
              failures);
  return failures == 0 ? 0 : 1;
}
