#include "ui/core/ui_screen_router.h"

#include <cstring>

namespace ui {

namespace {

// Order MUST match the UiPage enum in ui_controller.h.
constexpr const char* kInfoScreenIds[] = {
    "info-p0-global-status",       // UiPage::GlobalStatus
    "info-p1-instant-flow",        // UiPage::InstantFlow
    "info-p2-cumulative-liters",   // UiPage::CumulativeLiters
    "info-p3-cumulative-m3",       // UiPage::CumulativeCubicMeters
    "info-p4-session-liters",      // UiPage::SessionLiters
    "info-p5-session-m3",          // UiPage::SessionCubicMeters
    "info-p6-max-flow",            // UiPage::MaxFlow
    "info-p7-enter-config",        // UiPage::EnterConfiguration
};

static_assert(sizeof(kInfoScreenIds) / sizeof(kInfoScreenIds[0]) ==
                  static_cast<std::size_t>(UiPage::Count),
              "kInfoScreenIds must have one entry per UiPage");

// Configuration mode currently lands on its first page. Paging across
// C1..C7/S1..S4 needs the edit-state model that UiController does not have yet.
constexpr const char* kConfigurationScreenId = "config-c1-modbus-id";
constexpr const char* kFactoryResetCountdownScreenId = "countdown-factory-reset";

}  // namespace

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
