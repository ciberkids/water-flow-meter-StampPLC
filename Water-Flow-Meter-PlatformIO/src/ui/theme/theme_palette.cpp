#include "ui/theme/theme_palette.h"

#include <cstring>

namespace ui {

namespace {
bool keysEqual(std::string_view lhs, const char* rhs) {
  if (!rhs) {
    return false;
  }
  return lhs == rhs;
}
}  // namespace

std::uint32_t ThemePalette::color(std::string_view key, std::uint32_t fallback) const {
  if (!theme_ || !theme_->colors) {
    return fallback;
  }
  for (std::size_t i = 0; i < theme_->colorCount; ++i) {
    const auto& entry = theme_->colors[i];
    if (keysEqual(key, entry.key)) {
      return entry.argb8888;
    }
  }
  return fallback;
}

std::uint16_t ThemePalette::typographyBase(std::uint16_t fallback) const {
  return theme_ ? theme_->typographyBase : fallback;
}

std::uint16_t ThemePalette::typographyValue(std::uint16_t fallback) const {
  return theme_ ? theme_->typographyValue : fallback;
}

std::uint16_t ThemePalette::typographyBadge(std::uint16_t fallback) const {
  return theme_ ? theme_->typographyBadge : fallback;
}

}  // namespace ui
