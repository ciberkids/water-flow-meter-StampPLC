#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace plc {

/**
 * Whether this device is still keeping what it was told to keep — DF25.
 *
 * Every write to non-volatile storage in this firmware discarded its result. Arduino's
 * `Preferences::put*` return the number of bytes written and 0 on failure, and `begin()` returns whether
 * the store opened at all, and none of it was looked at. So a device whose store had failed kept
 * publishing correct live flow, session volume and lifetime totals from RAM, on Modbus and MQTT and the
 * panel, and only lost the lifetime totals at the next power cycle — the one number Home Assistant
 * records per channel and the one anyone would bill from. That is the fourth instance of one shape in
 * this register (`DF22`, `DF23`, `DF24`): a value with a home, a writer and no error path.
 *
 * WHAT COUNTS AS FAILING, decided 2026-08-30: **N consecutive failures on the same key**, N = 3. A
 * single refused write can succeed on the next pass once the store compacts, so one failure is not news;
 * three passes of the same key failing is. The litre writer runs once a minute, so the alarm means
 * roughly "three minutes of not keeping your totals" — long enough to ride out a compaction, short
 * enough that an operator standing at the device sees it. Any success on a key clears that key's run.
 *
 * RAM ONLY, and that is a requirement rather than a shortcut: a store that cannot be written cannot
 * record its own failure. The panel and the bus say it from RAM, and the count is rebuilt from the next
 * failure after a reboot. It is also the never-cache rule — the answer is derived from write outcomes,
 * not stored beside them, which is what the retired `SensorData::isReady` bug was about.
 *
 * ARDUINO-FREE and header-only, for the reason `sensor_config_nvs.h` and `mqtt_snapshot.h` both give:
 * `firmware.cpp` is in no link set in `test/host/run.sh`, and neither is `net_settings_nvs.h` (its own
 * header says so). Detection logic placed in any of them would be logic no test can reach, which is how
 * a serializer shipped writing three of five fields.
 */

/**
 * The storage fault code space — ONE sequence, `0` healthy, and the number is what the panel WILL show.
 *
 * One sequence rather than one per subsystem (DF25's question 3): the operator will see a single
 * warning triangle, so the CODE is the only thing that can disambiguate which part of the device
 * stopped persisting. (The triangle itself is NOT built yet — it rides with N-e's T5. Today the code
 * reaches a reader on Modbus register 34 and on the MQTT diagnostics topic only.) A per-subsystem space would need the triangle to say which space it came from first.
 *
 * NOT an extension of `NetApplyError`. That space is scoped to the network apply protocol on register
 * 732 and its 0-3 are spoken for; and the keys that can fail here span calibration, topology, litres,
 * the connected bitmap, the link settings, the network settings, the command epochs and the pack
 * counter, so filing a device-wide fact under one subsystem's address would be wrong twice.
 *
 * **The numbers are a wire contract from the moment this ships**, like every other code space here:
 * append only, never renumber, never reuse. `1`-`31` belong to storage; `32` upward is reserved so the
 * next subsystem to earn a place on the triangle cannot renumber these.
 *
 * WHAT A CODE CAN AND CANNOT SAY. Arduino's `Preferences` logs the underlying `esp_err_t` and drops it
 * inside framework code this project must not edit, so a code can only ever name WHICH GROUP OF KEYS
 * stopped persisting — never why. "Partition full" and "corrupt page" are indistinguishable from here.
 * That ceiling is stated so nobody designs a wiki page promising a cause.
 */
enum class StorageFault : std::uint16_t {
  None = 0,
  /** `begin()` returned false: nothing is stored and nothing is read — every get returns its default. */
  StoreDidNotOpen = 1,
  /** `cml_0`…`cml_7` — the lifetime volumes. The costliest loss, and the only one written on a timer. */
  CumulativeLitres = 2,
  /** `cfg_q/f/a/c/p` per channel. A channel returns as `SET?` after a power cycle. */
  SensorCalibration = 3,
  /** `parent_0`…`parent_7` — the cascade topology (`N-e`). */
  Topology = 4,
  /** `conn_map` — which channels are in service. */
  ConnectedBitmap = 5,
  /** `lnk_id`, `lnk_baud`, `lnk_par`, `lnk_stop`. Includes §4.1.1's unattended rollback write. */
  LinkSettings = 6,
  /** `flow_unit` — the panel's unit. */
  FlowUnit = 7,
  /** The red LED's volume step and pulse period. */
  LedSettings = 8,
  /** The nine `n_*` strings and five scalars. Partial failure here mixes old and new credentials. */
  NetworkSettings = 9,
  /** `cmd_ep_ses` / `cmd_ep_tot` — R4.4.2b's reboot-surviving command rate limit. */
  CommandEpoch = 10,
  /** The menu-pack attempt counter. */
  PackAttemptCounter = 11,
  /** `clear()` during a factory reset. A failure here means the reset was a no-op that looked done. */
  FactoryResetErase = 12,
};

