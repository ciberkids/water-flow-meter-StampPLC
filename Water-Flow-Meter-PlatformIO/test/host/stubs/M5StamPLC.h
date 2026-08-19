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
#include <Arduino.h>

#include <cstdint>
#include <string>
#include <vector>

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
  /** The unlit flow dots are OUTLINES, so they are counted separately from the lit one. */
  uint32_t drawCircles = 0;
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
  /**
   * WHERE each string landed, and WHERE each rectangle was filled.
   *
   * `strings` answers "what is on the panel" and cannot answer "where". That was tolerable while every
   * geometry question belonged to the web mockup, and stopped being tolerable when one of them acquired a
   * spec decision of its own: §2c places the warning banner at y=116..133, the footer row, and the
   * argument for the move is entirely about which band it covers. A recorder that keeps only the LAST
   * cursor pair (`cursorX`/`cursorY` below) and throws every fillRect argument away cannot check that —
   * the same shape of gap the print() comment below describes for text, one level down.
   *
   * ADDITIVE: `strings` keeps its exact format, because existing assertions match on it.
   */
  struct Placed {
    int16_t x;
    int16_t y;
    std::string text;
  };
  std::vector<Placed> placed;
  struct Rect {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
  };
  std::vector<Rect> rects;
  int rotation = 0;
  int16_t cursorX = 0;
  int16_t cursorY = 0;

  void setRotation(int value) { rotation = value; }
  void fillScreen(uint16_t) {
    ++fillScreens;
    // A full clear starts a new frame, so what was drawn before it is no longer on the panel. All three
    // records are cleared by the same event because they answer one question between them — "what is on
    // the panel, and where" — and a `strings` that resets while `placed` accumulates would let an
    // absence assertion match a position from a frame that is no longer showing.
    //
    // MEASURED, not assumed: removing these two lines fails nothing today, because every test that reads
    // the records calls resetFrames() (which reconstructs the whole recorder) and then ticks exactly one
    // frame. They are here for the invariant, and they become load-bearing the moment a test reads the
    // records after a window that painted more than once.
    strings.clear();
    placed.clear();
    rects.clear();
  }
  void setTextColor(uint16_t, uint16_t) {}
  void setTextColor(uint16_t) {}
  void setTextSize(int) {}
  void setFont(const void*) {}
  void drawString(const char* text, int32_t x, int32_t y) {
    ++drawStrings;
    if (text) {
      strings += text;
      strings += '|';
      // Recorded into the same vector as print() for the same reason both append to `strings`: "where is
      // it on the panel" is one question, and answering it from two records depending on which primitive
      // the renderer happened to choose is how the gap above stayed open.
      placed.push_back(Placed{static_cast<int16_t>(x), static_cast<int16_t>(y), text});
    }
  }
  void startWrite() { ++startWrites; }
  void endWrite() { ++endWrites; }
  void setCursor(int16_t x, int16_t y) {
    cursorX = x;
    cursorY = y;
  }
  /**
   * Records the TEXT, not just the fact that something was printed.
   *
   * It counted only, and that made the panel's actual content unobservable for every ordinary screen:
   * UiRenderer draws the firmware-owned Select Menu with drawString(), which `strings` captured, but
   * every element of every GENERATED screen goes through setCursor() + print(), which it did not. So a
   * test could assert what the selector shows and nothing whatsoever about what P0..P6 or any config
   * page shows — a binding could resolve perfectly and be attached to no element, and the suite would
   * agree with the export gates that everything was fine while the panel drew a blank row.
   *
   * Appended to the same buffer as drawString for that reason: "what is on the panel" is one question,
   * and answering it from two buffers depending on which primitive the renderer happened to choose is
   * how the gap above stayed open.
   */
  void print(const char* text) {
    ++prints;
    if (text) {
      strings += text;
      strings += '|';
      placed.push_back(Placed{cursorX, cursorY, text});
    }
  }
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t) {
    ++fillRects;
    rects.push_back(Rect{x, y, w, h});
  }
  void drawRect(int16_t, int16_t, int16_t, int16_t, uint16_t) { ++drawRects; }
  void fillCircle(int16_t, int16_t, int16_t, uint16_t) { ++fillCircles; }
  void drawCircle(int16_t, int16_t, int16_t, uint16_t) { ++drawCircles; }

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
 *
 * The STORAGE moved to Arduino.h, which is where millis() itself now lives — modbus_manager.cpp needs
 * the symbol and includes nothing that would reach this header. This stays as the name every existing
 * test writes through, and returns a reference to that one shared value rather than a second clock:
 * `m5stamplc_stub::clockMs() = now` must still be what ModbusManager reads, or a session reset driven
 * from the harness would be dated from a clock the harness never set.
 */
inline uint32_t& clockMs() { return arduino_stub::clockMs(); }

}  // namespace m5stamplc_stub

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
