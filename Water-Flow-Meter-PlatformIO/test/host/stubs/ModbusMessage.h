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
// to exist.
//
// The accessors are INERT BY DEFAULT and say so at each one. This is not laziness dressed up: a stub
// that half-implements eModbus's big-endian variadic appender would look right, read as tested, and be
// wrong in a way nothing here could catch — the exact failure this repo keeps recording. An accessor
// that visibly answers zero cannot be mistaken for coverage.
//
// ── THE OPT-IN FIELD CARRIER, added for modbus_write_multiple_test.cpp ──────────────────
//
// One host test now DOES drive a handler, because FC16 refused every address in the network block and
// the requirement that names that case ("§5.1 requires a block write across the whole region to
// succeed") was tested one layer below, against NetRegisterMap::stageWrite directly. A green suite over
// a broken shipped path is the failure this project keeps rediscovering, so the frame handler had to
// become reachable.
//
// What `withFields` carries is NOT A WIRE FRAME and deliberately not a parse of one: it records the
// values a request answers AT THE BYTE INDICES THE HANDLER ITSELF READS (2 address, 4 value or word
// count, 6 byte count, 7 payload). What is under test is the validation and dispatch loop, not
// eModbus's parsing — so reimplementing the framing would add risk while testing nothing new, exactly
// as the paragraph above argues. A default-constructed message is unchanged: `present_` is false and
// every reader behaves as it always did, which is what keeps the other four binaries that include this
// header honest.
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

  /**
   * Builds a request that answers the four reads the frame handlers perform.
   *
   * `valueOrWords` is index 4, which handleWriteSingle reads as the VALUE and handleWriteMultiple reads
   * as the word count — one index with two meanings, as the wire format has it. `payload` is what
   * index 7 yields, big-endian pairs, which is the caller's job to lay out because that is the part of
   * the frame the test is making a statement about.
   */
  static ModbusMessage withFields(uint8_t serverId, uint8_t functionCode, uint16_t address,
                                  uint16_t valueOrWords, uint8_t byteCount,
                                  std::vector<uint8_t> payload = {}) {
    ModbusMessage m;
    m.present_ = true;
    m.serverId_ = serverId;
    m.functionCode_ = functionCode;
    m.address_ = address;
    m.valueOrWords_ = valueOrWords;
    m.byteCount_ = byteCount;
    m.payload_ = std::move(payload);
    return m;
  }

  // ── Readers. Inert unless the carrier was populated: with no fields present every reader leaves its
  // out-parameter alone and reports that it consumed nothing, so a test that never called withFields
  // reads the zero the caller initialised rather than a value this stub invented.
  uint8_t getServerID() const { return present_ ? serverId_ : 0; }
  uint8_t getFunctionCode() const { return present_ ? functionCode_ : 0; }

  template <typename T>
  std::size_t get(std::size_t index, T& target) const {
    if (!present_) {
      (void)index;
      (void)target;
      return 0;
    }
    // The three indices the handlers read, and nothing else answers.
    switch (index) {
      case 2: target = static_cast<T>(address_); return 2;
      case 4: target = static_cast<T>(valueOrWords_); return 2;
      case 6: target = static_cast<T>(byteCount_); return 1;
      default: return 0;
    }
  }
  std::size_t get(std::size_t index, std::vector<uint8_t>& target, std::size_t length) const {
    (void)index;
    // Sized before it is filled: handleWriteMultiple indexes this buffer, so it must not be shorter
    // than the byte count it was told about, or the stub would turn "no frame data" into an
    // out-of-bounds read.
    target.assign(length, 0);
    if (!present_) {
      return 0;
    }
    const std::size_t copied = length < payload_.size() ? length : payload_.size();
    for (std::size_t i = 0; i < copied; ++i) {
      target[i] = payload_[i];
    }
    return copied;
  }

  // ── Writers. The RESPONSE BYTES are still not assembled — that layout is eModbus's and no host test
  // verifies this stub against the real library. What is recorded is only whether the handler took the
  // error exit or the success exit, and with which exception code, which is the distinction every
  // assertion here is about.
  void setError(uint8_t serverId, uint8_t functionCode, uint8_t error) {
    (void)serverId;
    (void)functionCode;
    errorSet = true;
    errorCode = error;
  }
  template <typename... Args>
  void add(Args&&...) {
    addCalled = true;
  }

  bool errorSet = false;
  uint8_t errorCode = 0;
  bool addCalled = false;

 private:
  std::vector<uint8_t> data_;
  bool present_ = false;
  uint8_t serverId_ = 0;
  uint8_t functionCode_ = 0;
  uint16_t address_ = 0;
  uint16_t valueOrWords_ = 0;
  uint8_t byteCount_ = 0;
  std::vector<uint8_t> payload_;
};

// The unqualified spellings, which predate this file naming the Modbus namespace. Kept because removing
// them would be an unrelated change to whatever still uses them.
using Error = uint8_t;
constexpr Error SUCCESS = 0;