/** One past the last code in use. Append here; never renumber what is above. */
inline constexpr std::uint16_t kStorageFaultCount = 13;
/** The band reserved to storage. `32` upward belongs to whatever joins the triangle next. */
inline constexpr std::uint16_t kStorageFaultBandEnd = 32;
static_assert(static_cast<std::uint16_t>(StorageFault::FactoryResetErase) + 1 == kStorageFaultCount,
              "kStorageFaultCount must be one past the last enumerator: append the new code to the enum, "
              "bump this count, and give it a description in tools/wiki/gen-registers.mjs");
static_assert(kStorageFaultCount <= kStorageFaultBandEnd,
              "the storage code sequence has outgrown its reserved band; widen the band deliberately "
              "rather than borrowing the next subsystem's numbers");

/**
 * Is this group written PERIODICALLY, so that counting consecutive failures means anything?
 *
 * This is the correction to a real hole found by review on 2026-08-30. N = 3 consecutive failures was
 * applied to all twelve groups, but only four of them have a writer that comes back on its own. For the
 * rest — an operator sets a baud rate once, a master writes the flow unit once — there is no next pass,
 * so a single failed write parked a count of 1 that nothing would ever raise. The failure was recorded
 * and unreportable, which is DF25's own shape.
 *
 * So the cadence is a property of the WRITER and it lives here, next to the policy it changes, rather
 * than at twelve call sites that would each have to remember it.
 *
 * A one-shot failure raises on the FIRST failure, and the argument is the same one `begin()` and
 * `clear()` already win: the operator has just done something that did not stick, and there is nothing
 * left to retry it. The cost is honest and worth stating — a transient failure on a one-shot write shows
 * the fault until the next successful write of that group, which for a setting nobody touches again
 * could be a long time. A per-group retry queue would distinguish the two; it is the richer answer and
 * it is not built.
 */
inline constexpr bool storageFaultIsPeriodic(StorageFault fault) {
  switch (fault) {
    case StorageFault::CumulativeLitres:
    case StorageFault::SensorCalibration:
    case StorageFault::ConnectedBitmap:
      // Written by the once-a-minute pass in firmware.cpp, whose shadow copies advance only on success —
      // so a failing key is re-attempted every minute and reaches three in three minutes.
      return true;
    case StorageFault::NetworkSettings:
      // Retried by the same pass since the review: the revision shadow advances only on success, and a
      // failed save is re-attempted at most once a minute.
      return true;
    case StorageFault::None:
    case StorageFault::StoreDidNotOpen:
    case StorageFault::Topology:
    case StorageFault::LinkSettings:
    case StorageFault::FlowUnit:
    case StorageFault::LedSettings:
    case StorageFault::CommandEpoch:
    case StorageFault::PackAttemptCounter:
    case StorageFault::FactoryResetErase:
      return false;
  }
  return false;
}

/**
 * Did a `put*` land? — `returned == expected`, not `returned != 0`.
 *
 * Every `Preferences::put*` this project uses returns a FIXED byte count on success: 1 for `putUChar`
 * and `putBool`, 2 for `putShort`/`putUShort`, 4 for `putUInt`, 8 for `putDouble`. Comparing against the
 * expected count rather than against zero costs nothing and catches a short write, which is as much a
 * failure as a refused one.
 */
inline bool nvsPutOk(std::size_t returned, std::size_t expected) { return returned == expected; }

/**
 * Did a `putString` land? — and this one is a TRAP, which is why it has its own function.
 *
 * `Preferences::putString` returns `strlen(value)`, not a fixed width. So a successful write of an empty
 * string returns 0 — indistinguishable from failure by the usual test. That path is not theoretical:
 * `net_settings_nvs.h` writes all nine text fields on every save, and an unset MQTT password or portal
 * user is an empty string on a perfectly healthy device. Testing `returned != 0` there would put a
 * warning triangle on every device that has no MQTT password, which is most of them.
 */
