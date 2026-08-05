#pragma once

#include <cstddef>
#include <cstdint>

namespace plc {

inline constexpr std::size_t kNumSensors = 8;
inline constexpr uint8_t kDefaultModbusSlaveId = 1;

// Global register addresses
inline constexpr uint16_t REG_POLLING_RATE_KHZ = 0;
inline constexpr uint16_t REG_CONNECTED_SENSORS_BITMAP = 10;
inline constexpr uint16_t REG_MASTER_RESET_ALL_SENSORS = 20;
inline constexpr uint16_t REG_MASTER_RESET_ALL_MEASURED = 21;
inline constexpr uint16_t REG_MASTER_RESET_ALL_SESSION = 22;
inline constexpr uint16_t REG_UNDERSAMPLING_FLAGS = 30;
inline constexpr uint16_t REG_LED_RED_VOLUME_STEP = 31;
inline constexpr uint16_t REG_LED_RED_PULSE_PERIOD = 32;

// Serial link configuration block — Project_document.md §4.1.1.
// Writes to 40-43 are staged; register 44 commits them.
/**
 * One past the highest holding register the device serves.
 *
 * Must cover the network block at 500-732 (WiFi_MQTT_Connectivity.md §5), not just the sensor blocks
 * that end at 419. RegisterBank sized itself from the sensor blocks alone, so every address in the
 * network block failed isRangeValid() and a master reading NET_WIFI_STATE got ILLEGAL_DATA_ADDRESS —
 * honest, but it made §5's entire remote-setup story unreachable.
 *
 * net_register_map.h static_asserts its own kEnd against this, so growing the network block without
 * growing the bank is a build failure rather than a range that silently stops answering.
 */
inline constexpr uint16_t kHoldingRegisterSpace = 733;

inline constexpr uint16_t REG_LINK_SLAVE_ID = 40;
inline constexpr uint16_t REG_LINK_BAUD_INDEX = 41;
inline constexpr uint16_t REG_LINK_PARITY = 42;
inline constexpr uint16_t REG_LINK_STOP_BITS = 43;
inline constexpr uint16_t REG_LINK_APPLY = 44;
inline constexpr uint16_t REG_LINK_REVISION = 45;

// Sensor register layout
inline constexpr uint16_t SENSOR_BLOCK_SIZE = 40;
inline constexpr uint16_t SENSOR_1_BASE_ADDR = 100;

inline constexpr uint16_t OFF_STATUS_FLAGS = 0;
inline constexpr uint16_t OFF_INSTANT_FLOW = 1;
inline constexpr uint16_t OFF_CUMULATIVE_LITERS = 3;
inline constexpr uint16_t OFF_CUMULATIVE_M3 = 7;
inline constexpr uint16_t OFF_SESSION_LITERS = 11;
inline constexpr uint16_t OFF_SESSION_M3 = 13;
inline constexpr uint16_t OFF_MAX_FLOW = 15;
inline constexpr uint16_t OFF_CMD_RESET_SESSION = 17;
inline constexpr uint16_t OFF_CMD_RESET_ALL = 18;
inline constexpr uint16_t OFF_CMD_RESET_CONFIG = 19;
inline constexpr uint16_t OFF_CFG_Q_MAX = 20;
inline constexpr uint16_t OFF_CFG_F_MULT = 21;
inline constexpr uint16_t OFF_CFG_ADJUST = 22;

inline constexpr uint16_t sensorBaseAddress(std::size_t index) {
  return SENSOR_1_BASE_ADDR + static_cast<uint16_t>(index) * SENSOR_BLOCK_SIZE;
}

}  // namespace plc

