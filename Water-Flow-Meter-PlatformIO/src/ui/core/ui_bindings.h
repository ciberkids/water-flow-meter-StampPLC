#pragma once

#include "units.h"

#include <cstddef>

#include "ui/core/ui_controller.h"
#include "ui/generated/GeneratedUi.h"
#include "ui/core/ui_settings.h"

namespace ui {

class UiBindingResolver {
 public:
  /**
   * Gives the resolver live access to the settings catalogue and the controller.
   *
   * Without this every `config.*` binding fell through to its static placeholder, so
   * 13 bindings across 44 elements rendered nothing at all on the device while the
   * mockup showed values.
   */
  void bindSettings(const SettingsAccess* access, const UiController* controller) {
    settings_ = access;
    controller_ = controller;
  }

  bool resolveText(const UiRenderContext& context,
                   const ui_exporter::Element& element,
                   char* buffer,
                   std::size_t bufferSize) const;

 private:
  /** The `net.*` values — read from the context's snapshot, never from WifiManager directly. */
  bool resolveNetworkBinding(const UiRenderContext& context,
                             const char* bindingId,
                             char* buffer,
                             std::size_t bufferSize) const;

  bool resolvePageBinding(const UiRenderContext& context,
                          const char* bindingId,
                          char* buffer,
                          std::size_t bufferSize) const;
  /** The panel's current flow unit, from `config.flowUnit`. A display choice; storage stays L/min. */
  units::FlowUnit panelFlowUnit() const;
  bool resolveTelemetryBinding(const UiRenderContext& context,
                               const char* bindingId,
                               char* buffer,
                               std::size_t bufferSize) const;
  bool resolveSensorBinding(const UiRenderContext& context,
                            const char* bindingId,
                            char* buffer,
                            std::size_t bufferSize) const;
  bool resolveLegendBinding(const UiRenderContext& context,
                            const char* bindingId,
                            char* buffer,
                            std::size_t bufferSize) const;
  bool resolveConfigBinding(const UiRenderContext& context,
                            const char* bindingId,
                            char* buffer,
                            std::size_t bufferSize) const;
  bool resolveDiagnosticsBinding(const UiRenderContext& context,
                                const char* bindingId,
                                char* buffer,
                                std::size_t bufferSize) const;
  bool resolveCountdownBinding(const UiRenderContext& context,
                               const char* bindingId,
                               char* buffer,
                               std::size_t bufferSize) const;

  const SettingsAccess* settings_ = nullptr;
  const UiController* controller_ = nullptr;
};

}  // namespace ui
