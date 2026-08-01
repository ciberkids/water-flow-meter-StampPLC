#include "modbus/link_settings.h"

#include <cstdio>

namespace plc {

void LinkSettings::frameSummary(char* out, std::size_t size) const {
  if (!out || size == 0) {
    return;
  }
  const char parityChar = (parity == 1) ? 'E' : (parity == 2) ? 'O' : 'N';
  std::snprintf(out, size, "8%c%u", parityChar, static_cast<unsigned>(stopBits));
}

void LinkSettingsManager::begin(const LinkSettings& stored) {
  live_ = stored.valid() ? stored : LinkSettings{};
  staged_ = live_;
  previous_ = live_;
  revision_ = 0;
  awaitingConfirm_ = false;
  appliedAtMs_ = 0;
}

bool LinkSettingsManager::stage(uint16_t address, uint16_t value, WriteOrigin origin) {
  // The display commits on the spot with no rollback window, so it must commit only the
  // field the operator actually edited. Basing its candidate on `staged_` would let a
  // value a master staged and chose not to commit ride along on that unprotected apply.
  LinkSettings candidate = (origin == WriteOrigin::Display) ? live_ : staged_;
  switch (address) {
    case REG_LINK_SLAVE_ID:
      if (value < LinkSettings::kMinSlaveId || value > LinkSettings::kMaxSlaveId) {
        return false;
      }
      candidate.slaveId = static_cast<uint8_t>(value);
      break;
    case REG_LINK_BAUD_INDEX:
      if (value >= LinkSettings::kBaudCount) {
        return false;
      }
      candidate.baudIndex = static_cast<uint8_t>(value);
      break;
    case REG_LINK_PARITY:
      if (value > 2) {
        return false;
      }
      candidate.parity = static_cast<uint8_t>(value);
      break;
    case REG_LINK_STOP_BITS:
      if (value != 1 && value != 2) {
        return false;
      }
      candidate.stopBits = static_cast<uint8_t>(value);
      break;
    default:
      return false;
  }
  if (!candidate.valid()) {
    return false;
  }
  staged_ = candidate;
  return true;
}

bool LinkSettingsManager::apply(uint32_t nowMs, WriteOrigin origin) {
  if (!staged_.valid() || staged_ == live_) {
    // Nothing to do. Notably this does NOT arm the rollback window: arming it for a
    // no-op apply would revert settings that were never changed.
    staged_ = live_;
    return false;
  }
  previous_ = live_;
  live_ = staged_;
  revision_ = static_cast<uint16_t>(revision_ + 1);
  // §4.1.1: only a change that arrived over the bus is at risk of orphaning the device,
  // because only then is the bus the operator's sole means of control. A change made at
  // the display has no confirming frame to wait for, so waiting for one would revert
  // every local link change after 60 s.
  //
  // Assigning rather than only setting matters: a display change following an unconfirmed
  // bus apply must CLEAR the armed window, not leave it armed against a previous_ that has
  // since been overwritten.
  awaitingConfirm_ = (origin == WriteOrigin::Bus);
  appliedAtMs_ = nowMs;
  return true;
}

void LinkSettingsManager::noteValidFrame(uint32_t nowMs, uint8_t servedSlaveId) {
  (void)nowMs;
  // A valid frame ON THE NEW ID proves the master followed the change, which is the whole
  // question rollback exists to answer. A frame on any other ID proves nothing: if the
  // device is still answering on the pre-apply ID, that is the failure rollback is for,
  // not evidence against it.
  if (servedSlaveId != live_.slaveId) {
    return;
  }
  awaitingConfirm_ = false;
}

bool LinkSettingsManager::rollbackDue(uint32_t nowMs) const {
  return awaitingConfirm_ && (nowMs - appliedAtMs_) >= kRollbackWindowMs;
}

bool LinkSettingsManager::rollback() {
  if (!awaitingConfirm_ || previous_ == live_) {
    awaitingConfirm_ = false;
    return false;
  }
  live_ = previous_;
  staged_ = previous_;
  revision_ = static_cast<uint16_t>(revision_ + 1);
  awaitingConfirm_ = false;
  return true;
}

void LinkSettingsManager::publish(RegisterBank& registers) const {
  // Reads report the STAGED values, so a master can read back what it wrote before
  // committing. The live settings are observable from the fact that communication
  // still works, and from the revision counter.
  registers.setUint16(REG_LINK_SLAVE_ID, staged_.slaveId);
  registers.setUint16(REG_LINK_BAUD_INDEX, staged_.baudIndex);
  registers.setUint16(REG_LINK_PARITY, staged_.parity);
  registers.setUint16(REG_LINK_STOP_BITS, staged_.stopBits);
  registers.setUint16(REG_LINK_APPLY, 0);
  registers.setUint16(REG_LINK_REVISION, revision_);
}

}  // namespace plc
