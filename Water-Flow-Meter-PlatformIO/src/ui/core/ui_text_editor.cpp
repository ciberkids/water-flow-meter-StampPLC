#include "ui/core/ui_text_editor.h"

#include <cstdio>
#include <cstring>

namespace ui {

void TextEditor::begin(const char* initial, std::size_t maxLength, bool masked) {
  maxLength_ = maxLength == 0 || maxLength > kMaxLength ? kMaxLength : maxLength;
  masked_ = masked;
  length_ = 0;
  std::memset(buffer_, 0, sizeof(buffer_));

  if (initial != nullptr) {
    while (initial[length_] != '\0' && length_ < maxLength_) {
      buffer_[length_] = initial[length_];
      ++length_;
    }
  }

  // Open on END rather than on a space. Someone re-entering a settings screen usually wants
  // to leave the value alone, and starting on the commit position makes that a single press
  // instead of a scrub back around the ring.
  wheel_ = TextCharset::kEndIndex;
}

void TextEditor::scrub(int32_t delta) { wheel_ = TextCharset::advance(wheel_, delta); }

TextEditResult TextEditor::enterShort() {
  switch (TextCharset::commandAt(wheel_)) {
    case TextCommand::End:
      return TextEditResult::Commit;

    case TextCommand::Delete:
      if (length_ > 0) {
        buffer_[--length_] = '\0';
      }
      // Stay on DEL: deleting several characters is the common case, and forcing a scrub
      // back to DEL between each one would be gratuitous.
      return TextEditResult::Continue;

    case TextCommand::None:
      break;
  }

  if (length_ < maxLength_) {
    buffer_[length_++] = TextCharset::characterAt(wheel_);
    buffer_[length_] = '\0';
  }
  // At capacity the press is deliberately ignored rather than wrapping or erroring: the
  // renderer shows the field is full, and silently dropping input is less surprising than
  // silently overwriting the last character.
  return TextEditResult::Continue;
}

void TextEditor::renderInto(char* out, std::size_t size) const {
  if (out == nullptr || size == 0) return;

  // Build the full line first, then decide what fits. The wheel token is never truncated —
  // losing sight of the character you are choosing would make the editor unusable.
  char wheelToken[8] = {};
  const char* label = TextCharset::labelAt(wheel_);
  if (label != nullptr) {
    std::snprintf(wheelToken, sizeof(wheelToken), "[%s]", label);
  } else {
    const char c = TextCharset::characterAt(wheel_);
    // A space is invisible between brackets, so name it.
    if (c == ' ') {
      std::snprintf(wheelToken, sizeof(wheelToken), "[SP]");
    } else {
      std::snprintf(wheelToken, sizeof(wheelToken), "[%c]", c);
    }
  }

  const std::size_t tokenLen = std::strlen(wheelToken);
  if (tokenLen + 1 > size) {
    // Not even the wheel fits; show as much of it as possible rather than nothing.
    std::memcpy(out, wheelToken, size - 1);
    out[size - 1] = '\0';
    return;
  }

  const std::size_t roomForText = size - 1 - tokenLen;
  // Keep the tail: the user is working at the end of the string.
  const std::size_t shown = length_ > roomForText ? roomForText : length_;
  const std::size_t skipped = length_ - shown;

  std::size_t w = 0;
  for (std::size_t i = 0; i < shown; ++i) {
    out[w++] = masked_ ? '*' : buffer_[skipped + i];
  }
  std::memcpy(out + w, wheelToken, tokenLen);
  out[w + tokenLen] = '\0';
}

}  // namespace ui
