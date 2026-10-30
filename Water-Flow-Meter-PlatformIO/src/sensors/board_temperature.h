#pragma once

#include <cstdint>
#include <limits>

namespace plc {

/**
 * The board temperature — what the LM75B on the StampPLC's internal bus says, and when to believe it.
 * `DF29`.
 *
 * `MqttDiagnosticsTelemetry::boardTemperatureC` was declared, published as `tempC` and announced to Home
 * Assistant as a *Board temperature* entity, and nothing ever assigned it. So every diagnostics message
 * said `"tempC":0.0` and Home Assistant drew a flat line at 0 °C: a plausible reading, which is worse
 * than an empty one. The same shape as `DF23` and `DF24`, a published field nobody assigns, so the fix
 * follows `DF24`'s: the decisions live here, Arduino-free and tested by
 * `test/host/board_temperature_test.cpp`, and `firmware.cpp` makes one call that the same test reads
 * the file to prove is there.
 *
 * ── Why not `M5StamPLC.getTemp()` ──────────────────────────────────────────────────────
 *
 * The library already reads this chip, and it is not used on purpose. `LM75B_Class::read16`
 * (`M5StamPLC/src/utils/lm75b/lm75b.cpp`) calls `readRegister(reg, buff, 2)`, ignores the `bool` it
 * returns, and decodes `buff` regardless. On a failed transaction `buff` is an uninitialised stack
 * array, so the library hands back a confident temperature made of whatever was on the stack. This
 * file reads the same two bytes through `m5::In_I2C.readRegister`, whose `bool` is the only thing that
 * can tell a failed read from a cold board.
 *
 * ── What the plausibility check can and cannot see ─────────────────────────────────────
 *
 * The LM75B's operating range is −55 °C to +125 °C. That is the clamp `LM75B_Class::writeAlertTempHelper`
 * applies, and the 11-bit register can also express −128 to +127.875 °C, so a code outside the
 * operating range is one a working chip does not produce, and it is refused. But the two codes a
 * broken bus is most likely to produce are INSIDE the range: all-zero bytes decode to 0.0 °C and
 * all-one bytes to −0.125 °C, and a garage in winter can genuinely be at either. So the range check
 * catches nonsense codes only. The transaction's `bool` is the real guard, and nothing here can stand
 * in for it.
 */

// ── The chip, as the vendored library describes it ─────────────────────────────────────

/** `LM75B_Class`'s default constructor, and `docs/hardware docs/StampPLC specifications.md`'s bus map. */
inline constexpr uint8_t kLm75bAddress = 0x48;
/** `LM75B_Class::REG_TEMP`. Two bytes, most significant first. */
inline constexpr uint8_t kLm75bTemperatureRegister = 0x00;
/**
 * `LM75B_Class`'s default constructor. The same frequency the sampler uses for the AW9523 on this bus
 * (`kAw9523FreqHz` in `firmware.cpp`), which is also what the per-read cost below assumes.
 */
inline constexpr uint32_t kLm75bBusFrequencyHz = 400000;
/** One step of the 11-bit reading, `LM75B_Class::temp()`'s `value * 0.125`. */
inline constexpr float kLm75bCelsiusPerStep = 0.125f;
/** The operating range, `LM75B_Class::writeAlertTempHelper`'s clamp. */
inline constexpr float kLm75bMinimumCelsius = -55.0f;
inline constexpr float kLm75bMaximumCelsius = 125.0f;
/**
 * `LM75B_Class::POWER_NORMAL`: "samples every 100ms". The first read waits at least this long after
 * boot, because nothing this project has read says what the temperature register holds before the
 * chip's first conversion — and a register that reads zero would publish 0.0 °C, which is the exact
 * value `DF29` was. On a software restart the chip has kept its power and the wait costs nothing.
 */
inline constexpr uint32_t kLm75bConversionPeriodMs = 100;

/**
 * The temperature register's two bytes, in °C.
 *
 * The reading is 11-bit two's complement in the top eleven bits of the big-endian 16-bit value, so the
 * bottom five bits are not part of it and are ignored. The sign is extended by hand rather than by
 * shifting a signed `int16_t` right, which is implementation-defined for a negative value before C++20.
 * The host test compares this against `LM75B_Class::temp()`'s own arithmetic for all 65,536 inputs.
 */
inline float decodeLm75bCelsius(uint8_t msb, uint8_t lsb) {
  const uint16_t word = static_cast<uint16_t>((static_cast<uint16_t>(msb) << 8) | lsb);
  const uint16_t steps = static_cast<uint16_t>(word >> 5);  // 0 … 2047, the 11-bit code
  const int32_t signedSteps = (steps & 0x400u) ? static_cast<int32_t>(steps) - 2048
                                               : static_cast<int32_t>(steps);
  return static_cast<float>(signedSteps) * kLm75bCelsiusPerStep;
}

/** Inside the chip's operating range. Catches nonsense codes only; see the header comment. */
inline bool lm75bCelsiusPlausible(float celsius) {
  // Written so that NaN fails: every comparison with NaN is false.
  return celsius >= kLm75bMinimumCelsius && celsius <= kLm75bMaximumCelsius;
}

/** What one call to `sampleBoardTemperature` did. Returned so the caller can log a failure. */
enum class BoardTemperatureRead : uint8_t {
  NotDue,       ///< No bus traffic this pass.
  Good,         ///< The bus answered and the code is in the operating range.
  BusFailed,    ///< `readRegister` returned false: a NACK, a timeout or a lost arbitration.
  Implausible,  ///< The bus answered with a code outside the operating range.
};

/**
 * When to read, and how long a reading may be published.
 *
 * ── THE CADENCE: ONE READ A MINUTE, AND ONLY WHILE SOMETHING IS LISTENING ──────────────
 *
 * The chip shares SCL 15 / SDA 13 and the driver's mutex with the AW9523 the pulse sampler polls on
 * core 0 (`firmware.cpp`, "Reading the eight digital inputs"), so every read is time the sampler waits
 * for the bus. R2.1.6 (`WiFi_MQTT_Connectivity.md` §2.1.2) says a device on this bus is "polled only as
 * often as its purpose requires", and this reading has one purpose: Home Assistant's *Board
 * temperature* entity. So:
 *
 * - **Every 60 s.** The same figure as `MqttPublisher::kHeartbeatMs`, which is already the bound this
 *   firmware sets on how stale a subscriber's view may be, so a reading of that age is inside what the
 *   device promises for every other value. Reading at the default 10 s publish period instead would
 *   cost the sampler the bus six times as often, for a figure nothing on the device acts on. A good
 *   read is the `readRegister8` the sampler issues plus one data byte: about 49 bit periods, about
 *   120 µs on the wire at 400 kHz. That is arithmetic from the frame and was not measured, and the
 *   driver's own overhead comes on top of it.
 * - **Only while MQTT is connected**, which the caller enforces by making the call inside its
 *   connected block. With no subscriber there is no purpose, and a device with MQTT off keeps the
 *   sampler's rate exactly what it was before this reading existed, which is the rate `G1` and `DF23`
 *   want to measure.
 * - **A failed read is retried at the same cadence, not sooner.** The failure that costs the most is a
 *   stalled bus, which the driver waits out up to its stall limit (`i2c_stall_limit_us`, M5GFX
 *   `platforms/esp32/common.cpp`) instead of a transaction's length. Retrying faster would multiply
 *   exactly the cost the cadence exists to bound, for a reading nothing on the device acts on.
 *
 * ── HOW LONG A READING MAY BE PUBLISHED: UNTIL THE NEXT READ IS DUE, AND NO LONGER ───────
 *
 * `celsius()` is non-finite (which the publisher emits as JSON `null`) in four cases:
 *
 * 1. no read has been attempted yet;
 * 2. the last read failed on the bus. The previous good value is NOT kept as a fallback, because a
 *    reading from before the failure, published as though it were current, is exactly the kind of
 *    plausible number `DF29` was;
 * 3. the last read was implausible, for the same reason;
 * 4. the last good read is `kReadIntervalMs` old or older. In normal operation that cannot happen,
 *    because the caller samples before it asks, in the same pass, so the read due at that moment has
 *    either just replaced the value or just failed. It is the backstop for a caller that stopped
 *    sampling: across an MQTT outage nothing is read, and no snapshot assembled on the reconnect may
 *    carry a value a full interval old. A reading from before a SHORTER outage is still inside its
 *    interval and goes into the snapshot exactly as it would between two reads; that is the sampling
 *    interval, not a stale value, and the rule below is what bounds it.
 *
 * So no snapshot is ever ASSEMBLED with a reading a full read interval old. Between reads it carries the
 * last reading, which is the sampling interval and not staleness.
 *
 * WHAT THIS DOES NOT BOUND: a message already queued. `MqttPublisher` keeps its queue across a
 * disconnect: `pump()` does nothing while the link is down, and neither `onDisconnected()` nor
 * `onConnected()` clears the queue. So a diagnostics payload queued just before a drop is the first one
 * sent after the reconnect, carrying the reading from before the outage however long the outage was,
 * and the next diagnostics publish replaces it. That next one is the reconnect's own full set. Until
 * `DF30` was fixed, a discovery burst that filled the queue got that full set refused and the
 * replacement waited a publish period; now the set is held only while the connect's discovery is queued
 * — a bounded number of passes, `HaDiscoveryJob::kConnectHoldPassLimit` at most — and is never refused
 * (net/ha_discovery_job.h). Both behaviours were replayed against the real publisher; the `DF29` and
 * `DF30` entries in `docs/active_work/open_decisions.md` have the runs. This is the queue every
 * telemetry field shares, not a rule about this reading, and changing it is not `DF29`.
 *
 * `nowMs` is `millis()`. Every age is `now - then`, which survives the ~49-day rollover.
 */
class BoardTemperature {
 public:
  /** R2.1.6's cadence; see above. Equal to `MqttPublisher::kHeartbeatMs` by choice, not by derivation. */
  static constexpr uint32_t kReadIntervalMs = 60000;
  /** How long a good reading is published. One interval: see case 4 above. */
  static constexpr uint32_t kMaxAgeMs = kReadIntervalMs;

