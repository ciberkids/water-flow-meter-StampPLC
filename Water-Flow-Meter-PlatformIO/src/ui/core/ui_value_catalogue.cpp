#include "ui/core/ui_value_catalogue.h"

#include "modbus/register_map.h"

namespace ui {

namespace {

using namespace plc;  // register offsets from modbus/register_map.h

/**
 * Offsets come from register_map.h rather than being repeated as literals, so a change to
 * the register layout moves the manifest with it.
 */
constexpr SensorMetric kSensorMetrics[] = {
    {"instantFlow", ValueCategory::Reading, ValueType::Number, "L/s", OFF_INSTANT_FLOW,
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
    {"maxFlowSinceReset", ValueCategory::Accumulated, ValueType::Number, "L/s", OFF_MAX_FLOW,
     "peak flow since last session reset"}};

constexpr SimpleValue kSimpleValues[] = {
    {"telemetry.totalFlowLps", ValueCategory::System, ValueType::Number, "L/s", kNoRegister,
     ValueSource::Telemetry, false, "Aggregate instantaneous flow across ready sensors"},
    {"telemetry.totalVolumeLiters", ValueCategory::System, ValueType::Number, "L", kNoRegister,
     ValueSource::Telemetry, false, "Aggregate session volume across ready sensors"},
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
    {"legend.warning", ValueCategory::Derived, ValueType::String, nullptr, kNoRegister,
     ValueSource::UiState, false, "Active sampling-warning summary"},
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
    {"config.sensor.undersamplingFlag", ValueCategory::Derived, ValueType::Boolean, nullptr,
     kNoRegister, ValueSource::UiState, true,
     "Whether the current sensor failed its last Nyquist validation"},
    {"config.editor.pending", ValueCategory::Derived, ValueType::String, nullptr, kNoRegister,
     ValueSource::UiState, false, "The value currently being edited, before it is committed"}};

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
