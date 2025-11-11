#pragma once

#include <cstddef>

#include "ui/core/ui_controller.h"
#include "ui/core/ui_renderer.h"
#include "ui/generated/GeneratedUi.h"
#include "ui/theme/theme_palette.h"

namespace ui {

struct UiAssets {
  const ui_exporter::Screen* screens = nullptr;
  std::size_t screenCount = 0;
  const ui_exporter::Theme* theme = nullptr;
  const ui_exporter::Metadata* metadata = nullptr;
  ThemePalette palette;
};

UiAssets loadGeneratedAssets();

}  // namespace ui
