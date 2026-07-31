#pragma once

#include <cstdint>

namespace plc {

/**
 * RS485 link limits, split out of link_settings.h so the Arduino-free settings table
 * can reference them without dragging in RegisterBank.
 */
struct LinkLimits {
  static constexpr uint32_t kBaudRates[] = {1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200};
  static constexpr uint8_t kBaudCount = 8;
  /** 1-247: address 0 is broadcast and 248-255 are reserved by the Modbus spec. */
  static constexpr uint8_t kMinSlaveId = 1;
  static constexpr uint8_t kMaxSlaveId = 247;
};

}  // namespace plc
