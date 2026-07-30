#include "ui/core/ui_bindings.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string_view>

#include "modbus/register_map.h"

namespace ui {

namespace {

constexpr std::size_t kBufferMin = 4;

const char* pageTitle(UiPage page) {
  switch (page) {
    case UiPage::GlobalStatus: return "System Status";
    case UiPage::InstantFlow: return "Instant Flow";
    case UiPage::CumulativeLiters: return "Cumulative Liters";
    case UiPage::CumulativeCubicMeters: return "Cumulative m^3";
    case UiPage::SessionLiters: return "Session Liters";
    case UiPage::SessionCubicMeters: return "Session m^3";
    case UiPage::MaxFlow: return "Max Flow";
    case UiPage::EnterConfiguration: return "Enter Configuration";
    default: return "";
  }
}

bool copyLiteral(const char* text, char* buffer, std::size_t bufferSize) {
  if (!text || !buffer || bufferSize < 2) {
    return false;
  }
  std::snprintf(buffer, bufferSize, "%s", text);
  return true;
}

std::string_view bindingView(const ui_exporter::Element& element) {
  if (!element.bindingId) {
    return {};
  }
  return std::string_view(element.bindingId);
}

bool parseSensorIndex(std::string_view binding, std::size_t* indexOut) {
  if (!indexOut) {
    return false;
  }
  // std::string_view::starts_with is C++20; the Arduino core builds this at C++17.
  if (binding.rfind("sensor.", 0) != 0) {
    return false;
  }
  const char* start = binding.data() + std::strlen("sensor.");
  char* end = nullptr;
  const unsigned long value = std::strtoul(start, &end, 10);
  if (end == start || value == 0 || value > plc::kNumSensors) {
    return false;
  }
  *indexOut = static_cast<std::size_t>(value - 1);
  return true;
}

}  // namespace

bool UiBindingResolver::resolveText(const UiRenderContext& context,
                                    const ui_exporter::Element& element,
                                    char* buffer,
                                    std::size_t bufferSize) const {
  if (!element.bindingId || !buffer || bufferSize < kBufferMin) {
    return false;
  }

  if (resolvePageBinding(context, element.bindingId, buffer, bufferSize)) {
    return true;
  }
  if (resolveTelemetryBinding(context, element.bindingId, buffer, bufferSize)) {
    return true;
  }
  if (resolveSensorBinding(context, element.bindingId, buffer, bufferSize)) {
    return true;
  }
  if (resolveLegendBinding(context, element.bindingId, buffer, bufferSize)) {
    return true;
  }
  if (resolveConfigBinding(context, element.bindingId, buffer, bufferSize)) {
    return true;
  }
  if (resolveCountdownBinding(context, element.bindingId, buffer, bufferSize)) {
    return true;
  }
  return false;
}

bool UiBindingResolver::resolvePageBinding(const UiRenderContext& context,
                                           const char* bindingId,
                                           char* buffer,
                                           std::size_t bufferSize) const {
  const std::string_view binding(bindingId);
  if (binding == "page.title") {
    return copyLiteral(pageTitle(context.page), buffer, bufferSize);
  }
  return false;
}

bool UiBindingResolver::resolveTelemetryBinding(const UiRenderContext& context,
                                                const char* bindingId,
                                                char* buffer,
                                                std::size_t bufferSize) const {
  const std::string_view binding(bindingId);
  if (binding == "telemetry.total") {
    std::snprintf(buffer,
                  bufferSize,
                  "Total %.2f L | Flow %.2f L/s",
                  context.totalSessionLiters,
                  context.aggregateFlowLps);
    return true;
  }
  if (binding == "telemetry.status") {
    if (context.hasWarnings) {
      std::snprintf(buffer, bufferSize, "%u warning%s", context.warningCount,
                    context.warningCount == 1 ? "" : "s");
    } else {
      copyLiteral("All sensors ready", buffer, bufferSize);
    }
    return true;
  }
  return false;
}

bool UiBindingResolver::resolveSensorBinding(const UiRenderContext& context,
                                             const char* bindingId,
                                             char* buffer,
                                             std::size_t bufferSize) const {
  std::size_t sensorIndex = 0;
  if (!parseSensorIndex(bindingId, &sensorIndex)) {
    return false;
  }
  if (sensorIndex >= context.sensors.size()) {
    return false;
  }

  const auto& sensor = context.sensors[sensorIndex];
  const unsigned sensorLabel = static_cast<unsigned>(sensorIndex + 1);

  if (!sensor.enabled) {
    std::snprintf(buffer, bufferSize, "%u: --", sensorLabel);
    return true;
  }
  if (!sensor.ready) {
    std::snprintf(buffer, bufferSize, "%u: WAIT", sensorLabel);
    return true;
  }
  if (context.page == UiPage::EnterConfiguration) {
    std::snprintf(buffer, bufferSize, "%u: Hold ENTER", sensorLabel);
    return true;
  }

  double value = 0.0;
  const char* unit = "";
  switch (context.page) {
    case UiPage::InstantFlow:
      value = sensor.instantFlow;
      unit = "L/s";
      break;
    case UiPage::CumulativeLiters:
      value = sensor.cumulativeLiters;
      unit = "L";
      break;
    case UiPage::CumulativeCubicMeters:
      value = sensor.cumulativeLiters / 1000.0;
      unit = "m^3";
      break;
    case UiPage::SessionLiters:
      value = sensor.sessionLiters;
      unit = "L";
      break;
    case UiPage::SessionCubicMeters:
      value = sensor.sessionLiters / 1000.0;
      unit = "m^3";
      break;
    case UiPage::MaxFlow:
      value = sensor.maxFlow;
      unit = "L/s";
      break;
    default:
      break;
  }

  std::snprintf(buffer, bufferSize, "%u: %6.2f %s", sensorLabel, value, unit);
  return true;
}

bool UiBindingResolver::resolveLegendBinding(const UiRenderContext& context,
                                             const char* bindingId,
                                             char* buffer,
                                             std::size_t bufferSize) const {
  const std::string_view binding(bindingId);
  if (binding == "legend.led") {
    std::snprintf(buffer,
                  bufferSize,
                  "LED: %uL pulses | %ums",
                  context.ledVolumeStep,
                  context.ledPulsePeriodMs);
    return true;
  }
  if (binding == "legend.warning") {
    return copyLiteral(context.warningSummary.c_str(), buffer, bufferSize);
  }
  return false;
}

bool UiBindingResolver::resolveConfigBinding(const UiRenderContext& context,
                                             const char* bindingId,
                                             char* buffer,
                                             std::size_t bufferSize) const {
  const std::string_view binding(bindingId);
  if (binding == "config.title") {
    if (context.mode == UiMode::Configuration) {
      return copyLiteral("Configuration", buffer, bufferSize);
    }
    return copyLiteral("Config Preview", buffer, bufferSize);
  }
  return false;
}

bool UiBindingResolver::resolveCountdownBinding(const UiRenderContext& context,
                                                const char* bindingId,
                                                char* buffer,
                                                std::size_t bufferSize) const {
  const std::string_view binding(bindingId);
  if (!context.countdownActive && binding != "countdown.hint") {
    return false;
  }
  if (binding == "countdown.title") {
    if (!context.countdownLabel.empty()) {
      return copyLiteral(context.countdownLabel.c_str(), buffer, bufferSize);
    }
    return copyLiteral("Confirmation", buffer, bufferSize);
  }
  if (binding == "countdown.message") {
    if (!context.countdownLabel.empty()) {
      return copyLiteral(context.countdownLabel.c_str(), buffer, bufferSize);
    }
    return copyLiteral("Hold buttons to continue...", buffer, bufferSize);
  }
  if (binding == "countdown.value") {
    std::snprintf(buffer, bufferSize, "%u s", static_cast<unsigned>(context.countdownSeconds));
    return true;
  }
  if (binding == "countdown.hint") {
    const char* text = context.countdownActive ? "Release to cancel" : "";
    return copyLiteral(text, buffer, bufferSize);
  }
  return false;
}

}  // namespace ui
