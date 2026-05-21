#include "ui/core/ui_module.h"

#include "ui/generated/GeneratedUi.h"

namespace ui {

UiAssets loadGeneratedAssets() {
  UiAssets assets;
  assets.screens = ui_exporter::kGeneratedScreens;
  assets.screenCount = ui_exporter::kGeneratedScreenCount;
  assets.theme = &ui_exporter::kGeneratedTheme;
  assets.metadata = &ui_exporter::kGeneratedMetadata;
  assets.palette.bind(assets.theme);
  return assets;
}

}  // namespace ui
