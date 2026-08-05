#pragma once

#include <cstddef>
#include <cstdint>

/**
 * Rising-edge detection for the eight digital inputs.
 *
 * Extracted from firmware.cpp's polling loop, which is the reason this file exists: the loop that
 * produces every number this product sells had **no host test at all**. It was the only subsystem
 * without one — firmware.cpp appears in no link set in test/host/run.sh — while the UI, the register
 * map, the pack loader and the network modules carry over a thousand checks between them. The
 * measurement core was the least covered code in the project.
 *
 * Deliberately Arduino-free and free of any I²C dependency: the caller samples the bus and passes the
 * levels in. That inversion is what makes the edge semantics testable without hardware, and it is the
 * same policy/transport split used by NetSettings, WifiManager and MqttPublisher.
 */
namespace plc {

/**
 * Channels whose level rose between two samples, restricted to `enabledMask`.
 *
 * `current & ~previous` is the rising edge; the mask then drops channels no sensor is using.
 *
 * THE MASK APPLIES TO COUNTING, NOT TO EDGE TRACKING, and the distinction is a correctness trap
 * rather than a style choice. The caller must keep feeding `previous` from the FULL bitmap including
 * disabled channels. If a disabled channel's level is not tracked while it is off, then enabling it
 * while its input happens to sit high manufactures a rising edge that never physically occurred — a
 * phantom litre, on the first sample after a configuration change. Tracking everything and masking
 * only the count makes that unreachable.
 */
constexpr uint8_t risingEdges(uint8_t previous, uint8_t current, uint8_t enabledMask) {
  return static_cast<uint8_t>(current & static_cast<uint8_t>(~previous) & enabledMask);
}

/**
 * Visits each set bit of `edges`, lowest channel first.
 *
 * This is the branch-free form of the per-channel `if` loop it replaced, and it is worth being honest
 * about what it does and does not buy.
 *
 * It does NOT make sampling faster in any way that can be measured. One bitmap sample costs two I²C
 * transactions — about 250 µs at 400 kHz including driver overhead — and the loop it replaces was
 * eight integer comparisons, well under a microsecond on a 240 MHz core. The counting was already
 * about 0.1 % of the sample period, so removing all of it could raise the sample rate by 0.1 % at
 * absolute best. Anyone reading this expecting a throughput win should look at the transaction count
 * instead; that is where the factor of four came from.
 *
 * What it does buy is worth having anyway:
 *
 * 1. **Uniform cost.** The old loop branched once per channel on `inUse`, so its duration depended on
 *    how many sensors were configured. This one costs a fixed few instructions plus one iteration per
 *    edge actually seen — and the common case is zero edges. Sample-to-sample spacing becomes more
 *    even, which matters for a counter whose accuracy depends on not missing a HIGH phase.
 * 2. **The intent is one expression.** `current & ~previous & enabled` is hard to get subtly wrong.
 *    A nested `if (inUse) if (cur & bit) if (!(last & bit))` is exactly the shape that acquires an
 *    off-by-one or an inverted test during a later edit.
 *
 * `__builtin_ctz` is undefined for zero, which is why the loop is `while (edges)` and not
 * `do { } while`. `edges &= edges - 1` clears the lowest set bit — the standard idiom, and the reason
 * the iteration count is the population count rather than the word width.
 */
template <typename Fn>
constexpr void forEachRisingChannel(uint8_t edges, Fn&& visit) {
  while (edges != 0) {
    const auto channel = static_cast<std::size_t>(__builtin_ctz(static_cast<unsigned>(edges)));
    visit(channel);
    edges &= static_cast<uint8_t>(edges - 1);
  }
}

/**
 * Builds the enabled-channel mask by ASKING, one channel at a time.
 *
 * This replaced `enabledMaskFromBitmap(connectedSensorsBitmap, ...)`, and the reason is the whole
 * point of the change rather than a style preference.
 *
 * There were TWO representations of "is this sensor connected":
 *   1. `connectedSensorsBitmap`, which the core-0 polling loop used to build its mask, and
 *   2. `sensors[i].inUse`, which the core-1 engine used to decide whether to convert pulses.
 *
 * They were written together at one site, so they could not actually diverge — but nothing enforced
 * it, and the consequence of divergence was severe and silent. Suppose the bitmap said enabled while
 * `inUse` was false: the loop would count edges into `pulseCount`, and the engine would never clear
 * it, because `sensor.pulseCount = 0` sits INSIDE `if (sensor.inUse)`. The backlog would grow for as
 * long as the disagreement lasted and then convert in a single interval — one enormous volume,
 * indistinguishable from real flow, written into the persisted `cumulativeLiters`.
 *
 * Passing a predicate lets the caller read the SAME flag the engine reads, so the two cannot disagree
 * by construction. That is the third time in this project a bug class has been removed by collapsing
 * two representations of one fact into one — the others were the MQTT flags register (live versus
 * staged) and the duplicated `out_buffer_size`.
 *
 * A predicate rather than a `SensorData*` parameter so this header stays free of
 * `modbus/sensor_types.h`, and so a host test can drive it with a lambda.
 */
template <typename IsEnabled>
constexpr uint8_t enabledMaskFrom(std::size_t channelCount, IsEnabled&& isEnabled) {
  uint8_t mask = 0;
  const std::size_t limit = channelCount < 8 ? channelCount : 8;
  for (std::size_t i = 0; i < limit; ++i) {
    if (isEnabled(i)) {
      mask = static_cast<uint8_t>(mask | (1u << i));
    }
  }
  return mask;
}

}  // namespace plc
