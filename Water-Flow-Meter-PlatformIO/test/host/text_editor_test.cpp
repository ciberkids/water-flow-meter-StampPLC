// Host tests for the character-wheel text editor.
//
// This is the interaction nobody will exercise until hardware exists, and the one where a
// subtle bug is most expensive: a user who cannot type their WiFi passphrase cannot use the
// feature at all, and "the wheel skips a character" is invisible in code review.
#include "ui/core/ui_text_editor.h"

#include <cstdio>
#include <cstring>
#include <string>

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const char* what) {
  ++checks;
  std::printf("  %-62s %s\n", what, condition ? "ok" : "FAIL");
  if (!condition) ++failures;
}

void checkStr(const char* actual, const char* expected, const char* what) {
  const bool same = std::strcmp(actual, expected) == 0;
  ++checks;
  std::printf("  %-62s %s\n", what, same ? "ok" : "FAIL");
  if (!same) {
    std::printf("      expected \"%s\"\n      actual   \"%s\"\n", expected, actual);
    ++failures;
  }
}

using ui::TextCharset;
using ui::TextCommand;
using ui::TextEditor;
using ui::TextEditResult;

/** Drives the wheel to a specific character the way a user would, then presses ENTER. */
void typeChar(TextEditor& editor, char c) {
  const std::size_t target = TextCharset::indexOf(c);
  // Shortest way round the ring, exactly as a user scrubbing would find it.
  const auto size = static_cast<int32_t>(TextCharset::kSize);
  int32_t delta = static_cast<int32_t>(target) - static_cast<int32_t>(editor.wheelIndex());
  if (delta > size / 2) delta -= size;
  if (delta < -size / 2) delta += size;
  editor.scrub(delta);
  editor.enterShort();
}

void typeString(TextEditor& editor, const char* text) {
  for (const char* p = text; *p != '\0'; ++p) typeChar(editor, *p);
}

void charsetTests() {
  std::printf("[charset]\n");

  check(TextCharset::kPrintableCount == 95, "95 printable characters, space through tilde");
  check(TextCharset::kSize == 97, "ring is 97 positions including DEL and END");
  check(TextCharset::characterAt(0) == ' ', "position 0 is space");
  check(TextCharset::characterAt(TextCharset::kPrintableCount - 1) == '~', "last printable is ~");
  check(TextCharset::commandAt(TextCharset::kDeleteIndex) == TextCommand::Delete, "DEL is a command");
  check(TextCharset::commandAt(TextCharset::kEndIndex) == TextCommand::End, "END is a command");
  check(TextCharset::commandAt(0) == TextCommand::None, "a printable position is not a command");

  // Every character valid in an SSID or a WPA2 passphrase must be reachable. If one is
  // missing, some users simply cannot enter their credentials.
  bool allReachable = true;
  for (int c = 0x20; c <= 0x7E; ++c) {
    const std::size_t idx = TextCharset::indexOf(static_cast<char>(c));
    if (idx >= TextCharset::kPrintableCount) allReachable = false;
    if (TextCharset::characterAt(idx) != static_cast<char>(c)) allReachable = false;
  }
  check(allReachable, "every printable ASCII character round-trips through the ring");

  // Characters outside the printable range must not silently map onto a real glyph.
  check(TextCharset::indexOf('\n') == TextCharset::kEndIndex, "a control character is not printable");
  check(TextCharset::indexOf(static_cast<char>(0xE9)) == TextCharset::kEndIndex,
        "a non-ASCII byte is rejected, per the Font0 limit (§4.6)");

  // Wrapping in both directions, including a delta far larger than the ring.
  check(TextCharset::advance(0, -1) == TextCharset::kSize - 1, "scrubbing down from 0 wraps to the end");
  check(TextCharset::advance(TextCharset::kSize - 1, 1) == 0, "scrubbing up from the end wraps to 0");
  check(TextCharset::advance(0, 25) == 25, "an accelerated step of 25 lands where expected");
  check(TextCharset::advance(10, -25) == TextCharset::advance(10, -25 + static_cast<int32_t>(TextCharset::kSize)),
        "a large negative delta is equivalent to its wrapped form");
  check(TextCharset::advance(5, static_cast<int32_t>(TextCharset::kSize)) == 5,
        "a full revolution returns to the same position");
}

