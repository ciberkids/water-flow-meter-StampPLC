#pragma once

#include <cstddef>

#include "ui/core/ui_controller.h"
#include "ui/generated/GeneratedUi.h"

namespace ui {

class UiBindingResolver {
 public:
  bool resolveText(const UiRenderContext& context,
                   const ui_exporter::Element& element,
                   char* buffer,
                   std::size_t bufferSize) const;

 private:
  bool resolvePageBinding(const UiRenderContext& context,
                          const char* bindingId,
                          char* buffer,
                          std::size_t bufferSize) const;
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
  bool resolveCountdownBinding(const UiRenderContext& context,
                               const char* bindingId,
                               char* buffer,
                               std::size_t bufferSize) const;
};

}  // namespace ui
