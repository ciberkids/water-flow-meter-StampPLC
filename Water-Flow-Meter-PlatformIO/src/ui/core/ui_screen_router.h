#pragma once

#include <cstddef>

#include "ui/core/ui_controller.h"
#include "ui/core/ui_module.h"

namespace ui {

// Maps logical UI state onto exporter-generated screen IDs.
//
// Info mode uses one authored screen per page (P0..P7), looked up by UiPage
// index. That keeps the "page reuse" model: the dataset defines what each page
// looks like, UiController owns which page is current, and no template
// expansion happens at runtime.
class UiScreenRouter {
 public:
  explicit UiScreenRouter(const UiAssets& assets);

  // Returns the screen to draw for the given mode/page, or nullptr if the
  // dataset does not define one.
  const ui_exporter::Screen* screenForMode(UiMode mode, UiPage page) const;

  // Overlay drawn while a hold-to-confirm countdown is running. Each guarded
  // action has its own countdown screen, so the caller passes the ID from the
  // active countdown; nullptr or an unknown ID falls back to the factory-reset
  // overlay so a countdown is never invisible.
  const ui_exporter::Screen* overlayForCountdown(const char* screenId = nullptr) const;

  const ui_exporter::Screen* screenById(const char* id) const;

  // True when every screen ID this router expects exists in the dataset.
  bool isFullyResolved() const { return fullyResolved_; }

 private:
  const ui_exporter::Screen* findById(const char* id) const;

  const UiAssets& assets_;
  const ui_exporter::Screen* infoPages_[static_cast<std::size_t>(UiPage::Count)] = {};
  const ui_exporter::Screen* configuration_ = nullptr;
  const ui_exporter::Screen* countdown_ = nullptr;
  bool fullyResolved_ = false;
};

}  // namespace ui
