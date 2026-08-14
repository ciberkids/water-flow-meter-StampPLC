#pragma once

#include <Wire.h>

#include <cstdint>

#include "M5StamPLC.h"
#include "time/device_clock.h"

namespace plc {

/**
 * Reads the RX8130CE's Voltage Low Flag — and it can only be called from ONE place in the program.
 *
 * VLF (register 0x1D, bit 1) is set by the chip when its oscillator has lost power, which is the only
 * evidence that the calendar registers hold drift rather than time. A drifted RX8130CE typically comes up
 * in the year 2000, which renders as a perfectly plausible date on the panel and would be published to
 * Modbus and MQTT with total confidence.
 *
 * ── WHY THE ORDERING IS LOAD-BEARING ─────────────────────────────────────────────────────
 *
 * `M5StamPLC.begin()` calls `_rtc_init()`, which calls `RX8130.clearIrqFlags()`, which writes 0 to the
 * whole flag register — VLF included — and the library exposes no reader for it. So this must run BEFORE
 * `M5StamPLC.begin()`, and there is exactly one moment in the device's life when the answer exists.
 * Call it afterwards and it returns a cleared flag, meaning a device with a dead clock reports a
 * trustworthy one.
 *
 * That is why this is a free function with a name that says "boot probe" rather than a method on
 * DeviceClock: a method invites being called whenever a caller wants an answer, and every call after the
 * first would be a lie. It also takes its own `Wire.begin()`, because the bus is not up yet at that point
 * — `begin()` is what normally brings it up.
 *
 * Returns TRUE — untrusted — on any I2C failure. A clock we could not interrogate is exactly as
 * untrustworthy as one that lost power, and the safe direction is to display nothing rather than a number
 * nobody verified.
 */
inline bool readRtcVoltageLowFlag() {
  constexpr uint8_t kRtcAddress = 0x32;
  constexpr uint8_t kFlagRegister = 0x1D;
  constexpr uint8_t kVoltageLowBit = 1 << 1;

  // The internal bus, from the library's own pin_config.h rather than repeated as literals here.
  Wire.begin(STAMPLC_PIN_I2C_INTER_SDA, STAMPLC_PIN_I2C_INTER_SCL);

  Wire.beginTransmission(kRtcAddress);
  Wire.write(kFlagRegister);
  if (Wire.endTransmission(false) != 0) {
    return true;
  }
  if (Wire.requestFrom(static_cast<uint8_t>(kRtcAddress), static_cast<uint8_t>(1)) != 1) {
    return true;
  }
  const uint8_t flags = static_cast<uint8_t>(Wire.read());
  return (flags & kVoltageLowBit) != 0;
}

/** The RTC's calendar as a Unix epoch, or 0 when it cannot be converted. Call AFTER `begin()`. */
inline uint32_t readRtcEpoch() {
  struct tm parts {};
  M5StamPLC.getRtcTime(&parts);
  // The arithmetic lives in device_clock.cpp so the host suite can test it; `struct tm` offsets are
  // converted here, at the one boundary that knows about them.
  return epochFromUtcCivil(parts.tm_year + 1900, parts.tm_mon + 1, parts.tm_mday, parts.tm_hour,
                           parts.tm_min, parts.tm_sec);
}

}  // namespace plc
