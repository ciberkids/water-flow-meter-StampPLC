#pragma once

// Host stub for the Arduino core, which on the host means one symbol: millis().
//
// It lives in its own header because more than one stub needs it and there may only be ONE definition.
// M5StamPLC.h used to define millis() itself, which was fine while the display harness was the only
// thing that wanted a clock. It stopped being fine when a test linked modbus_manager.cpp: that
// translation unit calls millis() four times and includes neither M5StamPLC.h nor anything that could
// reasonably provide it, because on the real device the symbol arrives transitively — the ESP32 core's
// Preferences.h and eModbus's ModbusMessage.h both pull in Arduino.h. Those two stubs now include this
// header for the same reason, and M5StamPLC.h delegates to it rather than declaring a second millis().
#include <cstdint>

namespace arduino_stub {

/**
 * The one simulated clock behind millis(), shared by every stub.
 *
 * Settable, and deliberately the SAME storage for all of them: a harness that advances the display's
 * clock while ModbusManager reads a different one would date a session reset to a time no other part of
 * the test agrees happened, which is precisely the class of confusion these tests exist to expose.
 */
inline uint32_t& clockMs() {
  static uint32_t value = 0;
  return value;
}

}  // namespace arduino_stub

inline uint32_t millis() { return arduino_stub::clockMs(); }
