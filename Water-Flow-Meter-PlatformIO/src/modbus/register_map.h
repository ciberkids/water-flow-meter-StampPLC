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
/**
 * Write 1 to clear every channel's peak flow, and nothing else.
 *
 * The cheapest reset in the system, and the only one that destroys nothing: the peak is volatile — it is
 * never written to NVS, so a power cycle already clears it. That is exactly why it deserves its own
 * command rather than being reachable only as a side effect of the two that do destroy something.
 * `REG_MASTER_RESET_ALL_SESSION` clears the peak along with the session volume, and
 * `REG_MASTER_RESET_ALL_MEASURED` takes the lifetime total with it, so before this an operator who
 * wanted to clear a spike after fixing the pipe that caused it had to give up real data to do it.
 *
 * Appended at 23, leaving 24-29 free. The three older master resets at 20-22 keep their addresses.
 */
inline constexpr uint16_t REG_MASTER_RESET_ALL_MAX = 23;
inline constexpr uint16_t REG_UNDERSAMPLING_FLAGS = 30;
inline constexpr uint16_t REG_LED_RED_VOLUME_STEP = 31;
inline constexpr uint16_t REG_LED_RED_PULSE_PERIOD = 32;
/**
 * Which unit the PANEL shows flows in: 0 = L/min, 1 = L/s, 2 = m3/h.
 *
 * A DISPLAY preference, and nothing more. Storage stays L/min (§2a) and every other surface is
 * unaffected: registers 101 and 115 keep publishing L/min, so does MQTT, so does Home Assistant, and
 * the calibration settings keep the datasheet's unit. A Modbus master must never shift by sixty
 * because somebody changed what the screen shows.
 *
 * It gets a register anyway, so an integrator can read what the panel is displaying — useful when a
 * support call describes a number that does not match the wire.
 */
inline constexpr uint16_t REG_DISPLAY_FLOW_UNIT = 33;

// Serial link configuration block — Project_document.md §4.1.1.
// Writes to 40-43 are staged; register 44 commits them.
/**
 * One past the highest holding register the device serves.
 *
 * Must cover the network block at 500-751 (WiFi_MQTT_Connectivity.md §5), not just the sensor blocks
 * that end at 419. RegisterBank sized itself from the sensor blocks alone, so every address in the
 * network block failed isRangeValid() and a master reading NET_WIFI_STATE got ILLEGAL_DATA_ADDRESS —
 * honest, but it made §5's entire remote-setup story unreachable.
 *
 * net_register_map.h static_asserts its own kEnd against this, so growing the network block without
 * growing the bank is a build failure rather than a range that silently stops answering.
 *
 * Grew from 733 to 752 when kPortalPassword was moved above the apply protocol: its 16 registers had
 * been overlapping kApply/kRevision/kLastError, which left only 20 of its 32 bytes writable over
 * RS485. The 19 extra registers cost 38 bytes of RAM.
 */
inline constexpr uint16_t kHoldingRegisterSpace = 752;

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
/**
 * How this channel is calibrated, and the pulses-per-litre figure when that is how.
 *
 * Meters are specified one of two ways. Some print a formula — `F = 6*Q - 8, Q = L/min +/-5%` — which
 * is what OFF_CFG_F_MULT and OFF_CFG_ADJUST already hold. Others print a single pulses-per-litre
 * number, and that is the more common form.
 *
 * The existing fields CANNOT express the second. `f_multiplier` is an int16_t used as a divisor, so a
 * meter rated 450 pulses/L needs 7.5 and there is no way to store it: the nearest integers are 7 and
 * 8, a 6% error on every reading. Pulses per litre therefore gets its own register at its own scale,
 * where 450 is exactly 450.
 *
 * ADDITIVE. Offsets 20-22 keep their meaning and their units, so no Modbus master that already reads
 * a formula-calibrated channel sees any change. `SENSOR_BLOCK_SIZE` is 40 and only 22 offsets were in
 * use, so this needed no block resize either.
 */
inline constexpr uint16_t OFF_CFG_CAL_TYPE = 23;
inline constexpr uint16_t OFF_CFG_PULSES_PER_L = 24;
/**
 * Write `1` to return THIS channel's calibration to defaults, keeping every measurement it has taken.
 *
 * Exists because OFF_CMD_RESET_CONFIG (19) does not do this, despite its name. Nineteen assigns
 * `SensorData{}` over the whole runtime struct — cumulative litres, session litres, the peak and the
 * pulse count — and then calls `saveCumulativeToNvs`, so the zeroed lifetime total is persisted too.
 * It is a config AND measurement wipe. That is the right command for decommissioning a channel and the
 * wrong one for the case this register serves: a broken meter swapped for one with different
 * characteristics, where the volume the old meter measured was real and has to keep accumulating.
 *
 * Nineteen is left exactly as it is. Any Modbus master already issuing it expects the wipe it performs,
 * and narrowing a shipped command's effect silently is worse than adding a second one that says what it
 * does.
 *
 * Nor could the panel achieve this by writing zeros to offsets 20-24: `prepareConfigUpdate` refuses a
 * candidate that fails `configIsValid` when the channel currently holds one that passes, and q_max = 0
 * fails it by definition. Returning a channel to "not set" is therefore only expressible as a command,
 * not as a value write. (It accepts an invalid candidate on a channel that was ALREADY invalid, which is
 * how a field-by-field entry reaches a complete configuration at all — see the reasoning there. That
 * direction cannot demolish anything, because there is nothing valid left to demolish.)
 *
 * ADDITIVE, like 23-24 before it. Offsets 0-24 were in use and `SENSOR_BLOCK_SIZE` is 40, so this
 * needed no block resize and no existing offset moved.
 */
inline constexpr uint16_t OFF_CMD_RESET_CALIBRATION = 25;

inline constexpr uint16_t sensorBaseAddress(std::size_t index) {
  return SENSOR_1_BASE_ADDR + static_cast<uint16_t>(index) * SENSOR_BLOCK_SIZE;
}

}  // namespace plc

