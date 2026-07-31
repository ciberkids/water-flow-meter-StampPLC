#include "ui/core/ui_screen_router.h"

#include <cstring>

#include "ui/core/ui_pages.h"

namespace ui {

// Screen ids and their UiPage mapping now live in ui/core/ui_pages.h, so the manifest
// generator declares exactly what this router resolves.

UiScreenRouter::UiScreenRouter(const UiAssets& assets) : assets_(assets) {
  bool resolved = true;
  for (std::size_t i = 0; i < static_cast<std::size_t>(UiPage::Count); ++i) {
    infoPages_[i] = findById(kInfoScreenIds[i]);
    resolved = resolved && infoPages_[i] != nullptr;
  }
  configuration_ = findById(kConfigurationScreenId);
  countdown_ = findById(kFactoryResetCountdownScreenId);
  fullyResolved_ = resolved && configuration_ != nullptr && countdown_ != nullptr;
}

const ui_exporter::Screen* UiScreenRouter::screenForMode(UiMode mode, UiPage page) const {
  switch (mode) {
    case UiMode::Info: {
      const auto index = static_cast<std::size_t>(page);
      if (index >= static_cast<std::size_t>(UiPage::Count)) {
        return nullptr;
      }
      return infoPages_[index];
    }
    case UiMode::Configuration:
      return configuration_;
    default:
      return nullptr;
  }
}

const ui_exporter::Screen* UiScreenRouter::overlayForCountdown(const char* screenId) const {
  if (screenId) {
    if (const auto* screen = findById(screenId)) {
      return screen;
    }
  }
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
