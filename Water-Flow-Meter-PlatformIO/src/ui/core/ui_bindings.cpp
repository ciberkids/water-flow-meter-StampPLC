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
    case UiPage::FactoryReset: return "Factory Reset";
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


/**
 * Splits `sensor.<n>.<metric>` into a zero-based index and the metric suffix.
 *
 * The suffix used to be ignored: the metric came from `context.page` instead, so
 * an element authored as `sensor.3.sessionLiters` rendered whatever the current
 * page's metric happened to be. The mockup and the device could therefore
 * disagree about what a number meant. Reading the suffix also *removes* the page
 * switch below rather than adding to it.
 */
bool parseSensorBinding(std::string_view binding,
                        std::size_t* indexOut,
                        std::string_view* metricOut) {
  if (!indexOut || !metricOut) {
    return false;
  }
  // std::string_view::starts_with is C++20; the Arduino core builds this at C++17.
  constexpr std::string_view kPrefix = "sensor.";
  if (binding.rfind(kPrefix, 0) != 0) {
    return false;
  }
  const std::string_view rest = binding.substr(kPrefix.size());
  const std::size_t dot = rest.find('.');
  const std::string_view digits = (dot == std::string_view::npos) ? rest : rest.substr(0, dot);
  if (digits.empty()) {
    return false;
  }

  unsigned long value = 0;
  for (const char c : digits) {
    if (c < '0' || c > '9') {
      return false;
    }
    value = value * 10 + static_cast<unsigned long>(c - '0');
    if (value > plc::kNumSensors) {
      return false;
    }
  }
  if (value == 0) {
    return false;
  }

  *indexOut = static_cast<std::size_t>(value - 1);
  *metricOut = (dot == std::string_view::npos) ? std::string_view{} : rest.substr(dot + 1);
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

  if (resolveNetworkBinding(context, element.bindingId, buffer, bufferSize)) {
    return true;
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
  if (resolveDiagnosticsBinding(context, element.bindingId, buffer, bufferSize)) {
    return true;
  }
  if (resolveCountdownBinding(context, element.bindingId, buffer, bufferSize)) {
    return true;
  }
  return false;
}

namespace {

/** `a.b.c.d`, or "---" when no address has been assigned. */
void formatIpv4(uint32_t packed, char* out, std::size_t size) {
  if (packed == 0) {
    // NOT "0.0.0.0": that reads as a configured address and would send somebody looking for a
    // routing problem. "---" says the device has not been given one yet.
    std::snprintf(out, size, "---");
    return;
  }
  std::snprintf(out, size, "%u.%u.%u.%u", static_cast<unsigned>((packed >> 24) & 0xFF),
                static_cast<unsigned>((packed >> 16) & 0xFF),
                static_cast<unsigned>((packed >> 8) & 0xFF), static_cast<unsigned>(packed & 0xFF));
}

/** The MQTT half of the summary, kept honest about the three states it can be in. */
const char* mqttStateText(const plc::NetStatusSnapshot& net) {
  if (!net.mqttEnabled) return "OFF";
  if (net.mqttConnected) return "OK";
  // Enabled but not connected. Distinguishing "no broker configured" from "configured and down" is
  // the difference between "finish setting it up" and "go look at the broker".
  return net.mqttConfigured ? "DOWN" : "UNSET";
}

}  // namespace

bool UiBindingResolver::resolveNetworkBinding(const UiRenderContext& context,
                                              const char* bindingId,
                                              char* buffer,
                                              std::size_t bufferSize) const {
  const std::string_view binding(bindingId);
  const plc::NetStatusSnapshot& net = context.net;

  if (binding == "net.status") {
    // The main-screen indicator the owner asked for first. One line because P0 has one row to spare,
    // and both halves matter: a device with WiFi up and MQTT down looks identical to a working one
    // from the WiFi indicator alone.
    std::snprintf(buffer, bufferSize, "WiFi %s  MQTT %s", plc::wifiStateText(net.wifiState),
                  mqttStateText(net));
    return true;
  }
  if (binding == "net.wifi.state") {
    return copyLiteral(plc::wifiStateText(net.wifiState), buffer, bufferSize);
  }
  if (binding == "net.wifi.ssid") {
    return copyLiteral(net.ssid[0] != '\0' ? net.ssid : "(not set)", buffer, bufferSize);
  }
  if (binding == "net.wifi.ip") {
    formatIpv4(net.ipAddress, buffer, bufferSize);
    return true;
  }
  if (binding == "net.wifi.rssi") {
    // Only meaningful while associated; anything else would present stale noise as a measurement.
    if (net.wifiState != plc::WifiState::Connected) {
      return copyLiteral("--", buffer, bufferSize);
    }
    std::snprintf(buffer, bufferSize, "%d", static_cast<int>(net.rssiDbm));
    return true;
  }
  if (binding == "net.mqtt.state") {
    return copyLiteral(mqttStateText(net), buffer, bufferSize);
  }
  if (binding == "net.ap.ssid") {
    return copyLiteral(net.apSsid[0] != '\0' ? net.apSsid : "(inactive)", buffer, bufferSize);
  }
  if (binding == "net.ap.password") {
    // Deliberately NOT masked. R5.3: this describes an access point the device is broadcasting, which
    // anyone in range already sees, and an operator standing at the panel needs to read it off. The
    // passphrase the operator GAVE us is a different thing and never renders.
    return copyLiteral(net.apPassword[0] != '\0' ? net.apPassword : "(inactive)", buffer, bufferSize);
  }
  if (binding == "net.ap.ip") {
    formatIpv4(net.apIpAddress, buffer, bufferSize);
    return true;
  }
  if (binding == "net.portal.remaining") {
    std::snprintf(buffer, bufferSize, "%u", static_cast<unsigned>(net.portalRemainingS));
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
  if (binding == "telemetry.totalFlowLps") {
    std::snprintf(buffer, bufferSize, "%.2f", context.aggregateFlowLps);
    return true;
  }
  if (binding == "telemetry.totalVolumeLiters") {
    std::snprintf(buffer, bufferSize, "%.2f", context.totalSessionLiters);
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
  std::string_view metric;
  if (!parseSensorBinding(bindingId, &sensorIndex, &metric)) {
    return false;
  }
  if (sensorIndex >= context.sensors.size()) {
    return false;
  }

  const auto& sensor = context.sensors[sensorIndex];
  const unsigned sensorLabel = static_cast<unsigned>(sensorIndex + 1);

  // "status" reports readiness regardless of whether a metric is available.
  if (metric == "status") {
    const char* state = !sensor.enabled ? "--" : (sensor.ready ? "OK" : "WAIT");
    std::snprintf(buffer, bufferSize, "%s", state);
    return true;
  }

  if (!sensor.enabled) {
    std::snprintf(buffer, bufferSize, "%u: --", sensorLabel);
    return true;
  }
  if (!sensor.ready) {
    std::snprintf(buffer, bufferSize, "%u: WAIT", sensorLabel);
    return true;
  }

  double value = 0.0;
  const char* unit = "";
  if (metric == "instantFlow") {
    value = sensor.instantFlow;
    unit = "L/s";
  } else if (metric == "cumulativeLiters") {
    value = sensor.cumulativeLiters;
    unit = "L";
  } else if (metric == "cumulativeM3") {
    value = sensor.cumulativeLiters / 1000.0;
    unit = "m^3";
  } else if (metric == "sessionLiters") {
    value = sensor.sessionLiters;
    unit = "L";
  } else if (metric == "sessionM3") {
    value = sensor.sessionLiters / 1000.0;
    unit = "m^3";
  } else if (metric == "maxFlowSinceReset") {
    value = sensor.maxFlow;
    unit = "L/s";
  } else {
    // Unknown metric: fail rather than render a plausible-looking wrong number.
    return false;
  }

  std::snprintf(buffer, bufferSize, "%u: %6.2f %s", sensorLabel, value, unit);
  return true;
}

bool UiBindingResolver::resolveDiagnosticsBinding(const UiRenderContext& context,
                                                 const char* bindingId,
                                                 char* buffer,
                                                 std::size_t bufferSize) const {
  // Uses the same `binding == "..."` idiom as its siblings, deliberately: the export
  // gate scrapes that pattern to learn which bindings the firmware can resolve, and an
  // inverted comparison here read as unresolvable long after it worked.
  const std::string_view binding(bindingId);
  if (binding == "diagnostics.pollingRateKhz") {
    std::snprintf(buffer, bufferSize, "%.1f", static_cast<double>(context.pollingRateKhz));
    return true;
  }
  if (binding != "diagnostics.undersampling") {
    return false;
  }
  if (context.warningFlags == 0) {
    return copyLiteral("OK", buffer, bufferSize);
  }
  // Name the offending channels rather than printing a bitmask.
  char list[24] = {};
  std::size_t used = 0;
  for (unsigned i = 0; i < plc::kNumSensors && used + 3 < sizeof(list); ++i) {
    if ((context.warningFlags >> i) & 0x01) {
      used += static_cast<std::size_t>(
          std::snprintf(list + used, sizeof(list) - used, used ? ",%u" : "%u", i + 1));
    }
  }
  std::snprintf(buffer, bufferSize, "! S%s", list);
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

  const uint8_t sensor = controller_ ? controller_->navigator().sensorIndex() : 0;

  // The value being edited, as opposed to the one in force. Bound by an editor's
  // pending-value element; the saved-value element binds the setting id itself and so
  // falls through to the live read below.
  if (binding == "config.editor.pending") {
    if (!controller_ || !controller_->editor().active || !controller_->editor().setting) {
      return false;
    }
    const auto& editor = controller_->editor();
    formatSetting(*editor.setting, editor.pending, buffer, bufferSize);
    return true;
  }

  if (binding == "config.selectedSensor") {
    if (sensor == 0) {
      return copyLiteral("-", buffer, bufferSize);
    }
    std::snprintf(buffer, bufferSize, "%u", static_cast<unsigned>(sensor));
    return true;
  }

  if (binding == "config.uartFrameSummary") {
    if (!settings_ || !settings_->link) {
      return false;
    }
    char frame[8] = {};
    settings_->link->staged().frameSummary(frame, sizeof(frame));
    return copyLiteral(frame, buffer, bufferSize);
  }

  if (binding == "config.sensor.undersamplingFlag") {
    if (sensor == 0) {
      return copyLiteral("-", buffer, bufferSize);
    }
    const bool flagged = (context.warningFlags >> (sensor - 1)) & 0x01;
    return copyLiteral(flagged ? "WARN" : "OK", buffer, bufferSize);
  }

  if (binding == "config.sensor.nyquistWarning") {
    if (controller_ && controller_->editor().commitFailed) {
      // A refusal an override cannot fix. Offering "Save anyway" here would be a lie.
      return copyLiteral("Write refused. UP=Back", buffer, bufferSize);
    }
    if (!controller_ || !controller_->editor().nyquistPrompt) {
      // Not a failure: with no prompt pending there is simply nothing to say, and the
      // element keeps whatever static text it was authored with.
      return copyLiteral("", buffer, bufferSize);
    }
    return copyLiteral("Sampling too slow. UP=Edit DOWN=Save anyway", buffer, bufferSize);
  }

  // Everything else in the catalogue: read the live value and format it with its
  // unit or enum label.
  if (const auto* setting = findSetting(bindingId)) {
    if (!settings_) {
      return false;
    }
    // Text settings take the other accessor pair. Without this arm they reached readSetting, which
    // has no int32_t reading for them and returns 0 — so an editor's "Saved" line would have shown
    // an SSID as "0". formatSettingText applies the masking, so a passphrase reads "********" and a
    // never-set field reads "(not set)" rather than rendering blank.
    if (setting->kind == ui::SettingKind::Text) {
      char stored[plc::NetSettings::kMaxValueBytes + 1] = {};
      if (!readSettingText(*setting, *settings_, stored, sizeof(stored))) {
        return false;
      }
      formatSettingText(*setting, stored, buffer, bufferSize);
      return true;
    }
    const int32_t value = readSetting(*setting, sensor, *settings_);
    formatSetting(*setting, value, buffer, bufferSize);
    return true;
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