inline bool nvsPutStringOk(std::size_t returned, const char* value) {
  return returned == (value == nullptr ? 0u : std::strlen(value));
}

/**
 * The per-key consecutive-failure counter and the alarm it raises.
 *
 * Keyed by fault code rather than by key STRING on purpose: a string map would put an allocation and a
 * comparison on the logic loop, and the thing the panel needs is the group, not the key. `cml_3` failing
 * and `cml_5` failing are one fact to an operator — the litres are not being kept.
 */
class NvsWriteHealth {
 public:
  /** N, from the 2026-08-30 decision. One line to change if bench experience says otherwise. */
  static constexpr std::uint8_t kConsecutiveFailuresBeforeAlarm = 3;

  /**
   * Raise a group to the alarm immediately, for a failure that NOTHING WILL RETRY.
   *
   * Two conditions earn this and they are both one-shot: `begin()` failing means the store is not open
   * for the whole of this boot, and a factory reset's `clear()` failing means the erase that was
   * supposed to happen did not. Waiting for three consecutive failures would mean waiting for three
   * events that cannot occur — the counting policy assumes a writer that comes back once a minute.
   *
   * Deliberately NOT a general escape hatch. Every periodic writer goes through `noteResult`, because
   * "one refused write is not news" is the whole content of the 2026-08-30 decision.
   */
  void raiseNow(StorageFault fault) {
    const std::size_t index = static_cast<std::size_t>(fault);
    if (index == 0 || index >= kStorageFaultCount) {
      return;
    }
    consecutive_[index] = kConsecutiveFailuresBeforeAlarm;
  }

  /**
   * Record one write outcome. A success CLEARS that key's run; nothing else does.
   *
   * `StorageFault::None` is not a key and is ignored, so a caller that has nothing to report cannot
   * accidentally clear somebody else's run.
   */
  void noteResult(StorageFault fault, bool ok) {
    const std::size_t index = static_cast<std::size_t>(fault);
    if (index == 0 || index >= kStorageFaultCount) {
      return;
    }
    if (ok) {
      consecutive_[index] = 0;
      return;
    }
    if (!storageFaultIsPeriodic(fault)) {
      // Nothing will retry this one, so counting to three would be counting events that cannot happen.
      // See `storageFaultIsPeriodic` for why this is decided by the group rather than by the caller.
      raiseNow(fault);
      return;
    }
    if (consecutive_[index] < 255) {  // saturate rather than wrap: 256 failures must not read as zero
      ++consecutive_[index];
    }
  }

  /** How many times in a row this group has failed. Zero after any success. */
  std::uint8_t consecutiveFailures(StorageFault fault) const {
    const std::size_t index = static_cast<std::size_t>(fault);
    return index == 0 || index >= kStorageFaultCount ? 0 : consecutive_[index];
  }

  bool inAlarm(StorageFault fault) const {
    return consecutiveFailures(fault) >= kConsecutiveFailuresBeforeAlarm;
  }

  /** Is anything in alarm — the panel's "should the triangle be up" question. */
  bool alarmRaised() const { return faultCode() != StorageFault::None; }

  /**
   * The code to publish: the LOWEST-NUMBERED group in alarm, or `None`.
   *
   * Lowest-numbered rather than most-recent or worst-count, because a code that changes while the fault
   * persists is a code an operator cannot write down and look up. The order the enum is written in is
   * therefore a priority order, and `StoreDidNotOpen` is first deliberately — when the store never
   * opened, every other group is failing too and reporting one of them would be a symptom, not the
   * cause.
   */
  StorageFault faultCode() const {
    for (std::size_t i = 1; i < kStorageFaultCount; ++i) {
      if (consecutive_[i] >= kConsecutiveFailuresBeforeAlarm) {
        return static_cast<StorageFault>(i);
      }
    }
    return StorageFault::None;
  }

  /** The same answer as a register value. */
  std::uint16_t code() const { return static_cast<std::uint16_t>(faultCode()); }

  /** How many groups are in alarm at once. For the diagnostics payload, not for the panel. */
  std::uint8_t alarmCount() const {
    std::uint8_t count = 0;
    for (std::size_t i = 1; i < kStorageFaultCount; ++i) {
      if (consecutive_[i] >= kConsecutiveFailuresBeforeAlarm) {
        ++count;
      }
    }
    return count;
  }

 private:
  std::uint8_t consecutive_[kStorageFaultCount] = {};
};

}  // namespace plc
