#include "ui/core/ui_screen_router.h"

#include <cstring>

namespace ui {

UiScreenRouter::UiScreenRouter(const UiAssets& assets, UiScreenMap map)
    : assets_(assets), map_(map) {
  info_ = findById(map_.infoScreenId);
  configuration_ = findById(map_.configurationScreenId);
  countdown_ = findById(map_.countdownScreenId);
}

const ui_exporter::Screen* UiScreenRouter::screenForMode(UiMode mode) const {
  switch (mode) {
    case UiMode::Info:
      return info_;
    case UiMode::Configuration:
      return configuration_;
    default:
      return nullptr;
  }
}

const ui_exporter::Screen* UiScreenRouter::overlayForCountdown() const {
  return countdown_;
}

const ui_exporter::Screen* UiScreenRouter::screenById(const char* id) const {
  return findById(id);
}

const ui_exporter::Screen* UiScreenRouter::findById(const char* id) const {
  if (!id || !assets_.screens) {
    return nullptr;
  }
  for (std::size_t i = 0; i < assets_.screenCount; ++i) {
    const auto& screen = assets_.screens[i];
    if (screen.id && std::strcmp(screen.id, id) == 0) {
      return &screen;
    }
  }
  return nullptr;
}

}  // namespace ui

