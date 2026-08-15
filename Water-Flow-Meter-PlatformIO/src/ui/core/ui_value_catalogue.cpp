#include "ui/core/ui_value_catalogue.h"

#include "modbus/register_map.h"
#include "net/net_register_map.h"

namespace ui {

namespace {

using namespace plc;  // register offsets from modbus/register_map.h

/**
 * Offsets come from register_map.h rather than being repeated as literals, so a change to
 * the register layout moves the manifest with it.
 */
constexpr SensorMetric kSensorMetrics[] = {
    // L/min, per §2a. Register 101 carries the same unit — a Modbus master reading it gets L/min,
    // and Project_document.md's register table says so.
    {"instantFlow", ValueCategory::Reading, ValueType::Number, "L/min", OFF_INSTANT_FLOW,
     "instantaneous flow"},
    {"status", ValueCategory::Reading, ValueType::String, nullptr, OFF_STATUS_FLAGS,
     "status flags (bit 0 inUse, bit 1 isReady)"},
    {"cumulativeLiters", ValueCategory::Accumulated, ValueType::Number, "L",
     OFF_CUMULATIVE_LITERS, "cumulative volume"},
    {"cumulativeM3", ValueCategory::Accumulated, ValueType::Number, "m3", OFF_CUMULATIVE_M3,
     "cumulative volume (derived from litres)"},
    {"sessionLiters", ValueCategory::Accumulated, ValueType::Number, "L", OFF_SESSION_LITERS,
     "session volume"},
    {"sessionM3", ValueCategory::Accumulated, ValueType::Number, "m3", OFF_SESSION_M3,
     "session volume (derived from litres)"},
    {"maxFlowSinceReset", ValueCategory::Accumulated, ValueType::Number, "L/min", OFF_MAX_FLOW,
     "peak flow since last session reset"},
    /**
     * Pulses per litre — SET on a pulses-calibrated channel, CALCULATED on a formula-calibrated one.
     *
     * The point is comparability. A datasheet gives you either `450 pulses/L` or `F = 6*Q - 8`, and
     * with the two forms side by side on the selector there is no way to tell whether two channels
     * are calibrated alike. Both reduce to a pulses-per-litre figure, so showing it makes them
     * comparable at a glance: `F = m*Q` means K = 60*m, because F = K*Q/60.
     *
     * The offset term is deliberately not folded in. `a` shifts the line, it does not scale it, so
     * there is no single K that expresses a formula with a non-zero offset — and pretending otherwise
     * would be worse than showing the scale factor and letting S5 report the offset separately.
     *
     * `kNoRegister` on purpose: on a formula channel register 24 holds zero while this shows 360, so
     * advertising an address would tell a Modbus master to read a value that is not this one.
     */
    {"pulsesPerLitre", ValueCategory::Derived, ValueType::Number, "p/L", kNoRegister,
     "pulses per litre, set or calculated from the multiplier"}};

constexpr SimpleValue kSimpleValues[] = {
    // ── Network status (WiFi_MQTT_Connectivity.md §3.4, §7.3) ─────────────────────────
    //
    // Read-only DERIVED values, not settings: the settings live in NetSettings and already appear in
    // the catalogue. These are what the radio and the broker are actually doing, which is the half the
    // owner asked for first — "the display should be able to display whether or not the wifi is
    // connected (main display) and whether or not the mqtt is connected".
    //
    // Registers are given where §5's block already publishes the same fact, so the manifest tells a
    // Modbus master where to read it rather than implying it is display-only.
    {"net.status", ValueCategory::Derived, ValueType::String, nullptr, kNoRegister,
     ValueSource::Network, false, "One-line WiFi + MQTT summary for the main screen"},
    {"net.wifi.state", ValueCategory::Derived, ValueType::String, nullptr,
     plc::net_reg::kWifiState, ValueSource::Network, false, "WiFi state as ASCII (§3.1)"},
    {"net.wifi.ssid", ValueCategory::Derived, ValueType::String, nullptr, kNoRegister,
     ValueSource::Network, false, "Network the radio is on, or (not set)"},
    {"net.wifi.ip", ValueCategory::Derived, ValueType::String, nullptr, plc::net_reg::kWifiIp,
     ValueSource::Network, false, "DHCP address, or --- before one is assigned"},
    {"net.wifi.rssi", ValueCategory::Derived, ValueType::Number, "dBm", plc::net_reg::kWifiRssi,
     ValueSource::Network, false, "Signal strength while associated"},
    {"net.mqtt.state", ValueCategory::Derived, ValueType::String, nullptr,
     plc::net_reg::kMqttState, ValueSource::Network, false, "MQTT broker connection state"},
    {"net.ap.ssid", ValueCategory::Derived, ValueType::String, nullptr, plc::net_reg::kApSsid,
     ValueSource::Network, false, "Provisioning AP name, water_flow_meter_<n> (R7.5a)"},
    {"net.ap.password", ValueCategory::Derived, ValueType::String, nullptr,
     plc::net_reg::kApPassword, ValueSource::Network, false,
     "Provisioning AP WPA2 key — shown in clear by R5.3, unlike the operator's own passphrase"},
    {"net.ap.ip", ValueCategory::Derived, ValueType::String, nullptr, plc::net_reg::kApIp,
     ValueSource::Network, false, "Address to browse to while the portal is up"},
    {"net.portal.remaining", ValueCategory::Derived, ValueType::Number, "s",
     plc::net_reg::kPortalRemainingS, ValueSource::Network, false,
     "Seconds left on the AP window before R7.6 closes it"},

    /**
     * The aggregate in L/s — now the DERIVED one.
     *
     * §2a moved storage to L/min, so this divides on the way out. It keeps its id and its unit
     * because a consumer bound to it asked for per-second and must keep getting it; renaming or
     * silently redefining it would leave every such reader off by sixty with nothing to notice by.
     */
    {"telemetry.totalFlowLps", ValueCategory::System, ValueType::Number, "L/s", kNoRegister,
     ValueSource::Telemetry, false, "Aggregate instantaneous flow, in litres per second (derived)"},
    /**
     * The same aggregate in L/min — the STORED unit, and what every surface shows (§2a).
     *
     * A separate id from the L/s one above rather than a redefinition of it, which is what let the
     * storage move happen without breaking a single existing reader: anything bound to the L/s name
     * still gets seconds, and it is now the arm that divides.
     */
    {"telemetry.totalFlowLpm", ValueCategory::System, ValueType::Number, "L/min", kNoRegister,
     ValueSource::Telemetry, false, "Aggregate instantaneous flow, in litres per minute"},
    {"telemetry.totalVolumeLiters", ValueCategory::System, ValueType::Number, "L", kNoRegister,
     ValueSource::Telemetry, false, "Aggregate session volume across ready sensors"},
    /**
     * Aggregate session volume in cubic metres — what the PANEL shows, per §2a.1.
     *
     * The litres binding above stays: §2a.1 makes every wire surface carry both units at full
     * precision, and only the 240x135 panel round to m3. Two ids for two audiences, not two homes
     * for one fact — the m3 arm divides the litres one rather than accumulating its own total.
     */
    {"telemetry.totalVolumeM3", ValueCategory::System, ValueType::Number, "m3", kNoRegister,
     ValueSource::Telemetry, false, "Aggregate session volume, in cubic metres"},
    /**
     * The highest per-channel peak and which channel owns it (spec §5a).
     *
     * An argmax over `SensorSnapshot::maxFlow`, which the render context already holds — no new
     * stored state, no new register, no new topic.
     */
    {"telemetry.maxFlowLpm", ValueCategory::System, ValueType::String, nullptr, kNoRegister,
     ValueSource::Telemetry, false, "Highest per-channel peak flow and the channel holding it"},
    /**
     * When the session counters were last cleared — the one thing P3's volume figures were missing.
     *
     * A String, not a Number, and deliberately not a register. There is a real epoch behind it, but the
     * value a panel needs is a rendered date OR one of three different reasons there isn't one, and a
     * numeric register would have to encode "no answer" as a zero that reads as 1970. `pulsesPerLitre`'s
     * comment above records why a half-true address is worse than none; the same applies to a half-true
     * timestamp. A Modbus consumer that wants the epoch should get its own register, typed as one.
     */
    {"telemetry.sessionStart", ValueCategory::Derived, ValueType::String, nullptr, kNoRegister,
     ValueSource::Telemetry, false,
     "When the session counters were last cleared, or why that cannot be said"},
    {"telemetry.total", ValueCategory::System, ValueType::String, nullptr, kNoRegister,
     ValueSource::Telemetry, false, "Aggregate volume and flow summary line"},
    {"telemetry.status", ValueCategory::System, ValueType::String, nullptr, kNoRegister,
     ValueSource::Telemetry, false, "Readiness / warning summary"},
    {"diagnostics.pollingRateKhz", ValueCategory::System, ValueType::Number, "kHz",
     REG_POLLING_RATE_KHZ, ValueSource::Diagnostics, false, "Measured core-0 polling rate"},
    {"diagnostics.undersampling", ValueCategory::System, ValueType::Number, nullptr,
     REG_UNDERSAMPLING_FLAGS, ValueSource::Diagnostics, false,
     "Undersampling flags bitmap (bits 0-7 = sensors 1-8)"},
    {"legend.led", ValueCategory::Derived, ValueType::String, nullptr, kNoRegister,
     ValueSource::UiState, false, "RGB LED status legend text"},
    /**
     * P0's combined network-and-LED row (spec §3.4), folding in what `net.status` used to render on
     * a row of its own so the walking dots can have the space instead.
     */
    {"legend.status", ValueCategory::Derived, ValueType::String, nullptr, kNoRegister,
     ValueSource::UiState, false, "Combined WiFi, MQTT and LED-pulse legend for the status page"},
    {"legend.warning", ValueCategory::Derived, ValueType::String, nullptr, kNoRegister,
     ValueSource::UiState, false, "Active sampling-warning summary"},
    /**
     * Where in the tree the operator is: `L2 3/8` — level 2, entry 3 of 8.
     *
     * The scrollbar shows the position within a level but says nothing about DEPTH, so three levels
     * into the sensor settings the panel looked identical to one level into the config root. The
     * title carries the crumb and this carries the coordinates.
     */
    /**
     * The unit the panel is currently showing flows in — `L/m`, `L/s` or `m3/h`.
     *
     * Bound by the header of every flow page, so the title follows the setting. Without it the unit
     * would be literal text on each of those screens: four second copies of `config.flowUnit`, and
     * the header would state a unit the rows were not using — the exact defect that made P1 render
     * L/s under an L/m title.
     */
    {"telemetry.flowUnitLabel", ValueCategory::Derived, ValueType::String, nullptr, kNoRegister,
     ValueSource::UiState, false, "Unit the panel is showing flows in"},
    {"nav.position", ValueCategory::Derived, ValueType::String, nullptr, kNoRegister,
     ValueSource::UiState, false, "Navigation depth and position within the current level"},
    {"page.title", ValueCategory::Derived, ValueType::String, nullptr, kNoRegister,
     ValueSource::UiState, false, "Title of the current page"},
    {"countdown.value", ValueCategory::Derived, ValueType::String, nullptr, kNoRegister,
     ValueSource::UiState, false, "Remaining seconds on the active countdown"},
    {"config.uartFrameSummary", ValueCategory::Derived, ValueType::String, nullptr, kNoRegister,
     ValueSource::UiState, false, "UART frame summary derived from parity and stop bits, e.g. 8E1"},
    {"config.sensor.nyquistWarning", ValueCategory::Derived, ValueType::String, nullptr,
     kNoRegister, ValueSource::UiState, true, "Nyquist validation prompt for the current sensor"},
    {"config.selectedSensor", ValueCategory::Derived, ValueType::Number, nullptr, kNoRegister,
     ValueSource::UiState, false,
     "Index (1-8) of the sensor implied by the current navigation level"},
    /**
     * The `adjust` term WITH its sign, for the formula line on the calibration screens.
     *
     * `F = 6*Q - 8` has to read as arithmetic, and `config.sensor.adjust` renders `-8` for a negative
     * but a bare `8` for a positive — which would print `F = 6*Q 8`. This renders `- 8` or `+ 8`, so
     * the row is a formula rather than two numbers next to each other.
     *
     * A separate id rather than a format change on `adjust`, because `adjust` is also the value the
     * S3 editor shows and steps, where a leading `+` would be noise.
     */
    {"config.sensor.adjustTerm", ValueCategory::Derived, ValueType::String, nullptr, kNoRegister,
     ValueSource::UiState, true, "The adjust term with its sign, for the formula line"},
    /**
     * The formula's Q range, e.g. `Q 0..150 L/m` — the variable the formula is in.
     *
     * Reads `--` on a channel calibrated by pulses per litre, because that form has no formula for
     * the term to belong to.
     */
    {"config.sensor.formulaQ", ValueCategory::Derived, ValueType::String, nullptr, kNoRegister,
     ValueSource::UiState, true, "The formula's Q range for the current sensor"},
    {"config.sensor.undersamplingFlag", ValueCategory::Derived, ValueType::Boolean, nullptr,
     kNoRegister, ValueSource::UiState, true,
     "Whether the current sensor failed its last Nyquist validation"},
    {"config.editor.pending", ValueCategory::Derived, ValueType::String, nullptr, kNoRegister,
     ValueSource::UiState, false, "The value currently being edited, before it is committed"},
    /**
     * The domain of the setting the current page is about, formatted for display (spec §7.2).
     *
     * Derived from the descriptor, never authored. Sixteen editors used to carry their range as
     * literal text — sixteen second copies of a fact `ui_settings_types.cpp` already holds, free to
     * drift, and one of them (the eight baud rates) overflowed the panel by 108 px.
     */
    {"config.editor.range", ValueCategory::Derived, ValueType::String, nullptr, kNoRegister,
     ValueSource::UiState, false, "Permitted range or option list of the setting on this page"}};

constexpr std::size_t kSensorMetricCount = sizeof(kSensorMetrics) / sizeof(kSensorMetrics[0]);
constexpr std::size_t kSimpleValueCount = sizeof(kSimpleValues) / sizeof(kSimpleValues[0]);

}  // namespace

std::size_t sensorMetricCount() { return kSensorMetricCount; }

const SensorMetric* sensorMetricAt(std::size_t index) {
  return index < kSensorMetricCount ? &kSensorMetrics[index] : nullptr;
}

std::size_t simpleValueCount() { return kSimpleValueCount; }

const SimpleValue* simpleValueAt(std::size_t index) {
  return index < kSimpleValueCount ? &kSimpleValues[index] : nullptr;
}

std::size_t catalogueSensorCount() { return plc::kNumSensors; }

uint16_t sensorMetricRegister(const SensorMetric& metric, std::size_t sensorNumber) {
  return static_cast<uint16_t>(plc::sensorBaseAddress(sensorNumber - 1) + metric.offset);
}

}  // namespace ui
