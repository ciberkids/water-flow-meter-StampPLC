#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "modbus/register_map.h"

namespace plc {

/** The three commands §4.4.1 specifies, and nothing else. */
enum class MqttCommand : uint8_t {
  None = 0,
  ResetSession = 1,
  ResetTotals = 2,
  Republish = 3
};

/**
 * What happened to a command, as published on register 565 and the status topic (R4.4.2d).
 *
 * The values are wire values: an integrator and a Home Assistant template both read them, so they are
 * append-only in the same sense as the value catalogue (I2).
 */
enum class MqttCommandResult : uint8_t {
  /** Nothing has arrived since boot. Distinct from Accepted so a fresh device does not claim a success. */
  Idle = 0,
  Accepted = 1,
  RateLimited = 2,
  RetainedIgnored = 3,
  BadPayload = 4,
  /** A topic under `<base>/cmd/` that names no command. Reported rather than dropped: a typo in an
   *  automation is otherwise invisible to the person who made it. */
  UnknownCommand = 5
};

const char* mqttCommandResultText(MqttCommandResult result);

/**
 * Decides whether an inbound MQTT command is acted on — §4.4.1 and R4.4.2, and nothing about MQTT itself.
 *
 * Arduino-free and host-tested, like `NtpPolicy` and for the same reason: every rule below is a decision
 * about safety on a metering device, and none of them is observable on a bench without a broker, a clock and
 * an hour. What this file does NOT do is subscribe, parse frames or perform resets — the adapter does that,
 * and R4.4.3 sends the reset through the same Modbus register a master would write.
 *
 * ── THE OWNER'S PRINCIPLE, WHICH SHAPES EVERY BRANCH ─────────────────────────────────────
 *
 * *A reset failing remotely is not a breaking thing; the device entering a reset loop is.* So every
 * ambiguous case here fails towards swallowing the command. That is why an unparsable payload, a retained
 * message and a too-soon repeat are all rejections rather than best-effort attempts.
 *
 * ── WHY BOTH A MILLIS GUARD AND A PERSISTED ONE ──────────────────────────────────────────
 *
 * `millis()` cannot jump (R4.4.2a): an NTP sync can move the wall clock backwards mid-operation and would
 * silently re-arm a wall-clock limit — and this device now syncs NTP (R7.13), so that is a live hazard
 * rather than a theoretical one. But `millis()` restarts at zero on reboot, and the loop that matters most
 * is the one that reboots: reset, crash, reconnect, redelivered command, reset. So the epoch of the last
 * ACCEPTED command is persisted too, and both guards must pass.
 *
 * **The persisted guard is only as good as the clock.** With no trusted time it cannot fire, and that limit
 * is stated rather than hidden: a device with a dead RTC and no network relies on the `millis()` guard alone,
 * which still covers every loop that does not reboot. The three routes that set the clock (registers 50-55,
 * the portal, NTP) all exist now, so the degraded case is a device nobody has commissioned.
 */
class MqttCommandRouter {
 public:
  /**
   * Per-kind minimum gap between accepted commands — R4.4.2a's `kResetMinIntervalMs`.
   *
   * Sixty seconds: far longer than any plausible loop, far shorter than any legitimate repeat. An operator
   * pressing a dashboard button twice in a minute means it the first time.
   */
  static constexpr uint32_t kResetMinIntervalMs = 60u * 1000u;

  /** The same interval in seconds, for the persisted wall-clock guard. */
  static constexpr uint32_t kResetMinIntervalS = 60u;

  /** The exact payload a destructive command requires (R4.4.1) — the `0x5AA5` idiom in ASCII. */
  static constexpr const char* kResetPayload = "RESET";

  /**
   * Which command a topic names, or `None`.
   *
   * Matches `<base>/cmd/<name>` exactly. A topic that starts with the base but names no known command
   * returns `Unknown` through `evaluate`, not silence — see `MqttCommandResult::UnknownCommand`.
   */
  static MqttCommand commandFor(const char* topic, const char* baseTopic, bool* underCmdPrefix = nullptr) {
    if (underCmdPrefix != nullptr) {
      *underCmdPrefix = false;
    }
    if (topic == nullptr || baseTopic == nullptr) {
      return MqttCommand::None;
    }
    const std::size_t baseLen = std::strlen(baseTopic);
    if (baseLen == 0 || std::strncmp(topic, baseTopic, baseLen) != 0) {
      return MqttCommand::None;
    }
    const char* rest = topic + baseLen;
    static constexpr const char* kCmd = "/cmd/";
    const std::size_t cmdLen = std::strlen(kCmd);
    if (std::strncmp(rest, kCmd, cmdLen) != 0) {
      return MqttCommand::None;
    }
    if (underCmdPrefix != nullptr) {
      *underCmdPrefix = true;
    }
    const char* name = rest + cmdLen;
    if (std::strcmp(name, "reset-session") == 0) return MqttCommand::ResetSession;
    if (std::strcmp(name, "reset-totals") == 0) return MqttCommand::ResetTotals;
    if (std::strcmp(name, "republish") == 0) return MqttCommand::Republish;
    return MqttCommand::None;
  }

