#pragma once

#include <array>
#include <cstdint>
#include <cstring>

#include "modbus/register_map.h"

namespace plc {

class RegisterBank {
 public:
  static constexpr uint16_t kTotalRegisters =
      SENSOR_1_BASE_ADDR + SENSOR_BLOCK_SIZE * static_cast<uint16_t>(kNumSensors);

  RegisterBank() { data_.fill(0); }

  bool isRangeValid(uint16_t address, uint16_t count = 1) const {
    return (address + count) <= kTotalRegisters;
  }

  void setUint16(uint16_t address, uint16_t value) {
    if (address < kTotalRegisters) {
      data_[address] = value;
    }
  }

  void setFloat(uint16_t address, float value) {
    if (!isRangeValid(address, 2)) {
      return;
    }
    uint32_t raw = 0;
    std::memcpy(&raw, &value, sizeof(float));
    data_[address] = static_cast<uint16_t>((raw >> 16) & 0xFFFF);
    data_[address + 1] = static_cast<uint16_t>(raw & 0xFFFF);
  }

  void setDouble(uint16_t address, double value) {
    if (!isRangeValid(address, 4)) {
      return;
    }
    uint64_t raw = 0;
    std::memcpy(&raw, &value, sizeof(double));
    data_[address] = static_cast<uint16_t>((raw >> 48) & 0xFFFF);
    data_[address + 1] = static_cast<uint16_t>((raw >> 32) & 0xFFFF);
    data_[address + 2] = static_cast<uint16_t>((raw >> 16) & 0xFFFF);
    data_[address + 3] = static_cast<uint16_t>(raw & 0xFFFF);
  }

  uint16_t at(uint16_t address) const { return data_[address]; }

  uint16_t* raw() { return data_.data(); }
  const uint16_t* raw() const { return data_.data(); }

  void fill(uint16_t value) { data_.fill(value); }

 private:
  std::array<uint16_t, kTotalRegisters> data_;
};

}  // namespace plc
