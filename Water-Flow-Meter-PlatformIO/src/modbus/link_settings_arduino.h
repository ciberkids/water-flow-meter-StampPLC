#pragma once

#include <HardwareSerial.h>

#include "modbus/link_settings.h"

namespace plc {

/**
 * Translates parity and stop bits into the Arduino `SERIAL_8N1`-style constant.
 *
 * Kept apart from link_settings.h so that header stays Arduino-free and its
 * validation, staging and rollback logic remains host-testable.
 */
inline uint32_t arduinoSerialConfig(const LinkSettings& settings) {
  if (settings.stopBits == 2) {
    switch (settings.parity) {
      case 1: return SERIAL_8E2;
      case 2: return SERIAL_8O2;
      default: return SERIAL_8N2;
    }
  }
  switch (settings.parity) {
    case 1: return SERIAL_8E1;
    case 2: return SERIAL_8O1;
    default: return SERIAL_8N1;
  }
}

}  // namespace plc