  /** Whether a read should happen on this pass. */
  bool due(uint32_t nowMs) const {
    if (!attempted_) {
      return nowMs >= kLm75bConversionPeriodMs;
    }
    return nowMs - lastAttemptMs_ >= kReadIntervalMs;
  }

  /**
   * Records one bus transaction. `msb` and `lsb` are not looked at when `busOk` is false — the
   * library's defect was precisely looking at them.
   */
  BoardTemperatureRead noteRead(uint32_t nowMs, bool busOk, uint8_t msb, uint8_t lsb) {
    attempted_ = true;
    lastAttemptMs_ = nowMs;
    ++attempts_;
    valid_ = false;
    if (!busOk) {
      return BoardTemperatureRead::BusFailed;
    }
    const float decoded = decodeLm75bCelsius(msb, lsb);
    if (!lm75bCelsiusPlausible(decoded)) {
      return BoardTemperatureRead::Implausible;
    }
    valid_ = true;
    celsius_ = decoded;
    lastGoodMs_ = nowMs;
    return BoardTemperatureRead::Good;
  }

  /** The reading to publish now, or quiet NaN in the four cases above. */
  float celsius(uint32_t nowMs) const {
    if (!valid_ || nowMs - lastGoodMs_ >= kMaxAgeMs) {
      return std::numeric_limits<float>::quiet_NaN();
    }
    return celsius_;
  }

