#include "ui/pack/ui_pack_selector.h"

#include <cstring>

namespace ui {

namespace {
constexpr const char* kBuiltInLabel = "Built-in";
}

void PackSelector::begin(const char (*names)[PackLoader::kMaxNameBytes],
                         std::size_t count,
                         const char* activeName) {
  entryCount_ = 1;  // entry 0 is the built-in default, always offered (§3.4)
  cursor_ = 0;
  activeIndex_ = kBuiltInIndex;
  truncated_ = false;
  std::memset(names_, 0, sizeof(names_));

  if (!names) {
    count = 0;
  }
  // Reserve slot 0 for the built-in, so a card with kMaxEntries packs shows kMaxEntries - 1.
  const std::size_t room = kMaxEntries - 1;
  truncated_ = count > room;
  const std::size_t take = truncated_ ? room : count;

  for (std::size_t i = 0; i < take; ++i) {
    std::memcpy(names_[entryCount_], names[i], PackLoader::kMaxNameBytes);
    names_[entryCount_][PackLoader::kMaxNameBytes - 1] = '\0';
    if (activeName && std::strcmp(names_[entryCount_], activeName) == 0) {
      activeIndex_ = entryCount_;
    }
    ++entryCount_;
  }

  // Open on whatever is running, not on entry 0. The operator is usually here to see which menu
  // is active or to switch away from it, and both start from the current selection.
  cursor_ = activeIndex_;
}

const char* PackSelector::labelAt(std::size_t index) const {
  if (index == kBuiltInIndex) {
    return kBuiltInLabel;
  }
  if (index >= entryCount_) {
    return "";
  }
  return names_[index];
}

bool PackSelector::isActive(std::size_t index) const { return index == activeIndex_; }

void PackSelector::moveCursor(int32_t delta) {
  if (entryCount_ == 0) {
    return;
  }
  const auto span = static_cast<int32_t>(entryCount_);
  int32_t next = static_cast<int32_t>(cursor_) + (delta % span);
  if (next < 0) next += span;
  if (next >= span) next -= span;
  cursor_ = static_cast<std::size_t>(next);
}

PackSelector::Commit PackSelector::commitAction() const {
  if (cursor_ == activeIndex_) {
    // Already running. Doing nothing beats writing the same pointer and rebooting into the
    // identical UI, which would look like the device ignoring the press.
    return Commit::Nothing;
  }
  return cursor_ == kBuiltInIndex ? Commit::DeletePointer : Commit::WritePointer;
}

const char* PackSelector::commitName() const {
  if (cursor_ == kBuiltInIndex || cursor_ >= entryCount_) {
    return "";
  }
  return names_[cursor_];
}

}  // namespace ui
