#pragma once

// Host stub for the eModbus message type.
//
// Deliberately NOT the real eModbus header: test/host/run.sh promises no PlatformIO and no container,
// and borrowing TUs out of .pio/libdeps would work locally and break the CI job that has neither.
//
// ── WHAT THIS IS AND IS NOT ─────────────────────────────────────────────────────────────
//
// It was an opaque stand-in while no host test linked modbus_manager.cpp — the type appeared only in
// signatures. modbus_manager_clock_test.cpp links that file, so ModbusManager::handleReadHolding,
// handleWriteSingle and handleWriteMultiple now have to COMPILE, which means every member they call has
// to exist. They still do not have to WORK: no host test drives a Modbus frame, because a frame is the
// one part of this class that eModbus itself parses.
//
// So the accessors below are INERT ON PURPOSE and say so at each one. This is not laziness dressed up:
// a stub that half-implements eModbus's big-endian variadic appender would look right, read as tested,
// and be wrong in a way nothing here could catch — the exact failure this repo keeps recording. An
// accessor that visibly answers zero cannot be mistaken for coverage. If a frame-level test is ever
// wanted, these bodies are where it starts, and the assertions must come with them.
#include <Arduino.h>

#include <cstddef>
#include <cstdint>
#include <vector>

/** The error codes modbus_manager.cpp names. Values match the Modbus spec's exception codes. */
namespace Modbus {
enum Error : uint8_t {
  SUCCESS = 0x00,
  ILLEGAL_FUNCTION = 0x01,
  ILLEGAL_DATA_ADDRESS = 0x02,
  ILLEGAL_DATA_VALUE = 0x03,
  SERVER_DEVICE_FAILURE = 0x04,
};
}  // namespace Modbus

class ModbusMessage {
 public:
  ModbusMessage() = default;
  explicit ModbusMessage(std::size_t n) : data_(n) {}
  std::size_t size() const { return data_.size(); }
  const uint8_t* data() const { return data_.data(); }
  uint8_t operator[](std::size_t i) const { return data_[i]; }

  // ── Inert readers. A real request carries a parsed RTU frame; this one carries nothing, so every
  // reader leaves its out-parameter alone and reports that it consumed nothing. A test that grew to
  // depend on these would read the zero the caller initialised, not a value this stub invented.
  uint8_t getServerID() const { return 0; }
  uint8_t getFunctionCode() const { return 0; }

  template <typename T>
  std::size_t get(std::size_t index, T& target) const {
    (void)index;
    (void)target;
    return 0;
  }
  std::size_t get(std::size_t index, std::vector<uint8_t>& target, std::size_t length) const {
    (void)index;
    // Sized but not filled: handleWriteMultiple indexes this buffer, so it must not be shorter than the
    // byte count it was told about, or the stub would turn "no frame data" into an out-of-bounds read.
    target.assign(length, 0);
    return 0;
  }

  // ── Inert writers. Recording the bytes would invite an assertion on a layout no host test verifies
  // against the real library, so nothing is stored.
  void setError(uint8_t serverId, uint8_t functionCode, uint8_t error) {
    (void)serverId;
    (void)functionCode;
    (void)error;
  }
  template <typename... Args>
  void add(Args&&...) {}

 private:
  std::vector<uint8_t> data_;
};

// The unqualified spellings, which predate this file naming the Modbus namespace. Kept because removing
// them would be an unrelated change to whatever still uses them.
using Error = uint8_t;
constexpr Error SUCCESS = 0;