void editingTests() {
  std::printf("\n[editing]\n");

  TextEditor editor;
  editor.begin(nullptr, 32, false);
  check(editor.length() == 0, "an empty field starts empty");
  check(editor.wheelIndex() == TextCharset::kEndIndex,
        "opens on END, so leaving a value unchanged is one press");

  typeString(editor, "MyNet");
  checkStr(editor.value(), "MyNet", "typing five characters produces them in order");

  // DEL removes the previous character and stays on DEL.
  editor.scrub(static_cast<int32_t>(TextCharset::kDeleteIndex) - static_cast<int32_t>(editor.wheelIndex()));
  check(editor.wheelIndex() == TextCharset::kDeleteIndex, "wheel reached DEL");
  editor.enterShort();
  checkStr(editor.value(), "MyNe", "DEL removes the last character");
  check(editor.wheelIndex() == TextCharset::kDeleteIndex, "wheel stays on DEL for repeated deletion");
  editor.enterShort();
  editor.enterShort();
  checkStr(editor.value(), "My", "DEL repeats without rescrubbing");

  // Deleting past empty must not underflow.
  for (int i = 0; i < 10; ++i) editor.enterShort();
  checkStr(editor.value(), "", "deleting past the start empties the field without underflowing");
  check(editor.length() == 0, "length is zero, not negative-wrapped");

  // END commits.
  editor.scrub(static_cast<int32_t>(TextCharset::kEndIndex) - static_cast<int32_t>(editor.wheelIndex()));
  check(editor.enterShort() == TextEditResult::Commit, "END commits");

  // ENTER long always discards, matching §3.2's meaning in every other editor.
  check(editor.enterLong() == TextEditResult::Discard, "ENTER long discards");
}

void capacityTests() {
  std::printf("\n[capacity]\n");

  TextEditor editor;
  editor.begin(nullptr, 4, false);
  typeString(editor, "abcdef");
  checkStr(editor.value(), "abcd", "typing past maxLength stops at the limit");
  check(editor.full(), "the editor reports being full");
  check(editor.length() == 4, "length equals maxLength, with no overrun");

  // The buffer must remain NUL-terminated at exactly the limit — the classic off-by-one.
  check(editor.value()[4] == '\0', "the value is terminated exactly at the limit");

  // A maxLength beyond the fixed buffer must clamp, not overflow.
  TextEditor big;
  big.begin(nullptr, 9999, false);
  std::string long_input(200, 'x');
  typeString(big, long_input.c_str());
  check(big.length() == TextEditor::kMaxLength, "an oversized maxLength clamps to the buffer");
  check(big.value()[TextEditor::kMaxLength] == '\0', "the clamped buffer is still terminated");

  // An initial value longer than maxLength must be truncated, not trusted.
  TextEditor seeded;
  seeded.begin("abcdefghij", 4, false);
  checkStr(seeded.value(), "abcd", "an over-long initial value is truncated to maxLength");

  // maxLength 0 is nonsense; treat it as the full buffer rather than an unusable editor.
  TextEditor zero;
  zero.begin("hi", 0, false);
  checkStr(zero.value(), "hi", "maxLength 0 falls back to the buffer size rather than rejecting input");
}

