#pragma once

// Host stub for the eModbus message type.
//
// modbus_manager.h names ModbusMessage in signatures but the host link does not include
// modbus_manager.cpp, so an opaque-enough stand-in suffices. Deliberately NOT the real
// eModbus header: test/host/run.sh promises no PlatformIO and no container, and borrowing
// TUs out of .pio/libdeps would work locally and break the CI job that has neither.
#include <cstdint>
#include <vector>

class ModbusMessage {
 public:
  ModbusMessage() = default;
  explicit ModbusMessage(std::size_t n) : data_(n) {}
  std::size_t size() const { return data_.size(); }
  const uint8_t* data() const { return data_.data(); }
  uint8_t operator[](std::size_t i) const { return data_[i]; }

 private:
  std::vector<uint8_t> data_;
};

using Error = uint8_t;
constexpr Error SUCCESS = 0;
