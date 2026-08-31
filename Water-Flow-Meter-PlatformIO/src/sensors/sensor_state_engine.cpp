#include "sensors/sensor_state_engine.h"

#include <limits>

namespace plc {

SensorStateEngine::SensorStateEngine(const Dependencies& deps) : deps_(deps) {}

void SensorStateEngine::update(float elapsedSeconds) {
  if (!deps_.sensors || !deps_.configs || elapsedSeconds <= 0.0f) {
    return;
  }

  double totalSessionLiters = 0.0;
  double aggregateFlowLpm = 0.0;
  bool allReady = true;
  std::size_t activeSensors = 0;

  // The DELIVERED pair — R2.1 — accumulated in the same loop, in the same ascending order, out of the
  // same two float fields. That is what earns R2.2's bit-identity rather than hoping for it: with every
  // parent at 0 every in-service channel is an effective root, so the index set and the accumulation
  // order are the gross pair's, and the IEEE-754 result is the same double by construction.
  //
  // NOT computed as `gross - downstream`, and the reason is STRUCTURAL rather than numerical — the
  // numerical claim is worth getting right because the obvious version of it is false. Measured: over
  // four million plausible eight-channel tuples the two forms are bit-identical, and a difference
  // appears only when one channel's volume sits about nine decades below another's (0.0108 L beside
  // 8,963,428 L), where it is a single ulp. So no test on realistic volumes can tell them apart, and
  // anybody who "optimises" this into a subtraction will find the suite still green.
  //
  // Summing the roots is still the right form: it computes what R2.1 DEFINES instead of deriving it
  // from an accumulator that R2.3 deliberately leaves for other consumers, so a later change to the
  // gross pair cannot move the delivered figure behind their backs. It also carries R2.5 without a
  // special case — an unknown branch is decided per delivery point, which a subtraction has no place
  // to express. And the order-independence above does NOT hold for `cumulativeLiters`, which is a
  // double: if the lifetime aggregate is ever netted (§7 Q5, open), the same subtraction would differ
  // from the sum in 99.5 % of cases.
  double deliveredSessionLiters = 0.0;
  double deliveredFlowLpm = 0.0;
  uint16_t unknownBranches = 0;

  // Which channels are in service, as the bitmap `SensorTopology::effectiveParent` takes (R1.4). Built
  // in its own pass because the effective-root question for channel 0 depends on channel 7's service
  // state, so the answer cannot be assembled as the main loop walks. Eight reads of a bool, and NO
  // destructive reads: `pulseCount` is consumed in the main loop and must be touched exactly once.
  uint16_t inServiceMask = 0;
  for (std::size_t i = 0; i < deps_.sensorCount && i < 16; ++i) {
    if (deps_.sensors[i].inUse) {
      inServiceMask = static_cast<uint16_t>(inServiceMask | (1u << i));
    }
  }

  for (std::size_t i = 0; i < deps_.sensorCount; ++i) {
    auto& sensor = deps_.sensors[i];
    const auto& config = deps_.configs[i];

    if (sensor.inUse) {
      ++activeSensors;
      const uint32_t pulses = sensor.pulseCount;
      sensor.pulseCount = 0;

      // One predicate, computed from the configuration in hand. This used to read a cached bit AND
      // re-check the multiplier — belt and braces around a value it could not trust, because boot cleared
      // the cache and nothing refilled it. configIsValid already requires a non-zero multiplier, so the
      // second clause was redundant as well as insufficient.
      if (configIsValid(config)) {
        const float frequency = static_cast<float>(pulses) / elapsedSeconds;
        /**
         * Two calibration forms, inverted from how a datasheet states them.
         *
         * Formula: the sheet gives `F = m*Q + a`, so flow is `(F - a) / m`.
         * Pulses per litre: the sheet gives K pulses per litre, so K*Q/60 pulses per second, so flow
         * is `F * 60 / K`.
         *
         * The second is not the first with `a = 0`. Expressing K = 450 as a multiplier needs 7.5, and
         * `f_multiplier` is an integer — 7 or 8, a 6% error on every reading. That is why the form has
         * its own field rather than being folded into the formula.
         */
        float flowRateLpm;
        if (config.calibration == CalibrationType::PulsesPerLitre) {
          flowRateLpm = frequency * 60.0f / static_cast<float>(config.pulses_per_litre);
        } else {
          flowRateLpm = (frequency - config.adjust) / config.f_multiplier;
        }
        if (flowRateLpm < 0.0f) {
          flowRateLpm = 0.0f;
        }
        if (flowRateLpm > config.q_max) {
          flowRateLpm = config.q_max;
        }

        // Stored as computed. The clamp above is already in L/min against q_max, so this is where
        // §2a removes a conversion rather than adding one.
        sensor.instantFlow_L_min = flowRateLpm;
        if (sensor.instantFlow_L_min > sensor.maxFlowSinceReset) {
          sensor.maxFlowSinceReset = sensor.instantFlow_L_min;
        }

        // The one division that remains, and it is unavoidable: volume is a rate times a TIME, and
        // the interval is in seconds while the rate is per minute.
        const double litersInterval = static_cast<double>(flowRateLpm) * elapsedSeconds / 60.0;
        sensor.sessionLiters += litersInterval;
        sensor.cumulativeLiters += litersInterval;
      } else {
        sensor.instantFlow_L_min = 0.0f;
      }

      totalSessionLiters += sensor.sessionLiters;
      aggregateFlowLpm += sensor.instantFlow_L_min;

      // R2.1. A null topology means every channel is a root, which is the parallel installation — so
      // this reduces to the two lines above, addend for addend.
      //
      // The predicate is `isEffectiveRoot` and NOTHING ELSE. Adding `configIsValid` here would look
      // tidy and would silently narrow R2.2 to the all-calibrated case: the gross sum above includes an
      // in-service uncalibrated channel's FROZEN volume (§5.4), so the delivered sum has to include it
      // too or the two disagree on a state that occurs every time a meter is swapped. What that state
      // means for a ROOT is R2.5's business, below, and it is answered with NaN rather than by dropping
      // an addend.
      const bool isDeliveryPoint =
          deps_.topology == nullptr || deps_.topology->isEffectiveRoot(i, inServiceMask);
      if (isDeliveryPoint) {
        deliveredSessionLiters += sensor.sessionLiters;
        deliveredFlowLpm += sensor.instantFlow_L_min;
        if (!configIsValid(config) && i < 16) {
          // R2.5: this root's branch volume is not a number anybody can state. Recorded per branch
          // rather than as a flag, so a consumer can name the branch.
          unknownBranches = static_cast<uint16_t>(unknownBranches | (1u << i));
        }
      }

      if (!configIsValid(config)) {
        allReady = false;
      }
    } else {
      sensor.instantFlow_L_min = 0.0f;
      // A DISABLED channel must not clear allReady. RGB_LED_Behavior.md §3.2 defines green
      // as "solid ON when every ACTIVE sensor is ready" — active, not all eight.
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

  // R2.5. NaN rather than the smaller number the sum happens to hold: an uncalibrated root means one
  // branch whose volume nobody knows, and a device should refuse to state a total it cannot support
  // rather than state one that is wrong by a whole branch. A wrong number is harder to notice than a
  // missing one. NaN also PROPAGATES — a master that ignores the bitmap and sums it gets NaN rather
  // than a plausible figure.
  //
  // Applied to the DELIVERED pair only. The gross pair must stay finite (R2.3), and that is now a hard
  // constraint rather than a preference: `LedController::update` keeps a `lastTotalLiters_` baseline,
  // and one NaN through it poisons the red volume LED for the rest of the boot — fixing the calibration
  // would not clear it.
  if (unknownBranches != 0) {
    const double unknown = std::numeric_limits<double>::quiet_NaN();
    deliveredSessionLiters = unknown;
    deliveredFlowLpm = unknown;
  }

  if (deps_.deliveredSessionLitersCache) {
    *deps_.deliveredSessionLitersCache = deliveredSessionLiters;
  }
  if (deps_.deliveredFlowLpmCache) {
    *deps_.deliveredFlowLpmCache = deliveredFlowLpm;
  }
  if (deps_.unknownBranchesCache) {
    *deps_.unknownBranchesCache = unknownBranches;
  }

  if (deps_.totalSessionLitersCache) {
    *deps_.totalSessionLitersCache = totalSessionLiters;
  }
  if (deps_.aggregateFlowLpmCache) {
    *deps_.aggregateFlowLpmCache = aggregateFlowLpm;
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
