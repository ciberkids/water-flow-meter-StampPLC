#pragma once

#include <cstdint>
#include <string_view>

#include "ui/generated/GeneratedUi.h"

namespace ui {

class ThemePalette {
 public:
  ThemePalette() = default;
  explicit ThemePalette(const ui_exporter::Theme* theme) : theme_(theme) {}

  void bind(const ui_exporter::Theme* theme) { theme_ = theme; }

  std::uint32_t color(std::string_view key, std::uint32_t fallback) const;
  std::uint16_t typographyBase(std::uint16_t fallback = 8) const;
  std::uint16_t typographyValue(std::uint16_t fallback = 10) const;
  std::uint16_t typographyBadge(std::uint16_t fallback = 8) const;
  const char* animationEasing(const char* fallback = "ease-in-out") const;

 private:
  const ui_exporter::Theme* theme_ = nullptr;
};

}  // namespace ui
