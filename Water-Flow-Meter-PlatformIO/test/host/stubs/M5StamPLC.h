#pragma once

// Host stub for the M5StamPLC board library.
//
// Only the surface the firmware actually touches: begin, three buttons, readPlcInput,
// setStatusLight, setBacklight and Display. Verified by grepping for `M5StamPLC.` across
// src/.
//
// The buttons are TEST-SETTABLE, which is the whole trick behind the device harness. It
// means button_input.cpp needs no production change at all — its only hardware contact is
// readHardware(), and pointing that at a settable stub turns the real InteractionHandler
// into something a host test can drive press-by-press. No injectable-button-source
// abstraction, no second implementation of the gesture contract to keep in sync.
//
// The Display is a COUNTER, not a framebuffer. Repaint cadence is the one display property
// with a hard requirement attached (§7: acknowledge a button within 100 ms), and it is
// checkable by counting fillScreen calls per unit of simulated time. Pixels are not: what
// they should contain is a layout question the web mockup already owns.
#include <cstdint>
#include <string>

namespace m5stamplc_stub {

/** One button's state, plus a record of what the firmware asked of the hardware. */
struct Button {
  bool pressed = false;
  bool isPressed() const { return pressed; }
};

/**
 * Counts what the renderer asked the panel to do.
 *
 * `fillScreens` is the interesting one: UiRenderer::update() clears the whole panel once
 * per repaint it decides to perform, so the count is exactly the number of frames drawn.
 */
struct DisplayRecorder {
  uint32_t fillScreens = 0;
  uint32_t prints = 0;
  uint32_t fillRects = 0;
  uint32_t drawRects = 0;
  uint32_t fillCircles = 0;
  uint32_t startWrites = 0;
  uint32_t endWrites = 0;
  uint32_t drawStrings = 0;
  /**
   * Everything drawn this frame, joined by '|'.
   *
   * Recorded rather than merely counted so a test can assert what the selector actually SHOWS —
   * which entry carries the cursor, which is marked active, whether the truncation notice
   * appeared. A count would only prove that something was painted.
   */
  std::string strings;
  int rotation = 0;
  int16_t cursorX = 0;
  int16_t cursorY = 0;

  void setRotation(int value) { rotation = value; }
  void fillScreen(uint16_t) {
    ++fillScreens;
    // A full clear starts a new frame, so what was drawn before it is no longer on the panel.
    strings.clear();
  }
  void setTextColor(uint16_t, uint16_t) {}
  void setTextColor(uint16_t) {}
  void setTextSize(int) {}
  void setFont(const void*) {}
  void drawString(const char* text, int32_t, int32_t) {
    ++drawStrings;
    if (text) {
      strings += text;
      strings += '|';
    }
  }
  void startWrite() { ++startWrites; }
  void endWrite() { ++endWrites; }
  void setCursor(int16_t x, int16_t y) {
    cursorX = x;
    cursorY = y;
  }
  void print(const char*) { ++prints; }
  void fillRect(int16_t, int16_t, int16_t, int16_t, uint16_t) { ++fillRects; }
  void drawRect(int16_t, int16_t, int16_t, int16_t, uint16_t) { ++drawRects; }
  void fillCircle(int16_t, int16_t, int16_t, uint16_t) { ++fillCircles; }

  void reset() { *this = DisplayRecorder{}; }
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
  /** Last backlight state written, so the idle path is observable. */
  bool backlight = false;
  uint32_t setBacklightCalls = 0;

  DisplayRecorder Display;

  bool begin() {
    begun = true;
    return true;
  }
  void setBacklight(bool on) {
    backlight = on;
    ++setBacklightCalls;
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

/**
 * The simulated clock behind `millis()`.
 *
 * UiRenderer::drawFlowDots() calls millis() directly for the §4.2 dot phase, so the link
 * set needs the symbol. Keeping it settable means the harness can hold it in step with the
 * `now` it feeds the rest of the stack, rather than having one component read a real wall
 * clock while every other one is driven by simulated time.
 */
inline uint32_t& clockMs() {
  static uint32_t value = 0;
  return value;
}

}  // namespace m5stamplc_stub

inline uint32_t millis() { return m5stamplc_stub::clockMs(); }

/** LovyanGFX names a handful of colours as macros; the renderer uses only this one. */
#ifndef WHITE
#define WHITE 0xFFFFu
#endif

/**
 * `&fonts::Font0` is passed to setFont(). Only its address is used, so an empty tag type
 * is enough — no glyph data has to be stubbed.
 */
namespace fonts {
struct FontStub {};
inline constexpr FontStub Font0{};
}  // namespace fonts

/**
 * Mirrors the real library's global-object style so the firmware source is unmodified.
 *
 * `M5StamPLC` is a reference to a function-local static, so a test can reset the board
 * between cases via `m5stamplc_stub::board()`.
 */
#define M5StamPLC (m5stamplc_stub::board())