void renderTests() {
  std::printf("\n[render]\n");

  char line[32] = {};

  TextEditor editor;
  editor.begin("MyNet", 32, false);
  editor.renderInto(line, sizeof(line));
  checkStr(line, "MyNet[END]", "shows the value and the wheel position");

  typeChar(editor, 'w');
  editor.renderInto(line, sizeof(line));
  checkStr(line, "MyNetw[w]", "the wheel token shows the character just chosen");

  // A space between brackets would be invisible, so it is named.
  editor.scrub(static_cast<int32_t>(TextCharset::indexOf(' ')) - static_cast<int32_t>(editor.wheelIndex()));
  editor.renderInto(line, sizeof(line));
  checkStr(line, "MyNetw[SP]", "a space on the wheel is labelled rather than shown blank");

  // Masking: committed characters hidden, the wheel still legible (R6.3.1).
  TextEditor secret;
  secret.begin("hunter2", 63, true);
  secret.renderInto(line, sizeof(line));
  checkStr(line, "*******[END]", "a masked field hides every committed character");
  check(std::strcmp(secret.value(), "hunter2") == 0,
        "masking is presentation only — the stored value is intact");

  typeChar(secret, 'X');
  secret.renderInto(line, sizeof(line));
  checkStr(line, "********[X]", "the character under the wheel stays legible while masked");

  // Truncation keeps the tail, because that is where the cursor is.
  TextEditor longValue;
  longValue.begin("abcdefghijklmnopqrstuvwxyz", 63, false);
  char narrow[12] = {};
  longValue.renderInto(narrow, sizeof(narrow));
  // 12 bytes: 11 usable, minus the 5-char token leaves 6 characters of tail.
  checkStr(narrow, "uvwxyz[END]", "truncates from the left, keeping the cursor visible");

  // A buffer too small even for the wheel token must not overrun.
  char tiny[4] = {};
  longValue.renderInto(tiny, sizeof(tiny));
  check(std::strlen(tiny) == 3, "a buffer smaller than the wheel token is filled, not overrun");
  check(tiny[3] == '\0', "the tiny buffer is still terminated");

  // Degenerate arguments must be survivable — this runs on a device with no debugger.
  longValue.renderInto(nullptr, 10);
  longValue.renderInto(line, 0);
  check(true, "null and zero-size render targets are ignored rather than crashing");
}

void realisticPassphraseTest() {
  std::printf("\n[realistic]\n");

  // The case the requirement is really about: a mixed-character WPA2 passphrase.
  TextEditor editor;
  editor.begin(nullptr, 63, true);
  const char* passphrase = "Tr0ub4dor&3-xK";
  typeString(editor, passphrase);
  checkStr(editor.value(), passphrase, "a mixed-case passphrase with digits and symbols round-trips");

  editor.scrub(static_cast<int32_t>(TextCharset::kEndIndex) - static_cast<int32_t>(editor.wheelIndex()));
  check(editor.enterShort() == TextEditResult::Commit, "the passphrase commits");

  // Count the presses the requirement claims, so the documented cost stays honest.
  std::size_t presses = 0;
  TextEditor counter;
  counter.begin(nullptr, 63, true);
  for (const char* p = passphrase; *p != '\0'; ++p) {
    const std::size_t target = TextCharset::indexOf(*p);
    const auto size = static_cast<int32_t>(TextCharset::kSize);
    int32_t delta = static_cast<int32_t>(target) - static_cast<int32_t>(counter.wheelIndex());
    if (delta > size / 2) delta -= size;
    if (delta < -size / 2) delta += size;
    presses += static_cast<std::size_t>(delta < 0 ? -delta : delta);  // one short press per step
    presses += 1;                                                     // the ENTER
    counter.scrub(delta);
    counter.enterShort();
  }
  std::printf("      %zu short presses for a %zu-character passphrase by single-stepping\n",
              presses, std::strlen(passphrase));
  check(presses > 100,
        "single-stepping really is as costly as the requirement admits (hence acceleration)");
}

}  // namespace

int main() {
  std::printf("ui::TextEditor — character-wheel text entry\n\n");
  charsetTests();
  editingTests();
  capacityTests();
  renderTests();
  realisticPassphraseTest();
  std::printf("\n%s (%d checks, %d failures)\n", failures == 0 ? "ALL PASSED" : "FAILURES", checks,
              failures);
  return failures == 0 ? 0 : 1;
}
