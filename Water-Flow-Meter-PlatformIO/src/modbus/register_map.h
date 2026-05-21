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

