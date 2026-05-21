#pragma once

#include "ui/core/ui_controller.h"
#include "ui/core/ui_module.h"

namespace ui {

struct UiScreenMap {
  const char* infoScreenId = "info-overview";
  const char* configurationScreenId = "configuration";
  const char* countdownScreenId = "countdown";
};

class UiScreenRouter {
 public:
  UiScreenRouter(const UiAssets& assets, UiScreenMap map = {});

  const ui_exporter::Screen* screenForMode(UiMode mode) const;
  const ui_exporter::Screen* overlayForCountdown() const;
  const ui_exporter::Screen* screenById(const char* id) const;

 private:
  const ui_exporter::Screen* findById(const char* id) const;

  const UiAssets& assets_;
  UiScreenMap map_;
  const ui_exporter::Screen* info_ = nullptr;
  const ui_exporter::Screen* configuration_ = nullptr;
  const ui_exporter::Screen* countdown_ = nullptr;
};

}  // namespace ui