  /**
   * The holding register a master would write to perform this command — R4.4.3 — or 0.
   *
   * Here rather than in the adapter so the mapping is host-tested: it is the half of R4.4.3 that can
   * be wrong silently. `reset-totals` → `REG_MASTER_RESET_ALL_MEASURED` is an INFERENCE from
   * `register_map.h`'s "`REG_MASTER_RESET_ALL_MEASURED` takes the lifetime total with it" and not a
   * mapping §4.4.1 spells out; it is written down here so the next reader checks the register rather
   * than trusting the topic name.
   *
   * `republish` has no register because it destroys nothing and touches no measurement — it re-sends
   * discovery, which is not a thing a Modbus master can ask for.
   *
   * The value to write is **1**, not `0x5AA5`: registers 20-23 predate that idiom and
   * `applyHoldingWrite` acts on `value == 1`. Two magics for one concept would be worse than the
   * inconsistency.
   */
  static uint16_t registerFor(MqttCommand command) {
    switch (command) {
      case MqttCommand::ResetSession:
        return REG_MASTER_RESET_ALL_SESSION;
      case MqttCommand::ResetTotals:
        return REG_MASTER_RESET_ALL_MEASURED;
      case MqttCommand::Republish:
      case MqttCommand::None:
        return 0;
    }
    return 0;
  }

  /** The value that register takes. Not `0x5AA5` — see `registerFor`. */
  static constexpr uint16_t kRegisterValue = 1;

  /** True for the two commands that destroy measurements, and therefore carry both safeguards. */
  static bool isDestructive(MqttCommand command) {
    return command == MqttCommand::ResetSession || command == MqttCommand::ResetTotals;
  }

  /**
   * Boot-time seed from NVS: the epoch at which this kind was last accepted, or 0 if never (R4.4.2b).
   *
   * Only ACCEPTED commands are persisted, so a looping command does not loop NVS writes — the same
   * discipline as the menu-pack boot counter (`Loadable_UI_Menu_Packs` §3.6).
   */
  void seedLastAcceptedEpoch(MqttCommand command, uint32_t epoch) {
    const std::size_t slot = slotFor(command);
    if (slot < kSlots) {
      persistedEpoch_[slot] = epoch;
    }
  }

  uint32_t lastAcceptedEpoch(MqttCommand command) const {
    const std::size_t slot = slotFor(command);
    return slot < kSlots ? persistedEpoch_[slot] : 0u;
  }

  /**
   * Decides, and records an acceptance so the caller cannot forget to.
   *
   * `nowEpoch` is 0 when the clock is unset, which disables the persisted guard for this call — stated in
   * the class comment rather than silently treated as "1970, therefore long ago".
   */
  MqttCommandResult evaluate(MqttCommand command,
                             const char* payload,
                             bool retained,
                             uint32_t nowMs,
                             uint32_t nowEpoch) {
    if (command == MqttCommand::None) {
      return MqttCommandResult::UnknownCommand;
    }
    // R4.4.2c, and FIRST: a retained command is a fault at the broker, not a request. Checked before the
    // payload so a retained `RESET` cannot be mistaken for a valid one, and before the rate limit so the
    // operator is told to clear it rather than being told to wait a minute forever.
    if (retained) {
      return MqttCommandResult::RetainedIgnored;
    }
    if (isDestructive(command)) {
      if (payload == nullptr || std::strcmp(payload, kResetPayload) != 0) {
        return MqttCommandResult::BadPayload;
      }
      const std::size_t slot = slotFor(command);
      if (slot >= kSlots) {
        return MqttCommandResult::UnknownCommand;
      }
      if (accepted_[slot] && static_cast<int32_t>(nowMs - (lastAcceptedMs_[slot] + kResetMinIntervalMs)) < 0) {
        return MqttCommandResult::RateLimited;
      }
      if (nowEpoch != 0 && persistedEpoch_[slot] != 0 && nowEpoch >= persistedEpoch_[slot] &&
          (nowEpoch - persistedEpoch_[slot]) < kResetMinIntervalS) {
        return MqttCommandResult::RateLimited;
      }
      accepted_[slot] = true;
      lastAcceptedMs_[slot] = nowMs;
      persistedEpoch_[slot] = nowEpoch;
      return MqttCommandResult::Accepted;
    }
    // `republish` destroys nothing, so it needs no magic payload and no rate limit — the worst a loop can
    // do is make the device chatty, and R4.4.4 wants the telemetry to be the acknowledgement anyway.
    return MqttCommandResult::Accepted;
  }

 private:
  static constexpr std::size_t kSlots = 2;  // the two destructive kinds

  static std::size_t slotFor(MqttCommand command) {
    switch (command) {
      case MqttCommand::ResetSession:
        return 0;
      case MqttCommand::ResetTotals:
        return 1;
      default:
        return kSlots;
    }
  }

  bool accepted_[kSlots] = {false, false};
  uint32_t lastAcceptedMs_[kSlots] = {0, 0};
  uint32_t persistedEpoch_[kSlots] = {0, 0};
};

}  // namespace plc
