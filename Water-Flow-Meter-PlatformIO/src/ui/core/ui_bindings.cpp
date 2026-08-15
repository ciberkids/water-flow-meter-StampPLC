#include "ui/core/ui_bindings.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string_view>

#include "modbus/register_map.h"
// Included directly rather than relied on through ui_controller.h: this file names plc::civilFromEpoch
// and plc::UtcCivil itself, and a transitive include is one refactor away from disappearing.
#include "time/device_clock.h"
#include "units.h"

namespace ui {

namespace {

constexpr std::size_t kBufferMin = 4;

const char* pageTitle(UiPage page) {
  switch (page) {
    // These are the titles the SCREENS carry (spec §3.1, §4.2, §5.4, §5.5, §5a.3, §5b.3, §5b.4), not
    // paraphrases of the enum names. A flow page whose enum says "Instant Flow" while the panel says
    // "Instant Flow (L/m)" is two homes for the page's name, and the unit is the half that matters.
    case UiPage::GlobalStatus: return "System Status";
    case UiPage::InstantFlow: return "Instant Flow (L/m)";
    case UiPage::CumulativeCubicMeters: return "Cumulative (m3)";
    case UiPage::SessionCubicMeters: return "Session (m3)";
    case UiPage::MaxFlow: return "Max Flow (L/m)";
    case UiPage::EnterConfiguration: return "Configuration";
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
  if (binding == "telemetry.flowUnitLabel") {
    return copyLiteral(units::flowUnitLabel(panelFlowUnit()), buffer, bufferSize);
  }
  if (binding == "nav.position") {
    const unsigned depth = controller_ ? controller_->navigator().depth() : 0;
    // Depth alone when the ring size is unknown: `ringCount` is 0 before the navigator has resolved
    // a level, and printing `3/0` would state a position that does not exist.
    if (context.ringCount == 0) {
      std::snprintf(buffer, bufferSize, "L%u", depth);
      return true;
    }
    std::snprintf(buffer, bufferSize, "L%u %u/%u", depth,
                  static_cast<unsigned>(context.ringIndex), static_cast<unsigned>(context.ringCount));
    return true;
  }
  return false;
}

/** The panel's current flow unit, from the setting. L/m when settings are not bound yet. */
units::FlowUnit UiBindingResolver::panelFlowUnit() const {
  if (!settings_ || !settings_->displayFlowUnit) {
    return units::FlowUnit::LitresPerMinute;
  }
  const uint16_t raw = *settings_->displayFlowUnit;
  return raw <= 2 ? static_cast<units::FlowUnit>(raw) : units::FlowUnit::LitresPerMinute;
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
                  context.aggregateFlowLpm);
    return true;
  }
  if (binding == "telemetry.totalFlowLps") {
    // The L/s reading is now the derived one: §2a moved storage to L/min, so this converts on the way
    // OUT for any reader that still wants per-second. It stays in the catalogue rather than being
    // renamed, because a Modbus/MQTT consumer bound to it must keep getting seconds.
    std::snprintf(buffer, bufferSize, "%.2f", context.aggregateFlowLpm / 60.0);
    return true;
  }
  // `%7.2f`, not `%.2f`: a field WIDTH so the column cannot shift under a growing integer part.
  // Eight channels each clamped to q_max = 65535 L/min reach 524280.00, which is nine characters —
  // the width is a floor, so the layout's declared worst case comes from that bound and not from here.
  if (binding == "telemetry.totalFlowLpm") {
    std::snprintf(buffer, bufferSize, "%7.2f",
                  units::flowFromLpm(context.aggregateFlowLpm, panelFlowUnit()));
    return true;
  }
  if (binding == "telemetry.totalVolumeLiters") {
    std::snprintf(buffer, bufferSize, "%.2f", context.totalSessionLiters);
    return true;
  }
  if (binding == "telemetry.totalVolumeM3") {
    std::snprintf(buffer, bufferSize, "%.2f", units::litresToCubicMeters(context.totalSessionLiters));
    return true;
  }
  /**
   * When this session's volume started accumulating — or which of three reasons that is not knowable.
   *
   * The four renderings are chosen so the operator can tell what to DO, which is the only thing that
   * makes three negatives worth three strings instead of one "n/a":
   *
   *   "2026-08-12 14:32 UTC"  the answer. UTC is stated because this device has no timezone setting at
   *                           all — `epochFromUtcCivil` and `civilFromEpoch` are zone-free by design —
   *                           so an unlabelled local-looking time would be a quiet lie in whichever
   *                           country the panel is installed. Minutes, not seconds: a session boundary
   *                           is an operator event, and the three characters buy the ` UTC` instead.
   *   "AWAITING CLOCK"        a reset DID happen, with no clock to date it. Setting the clock fills this
   *                           row in retroactively (DeviceClock::setTime bounds the waiting start), so
   *                           the operator has an action that works.
   *   "CLOCK UNSET"           no clock, and no reset waiting either. Setting the clock is still the
   *                           right move but will NOT produce a timestamp here — only the next reset
   *                           will. Reusing clockSourceText's own "UNSET" vocabulary (§4.6's word for
   *                           the same condition) rather than inventing a second one.
   *   "UNKNOWN"               the clock is trusted and nothing ever recorded a start. No amount of
   *                           re-syncing changes this; it is the module's own word for the case
   *                           (see DeviceClock::sessionStartEpoch's comment).
   *
   * Never a zero and never 1970: the epoch is checked before it is ever handed to the formatter, so the
   * sentinel cannot reach a date field. Longest rendering is 20 characters, which is what P3's spec
   * declares as its worst case.
   */
  if (binding == "telemetry.sessionStart") {
    if (context.clockSet && context.sessionStartEpoch != 0) {
      const plc::UtcCivil at = plc::civilFromEpoch(context.sessionStartEpoch);
      std::snprintf(buffer, bufferSize, "%04d-%02d-%02d %02d:%02d UTC", at.year, at.month, at.day,
                    at.hour, at.minute);
      return true;
    }
    if (!context.clockSet) {
      return copyLiteral(context.sessionStartAwaitingClock ? "AWAITING CLOCK" : "CLOCK UNSET", buffer,
                         bufferSize);
    }
    return copyLiteral("UNKNOWN", buffer, bufferSize);
  }
  /**
   * The worst channel and which one it is.
   *
   * Three states need three formats, so this branches rather than forcing one: with nothing enabled
   * there is no peak to report and no channel to blame; with channels enabled but no flow yet the
   * honest answer is zero, not "--"; and once there is a peak the operator needs to know WHICH pipe
   * it is on, because §5a's whole purpose is spotting an under-dimensioned sensor.
   */
  if (binding == "telemetry.maxFlowLpm") {
    std::size_t owner = 0;
    float peak = 0.0f;
    bool anyEnabled = false;
    for (std::size_t i = 0; i < context.sensors.size(); ++i) {
      if (!context.sensors[i].enabled) {
        continue;
      }
      anyEnabled = true;
      if (context.sensors[i].maxFlow > peak) {
        peak = context.sensors[i].maxFlow;
        owner = i + 1;
      }
    }
    if (!anyEnabled) {
      return copyLiteral("Max Flow: --", buffer, bufferSize);
    }
    const units::FlowUnit unit = panelFlowUnit();
    if (owner == 0) {
      std::snprintf(buffer, bufferSize, "Max Flow: 0.00 %s", units::flowUnitLabel(unit));
      return true;
    }
    std::snprintf(buffer,
                  bufferSize,
                  "Max Flow: %7.2f %s (S%u)",
                  units::flowFromLpm(peak, unit),
                  units::flowUnitLabel(unit),
                  static_cast<unsigned>(owner));
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

  /**
   * `SET?`, not `WAIT` (§4.4).
   *
   * `WAIT` implies warming up. The real condition is that the channel has no valid calibration yet —
   * q_max or the multiplier still zero — and there is nothing to wait for: it needs an operator. The
   * spec settled this and neither the firmware nor the mockup had followed it; only the gallery's
   * hand-written sample table did, so the three disagreed about what a just-wired channel says.
   *
   * `--` means NOT IN SERVICE, never "not detected". No hardware presence detection exists and none
   * is possible — an idle passive pulse sensor is indistinguishable from one whose wire fell off — so
   * the panel must not imply it (§4.4).
   */
  if (metric == "status") {
    const char* state = !sensor.enabled ? "--" : (sensor.ready ? "OK" : "SET?");
    std::snprintf(buffer, bufferSize, "%s", state);
    return true;
  }

  /**
   * NO `%u: ` PREFIX. The channel number is a row LABEL, which the screens now carry themselves.
   *
   * It used to be baked into every reading, which made it redundant wherever the number is already
   * known — the per-sensor config pages say "Sensor 3" in their header and then showed `3: 140.40`
   * underneath — and made the value unusable anywhere but the eight-row telemetry pages it was shaped
   * for. Those pages gained an `N:` label element each, so nothing is lost and the value became
   * reusable.
   */
  if (!sensor.enabled) {
    return copyLiteral("--", buffer, bufferSize);
  }
  if (!sensor.ready) {
    return copyLiteral("SET?", buffer, bufferSize);
  }

  /**
   * THE UNIT LIVES IN THE HEADER, on every telemetry page (§4.3).
   *
   * This wrote the unit into every row, so P1 rendered `1: 2.34 L/s` under a title reading
   * `Instant Flow (L/m)` — the row contradicting its own header, in the wrong unit, with the L/s
   * number. Per-row units were the first choice and §4.3 rejected them: they are impossible on the
   * volume pages, where `8: 99999999.99 m^3` is 18 characters and overflows two columns by 19 px.
   * Rather than let two pages disagree with the other two for a purely arithmetic reason, the unit
   * sits once in the title, two rows above and always visible.
   *
   * So the trailing `%s` is a MARKER, not a unit — and there is exactly one marker: `MAX`, on the
   * peak page, for a channel whose peak reached its own q_max ceiling. That is the whole point of
   * §5a's page: a channel pinned at its ceiling is under-dimensioned for the pipe it is on, and the
   * number alone cannot say so because a legitimate peak can sit just below the same value.
   *
   * Flow is converted to L/min here (§2a). `%7.2f` rather than `%6.2f`: a single channel can reach
   * 9999.99 L/m, which six characters cannot hold.
   */
  double value = 0.0;
  const char* marker = "";
  // Pulses per litre is a COUNT, so it prints without decimals. `360.00 p/L` invites the reader to
  // wonder what the hundredths mean on a quantity that only ever takes whole values.
  bool integral = false;
  if (metric == "instantFlow") {
    value = units::flowFromLpm(sensor.instantFlow, panelFlowUnit());
  } else if (metric == "cumulativeLiters") {
    value = sensor.cumulativeLiters;
  } else if (metric == "cumulativeM3") {
    value = units::litresToCubicMeters(sensor.cumulativeLiters);
  } else if (metric == "sessionLiters") {
    value = sensor.sessionLiters;
  } else if (metric == "sessionM3") {
    value = units::litresToCubicMeters(static_cast<double>(sensor.sessionLiters));
  } else if (metric == "pulsesPerLitre") {
    integral = true;
    /**
     * SET on a pulses-calibrated channel, CALCULATED on a formula-calibrated one.
     *
     * `F = m*Q` with Q in L/min and F in Hz means K = 60*m, since F = K*Q/60. The offset is not
     * folded in: `a` shifts the line rather than scaling it, so no single K expresses a formula with
     * a non-zero offset, and S5 reports the offset on its own row.
     */
    const auto& cfg = settings_ && settings_->configs ? settings_->configs[sensorIndex]
                                                      : SensorCharacteristics{};
    value = cfg.calibration == CalibrationType::PulsesPerLitre
                ? static_cast<double>(cfg.pulses_per_litre)
                : static_cast<double>(cfg.f_multiplier) * 60.0;
  } else if (metric == "maxFlowSinceReset") {
    // The MAX test compares in L/MIN against q_max, which is stored in L/min — before any display
    // conversion. Testing the converted value would flag nothing on a channel shown in m3/h.
    const auto& cfg = settings_ && settings_->configs ? settings_->configs[sensorIndex]
                                                      : SensorCharacteristics{};
    if (cfg.q_max > 0 && sensor.maxFlow >= static_cast<float>(cfg.q_max)) {
      marker = "MAX";
    }
    value = units::flowFromLpm(sensor.maxFlow, panelFlowUnit());
  } else {
    // Unknown metric: fail rather than render a plausible-looking wrong number.
    return false;
  }

  // No trailing space when there is no marker, so P1's row is `1: 65535.00` exactly as declared
  // rather than carrying an invisible twelfth character.
  if (marker[0] == '\0') {
    std::snprintf(buffer, bufferSize, integral ? "%7.0f" : "%7.2f", value);
  } else {
    std::snprintf(buffer, bufferSize, integral ? "%7.0f %s" : "%7.2f %s", value, marker);
  }
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
  /**
   * P0's one legend row: network on the left, LED meaning on the right (spec §3.4).
   *
   * It replaces two separate rows — `net.status` and `legend.led` — because P0 needed a row back to
   * give the walking dots breathing space. The network half is rendered with the SAME helpers
   * `net.status` uses, so the two rows cannot disagree about what state the link is in.
   *
   * `legend.led`'s authored fallback text used to read `LED: Red=Pulse Grn=Ready Blu=Flow`, which its
   * binding always overwrote — a caption that documented three colours where the value showed one
   * number. Only the pulse volume survives here, because that is the part an operator cannot infer
   * by watching the LED.
   */
  if (binding == "legend.status") {
    std::snprintf(buffer,
                  bufferSize,
                  "WiFi %s  MQTT %s  LED 1p/%uL",
                  plc::wifiStateText(context.net.wifiState),
                  mqttStateText(context.net),
                  static_cast<unsigned>(context.ledVolumeStep));
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

  /**
   * The domain of the setting this page is about (§7.2).
   *
   * An open editor names its own descriptor, but a SETTING page has no editor and still shows the
   * range — knowing `1 to 247` before entering the editor is cheaper than discovering it by hitting
   * a limit. So when nothing is being edited this asks the SCREEN which setting it is about, by
   * finding the element whose binding is a known setting. That is the same question
   * `settingOfScreen` answers in the mockup, asked the same way: by looking the binding up rather
   * than by trusting an element id, so renaming `field-value` cannot silently blank the row.
   */
  if (binding == "config.editor.range") {
    const ui::SettingDescriptor* descriptor = nullptr;
    if (controller_ && controller_->editor().active && controller_->editor().setting) {
      descriptor = controller_->editor().setting;
    } else if (context.currentScreen) {
      for (std::size_t i = 0; i < context.currentScreen->elementCount; ++i) {
        const char* elementBinding = context.currentScreen->elements[i].bindingId;
        if (!elementBinding) {
          continue;
        }
        if (const auto* candidate = ui::findSetting(elementBinding)) {
          descriptor = candidate;
          break;
        }
      }
    }
    if (!descriptor) {
      // A page with no setting has no range. Empty rather than false: returning false would let the
      // element fall back to authored placeholder text, which is exactly the authored duplication
      // this value replaces.
      return copyLiteral("", buffer, bufferSize);
    }
    ui::formatSettingRange(*descriptor, buffer, bufferSize);
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

  /**
   * The formula line's two derived pieces (spec §7.8).
   *
   * Both read `--` on a channel calibrated by pulses per litre: that form has no formula, so a term
   * of one is not "zero", it is not applicable. Saying `--` is what makes the calibration choice
   * legible on the very rows it disables.
   */
  if (binding == "config.sensor.adjustTerm" || binding == "config.sensor.formulaQ") {
    if (sensor == 0 || !settings_ || !settings_->configs) {
      return copyLiteral("--", buffer, bufferSize);
    }
    const auto& cfg = settings_->configs[sensor - 1];
    if (cfg.calibration != CalibrationType::Formula) {
      return copyLiteral("--", buffer, bufferSize);
    }
    if (binding == "config.sensor.adjustTerm") {
      const int32_t adjust = static_cast<int32_t>(cfg.adjust);
      std::snprintf(buffer, bufferSize, "%c %ld", adjust < 0 ? '-' : '+',
                    static_cast<long>(std::abs(adjust)));
      return true;
    }
    std::snprintf(buffer, bufferSize, "Q 0..%u L/m", static_cast<unsigned>(cfg.q_max));
    return true;
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
