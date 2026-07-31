#pragma once

#include <cstddef>
#include <cstdint>

namespace ui {

/**
 * Character-wheel text entry for a three-button device.
 *
 * The device has UP, DOWN and ENTER. Every other gesture is already spoken for by
 * Display_UI_Requirements §3, so text entry cannot invent one: UP/DOWN move through a
 * character ring, ENTER advances the cursor, and the ring itself carries the two editing
 * commands as pseudo-characters. That is why `DEL` and `END` are members of the charset
 * rather than gestures — the gesture space is full, and overloading UP+DOWN would break the
 * display-off combo that works from every screen at every depth.
 *
 * Deliberately free of Arduino and of the rest of the UI, like ui_accel.h and
 * led_patterns.h. Entering a WPA2 passphrase is the least forgiving interaction in the
 * product and the one nobody will exercise until hardware exists, so it is written to be
 * exhaustively testable on a host.
 *
 * See WiFi_MQTT_Connectivity.md §6.3. Honest cost, from that document: roughly 380 short
 * presses for a 16-character passphrase, or a few tens of seconds of held-button scrubbing
 * using the acceleration tiers. This class does not make that pleasant; it makes it correct.
 */

/** The ring: printable ASCII plus two commands. Order is what the user scrubs through. */
enum class TextCommand : uint8_t { None, Delete, End };

/**
 * Position in the character ring.
 *
 * Indices 0..kPrintableCount-1 are printable ASCII in the order below; the last two are
 * DEL and END. Kept as a single ring so one pair of buttons reaches everything and the
 * acceleration tiers apply uniformly.
 */
class TextCharset {
 public:
  /** Space (0x20) through tilde (0x7E) — every character valid in an SSID or a PSK. */
  static constexpr uint8_t kFirstPrintable = 0x20;
  static constexpr uint8_t kLastPrintable = 0x7E;
  static constexpr std::size_t kPrintableCount = kLastPrintable - kFirstPrintable + 1;  // 95
  static constexpr std::size_t kDeleteIndex = kPrintableCount;
  static constexpr std::size_t kEndIndex = kPrintableCount + 1;
  static constexpr std::size_t kSize = kPrintableCount + 2;  // 97

  /** The character at a ring position, or '\0' for the two commands. */
  static constexpr char characterAt(std::size_t index) {
    if (index >= kPrintableCount) return '\0';
    return static_cast<char>(kFirstPrintable + index);
  }

  static constexpr TextCommand commandAt(std::size_t index) {
    if (index == kDeleteIndex) return TextCommand::Delete;
    if (index == kEndIndex) return TextCommand::End;
    return TextCommand::None;
  }

  /** Ring position for a character, or kEndIndex if it is not representable. */
  static constexpr std::size_t indexOf(char c) {
    const auto u = static_cast<unsigned char>(c);
    if (u < kFirstPrintable || u > kLastPrintable) return kEndIndex;
    return static_cast<std::size_t>(u - kFirstPrintable);
  }

  /** Label shown for a ring position: the glyph itself, or "DEL"/"END". */
  static constexpr const char* labelAt(std::size_t index) {
    if (index == kDeleteIndex) return "DEL";
    if (index == kEndIndex) return "END";
    return nullptr;  // Caller renders characterAt(index).
  }

  /** Moves `index` by `delta`, wrapping. Wrapping matters: the ring has no dead ends. */
  static constexpr std::size_t advance(std::size_t index, int32_t delta) {
    const auto size = static_cast<int32_t>(kSize);
    int32_t next = static_cast<int32_t>(index) + (delta % size);
    if (next < 0) next += size;
    if (next >= size) next -= size;
    return static_cast<std::size_t>(next);
  }
};

/** What a commit attempt concluded. */
enum class TextEditResult : uint8_t {
  Continue,  /**< Still editing. */
  Commit,    /**< END selected; the buffer is the new value. */
  Discard    /**< Caller asked to abandon (ENTER long, per §3.2). */
};

/**
 * Editing state for one text setting.
 *
 * Fixed capacity, no allocation: the longest field in the catalogue is a 64-byte MQTT host,
 * and a firmware that allocates while a user holds a button is a firmware that fragments.
 */
class TextEditor {
 public:
  /** Longest text setting plus a terminator. See WiFi_MQTT_Connectivity.md §6.1. */
  static constexpr std::size_t kMaxLength = 64;

  /**
   * Begins editing.
   *
   * `initial` may be nullptr or empty. `maxLength` is clamped to kMaxLength. When `masked`,
   * `renderInto` hides every committed character but leaves the one under the cursor
   * legible — typing a passphrase blind on three buttons is not reasonable, and showing it
   * whole on a wall-mounted display is not either (R6.3.1).
   */
  void begin(const char* initial, std::size_t maxLength, bool masked);

  /** Moves the wheel at the cursor. `delta` comes from the acceleration tiers. */
  void scrub(int32_t delta);

  /**
   * ENTER short.
   *
   * On a printable character: accept it and advance the cursor. On DEL: remove the previous
   * character. On END: commit.
   */
  TextEditResult enterShort();

  /** ENTER long — discard, the same meaning it has in every other editor (§3.2). */
  TextEditResult enterLong() const { return TextEditResult::Discard; }

  /** The value as it stands, NUL-terminated. Never masked — this is what gets stored. */
  const char* value() const { return buffer_; }
  std::size_t length() const { return length_; }
  std::size_t cursor() const { return length_; }
  std::size_t wheelIndex() const { return wheel_; }
  bool masked() const { return masked_; }
  bool full() const { return length_ >= maxLength_; }

  /**
   * Renders the editing line for the display: the committed text (masked if required)
   * followed by the wheel position in brackets, e.g. `***[k]` or `MyNet[END]`.
   *
   * Truncates from the LEFT when it will not fit, keeping the cursor visible — the end of
   * the string is where the user is working, so that is what must stay on screen.
   */
  void renderInto(char* out, std::size_t size) const;

 private:
  char buffer_[kMaxLength + 1] = {};
  std::size_t length_ = 0;
  std::size_t maxLength_ = kMaxLength;
  std::size_t wheel_ = 0;
  bool masked_ = false;
};

}  // namespace ui