  /** Bus transactions so far, so a test can count what "rarely" means. */
  uint32_t attempts() const { return attempts_; }

 private:
  bool attempted_ = false;
  uint32_t lastAttemptMs_ = 0;
  uint32_t attempts_ = 0;
  bool valid_ = false;
  float celsius_ = 0.0f;
  uint32_t lastGoodMs_ = 0;
};

/**
 * Reads the chip if a read is due, and records the outcome — the one call `firmware.cpp` makes.
 *
 * `readTwoBytes(uint8_t* out)` performs the transaction and returns the bus's verdict; on the device it
 * is `m5::In_I2C.readRegister(kLm75bAddress, kLm75bTemperatureRegister, out, 2, kLm75bBusFrequencyHz)`.
 * A template so the host test can drive it with a fake bus, the way `drivePortal` takes a fake server.
 * The buffer is zeroed first, so even a reader that wrote nothing cannot leak stack bytes — though
 * `noteRead` already ignores them on a failure.
 */
template <typename ReadTwoBytes>
BoardTemperatureRead sampleBoardTemperature(BoardTemperature& temperature, uint32_t nowMs,
                                            ReadTwoBytes&& readTwoBytes) {
  if (!temperature.due(nowMs)) {
    return BoardTemperatureRead::NotDue;
  }
  uint8_t bytes[2] = {0, 0};
  const bool busOk = readTwoBytes(bytes);
  return temperature.noteRead(nowMs, busOk, bytes[0], bytes[1]);
}

}  // namespace plc
