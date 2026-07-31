#pragma once

// Host stub for the M5StamPLC board library.
//
// Only the surface the firmware actually touches: begin, three buttons, readPlcInput and
// setStatusLight. Verified by grepping for `M5StamPLC.` across src/.
//
// The buttons are TEST-SETTABLE, which is the whole trick behind the device harness. It
// means button_input.cpp needs no production change at all — its only hardware contact is
// readHardware(), and pointing that at a settable stub turns the real InteractionHandler
// into something a host test can drive press-by-press. No injectable-button-source
// abstraction, no second implementation of the gesture contract to keep in sync.
#include <cstdint>

namespace m5stamplc_stub {

/** One button's state, plus a record of what the firmware asked of the hardware. */
struct Button {
  bool pressed = false;
  bool isPressed() const { return pressed; }
};

/** Everything a test can observe or control. Global because the real library is too. */
struct Board {
  Button BtnA;
  Button BtnB;
  Button BtnC;

  bool begun = false;
  /** Digital inputs, read one channel at a time — the 1.2.0 API has no bulk read. */
  bool plcInput[8] = {};
  /** Last RGB written, so LED requirements can be asserted without an oscilloscope. */
  uint8_t r = 0, g = 0, b = 0;
  uint32_t setStatusLightCalls = 0;

  bool begin() {
    begun = true;
    return true;
  }
  bool readPlcInput(uint8_t channel) { return channel < 8 ? plcInput[channel] : false; }
  void setStatusLight(uint8_t red, uint8_t green, uint8_t blue) {
    r = red;
    g = green;
    b = blue;
    ++setStatusLightCalls;
  }

  /** Test helper: release every button. */
  void releaseAll() {
    BtnA.pressed = false;
    BtnB.pressed = false;
    BtnC.pressed = false;
  }
};

inline Board& board() {
  static Board instance;
  return instance;
}

}  // namespace m5stamplc_stub

/**
 * Mirrors the real library's global-object style so the firmware source is unmodified.
 *
 * `M5StamPLC` is a reference to a function-local static, so a test can reset the board
 * between cases via `m5stamplc_stub::board()`.
 */
#define M5StamPLC (m5stamplc_stub::board())
