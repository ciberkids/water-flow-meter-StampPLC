#pragma once

#include <cstddef>
#include <cstdint>

#include "modbus/link_limits.h"
#include "modbus/register_bank.h"

namespace plc {

/**
 * RS485 link parameters — Project_document.md §4.1.1, registers 40-47.
 *
 * Deliberately free of any Arduino dependency so the validation, staging and
 * rollback logic can be exercised on the host. The one Arduino-shaped concern,
 * translating parity/stop bits into a `SERIAL_8N1`-style constant, lives in
 * `arduinoSerialConfig()` in link_settings_arduino.h instead.
 */
struct LinkSettings {
  /** 1-247. Address 0 is broadcast and 248-255 are reserved by the Modbus spec. */
  uint8_t slaveId = kDefaultModbusSlaveId;
  /** Index into kBaudRates. 115200 does not fit a uint16, hence an index. */
  uint8_t baudIndex = 3;  // 9600
  uint8_t parity = 0;     // 0 None, 1 Even, 2 Odd
  uint8_t stopBits = 1;   // 1 or 2

  static constexpr auto& kBaudRates = LinkLimits::kBaudRates;
  static constexpr uint8_t kBaudCount = LinkLimits::kBaudCount;
  static constexpr uint8_t kMinSlaveId = LinkLimits::kMinSlaveId;
  static constexpr uint8_t kMaxSlaveId = LinkLimits::kMaxSlaveId;

  uint32_t baudRate() const {
    return kBaudRates[baudIndex < kBaudCount ? baudIndex : 3];
  }

  bool valid() const {
    return slaveId >= kMinSlaveId && slaveId <= kMaxSlaveId && baudIndex < kBaudCount &&
           parity <= 2 && (stopBits == 1 || stopBits == 2);
  }

  /** Writes the UART frame summary, e.g. "8E1", for config.uartFrameSummary. */
  void frameSummary(char* out, std::size_t size) const;

  bool operator==(const LinkSettings& other) const {
    return slaveId == other.slaveId && baudIndex == other.baudIndex &&
           parity == other.parity && stopBits == other.stopBits;
  }
  bool operator!=(const LinkSettings& other) const { return !(*this == other); }
};

/**
 * Owns the live and staged link settings and the apply/rollback protocol.
 *
 * Writes to registers 40-43 are **staged**, never applied. The register that
 * describes the transport cannot be changed by the request travelling over it: a
 * master that set baud=19200 would receive its reply at the old rate and then lose
 * the link. Committing is a separate, deliberate write of 0x5AA5 to register 44.
 *
 * Rollback exists because that commit can still be wrong. If no valid Modbus frame
 * arrives within kRollbackWindowMs of an apply, the previous settings are restored —
 * otherwise one bad write orphans the device permanently, with no way back except a
 * physical factory reset.
 */
class LinkSettingsManager {
 public:
  static constexpr uint16_t kApplyMagic = 0x5AA5;
  static constexpr uint32_t kRollbackWindowMs = 60000;

  void begin(const LinkSettings& stored);

  const LinkSettings& live() const { return live_; }
  const LinkSettings& staged() const { return staged_; }
  uint16_t revision() const { return revision_; }

  /** Stages a write to 40-43. False if the address or value is not acceptable. */
  bool stage(uint16_t address, uint16_t value);

  /**
   * Commits the staged values. Returns true when the live settings changed, which
   * is the caller's cue to persist and restart the UART.
   */
  bool apply(uint32_t nowMs);

  /** Called on every valid Modbus frame; confirms an apply did not break the link. */
  void noteValidFrame(uint32_t nowMs);

  /** True when an apply has gone unconfirmed for longer than the window. */
  bool rollbackDue(uint32_t nowMs) const;

  /** Restores the pre-apply settings. Returns true if anything changed. */
  bool rollback();

  /** True while an apply is waiting for its first confirming frame. */
  bool awaitingConfirmation() const { return awaitingConfirm_; }

  /** Mirrors 40-43 and 45 into the register bank so masters can read them. */
  void publish(RegisterBank& registers) const;

 private:
  LinkSettings live_{};
  LinkSettings staged_{};
  LinkSettings previous_{};
  uint16_t revision_ = 0;
  bool awaitingConfirm_ = false;
  uint32_t appliedAtMs_ = 0;
};

}  // namespace plc
