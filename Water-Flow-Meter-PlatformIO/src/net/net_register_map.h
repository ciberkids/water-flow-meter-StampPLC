#pragma once

#include <cstddef>
#include <cstdint>

#include "modbus/register_map.h"  // kHoldingRegisterSpace — the bank must cover this block
#include "net/net_settings.h"

namespace plc {

/**
 * Declared, not included: only `publishStatus`'s DEFINITION needs the members, so `net_status.h`
 * stays out of this header's include graph — and with it `wifi_manager.h`, which `net_status.h`
 * needs for `netStatusFrom` and which nothing else here has any business pulling in.
 */
struct NetStatusSnapshot;

/**
 * The network holding-register block (WiFi_MQTT_Connectivity.md §5).
 *
 * Placed at 500, leaving 420-499 free so the sensor count can grow — the sensor blocks currently
 * end at 419 (`SENSOR_1_BASE_ADDR` 100 + 40 x 8).
 *
 * Strings are packed **two characters per register, high byte first**, NUL-padded. That convention
 * is stated here and implemented once, because it is the kind of detail two implementations get
 * subtly different: this header is the only definition, and `net_register_map.cpp` is the only
 * code that acts on it.
 */
namespace net_reg {

inline constexpr uint16_t kBase = 500;

// ── WiFi ──────────────────────────────────────────────────────────────────────────
inline constexpr uint16_t kWifiEnabled   = 500;
inline constexpr uint16_t kWifiState     = 501;  // read-only
inline constexpr uint16_t kWifiRssi      = 502;  // read-only, int16
inline constexpr uint16_t kWifiIp        = 503;  // read-only, 2 registers
inline constexpr uint16_t kWifiMac       = 505;  // read-only, 3 registers
inline constexpr uint16_t kWifiSsid      = 510;  // 16 registers, 32 bytes
inline constexpr uint16_t kWifiPsk       = 526;  // 32 registers, 64 bytes, write-only

// ── MQTT ──────────────────────────────────────────────────────────────────────────
inline constexpr uint16_t kMqttEnabled       = 560;
inline constexpr uint16_t kMqttState         = 561;  // read-only
inline constexpr uint16_t kMqttPort          = 562;
inline constexpr uint16_t kMqttPeriodS       = 563;
// bit 0 HA discovery, bit 1 QoS. Bit 2 is RESERVED — it briefly carried a TLS toggle before
// Q3/R8.3 was honoured (TLS is out of scope; a toggle that does nothing implies protection that is
// not there). Left unused rather than reassigned, so a master written against the interim build
// cannot silently mean something new.
//
// Booleans packed into one register rather than one register each, because a master that wants to
// turn the whole MQTT client on in one shot should not need a round trip per flag. The cost is that
// a read-modify-write is mandatory: writing this register sets EVERY flag, so a caller changing one
// bit must preserve the rest. `NetRegisterMap::mqttFlags()` exists so the UI has one place to get
// the current word from rather than reconstructing the bit layout at each call site.
inline constexpr uint16_t kMqttFlags         = 564;
inline constexpr uint16_t kMqttLastCmdResult = 565;  // read-only, R4.4.2d
inline constexpr uint16_t kMqttHost          = 570;  // 32 registers, 64 bytes
inline constexpr uint16_t kMqttUser          = 602;  // 16 registers, 32 bytes
inline constexpr uint16_t kMqttPassword      = 618;  // 16 registers, write-only
inline constexpr uint16_t kMqttBaseTopic     = 634;  // 24 registers, 48 bytes
inline constexpr uint16_t kMqttPrefix        = 658;  // 16 registers, 32 bytes

// ── Portal and AP (§5.2, remote setup) ────────────────────────────────────────────
inline constexpr uint16_t kPortalRemainingS = 675;  // read-only
inline constexpr uint16_t kApSsid           = 676;  // 16 registers, read-only
inline constexpr uint16_t kApPassword       = 692;  // 16 registers, read-only — R5.3
inline constexpr uint16_t kApIp             = 708;  // 2 registers, read-only
// Write kApplyMagic here to restore the portal login to admin/admin (R8.2a).
//
// Acts IMMEDIATELY rather than staging, unlike every value register in this block. A recovery
// action that needs a second write to take effect is a recovery action somebody gets half-way
// through — and the whole point is that it works when the operator is locked out and improvising.
//
// This adds no exposure a master did not already have: kPortalPassword is WRITABLE, so anyone who
// can reach this register could already set the login to a value of their choosing. The command
// exists for discoverability, and because "reset to a known default" is a different intention from
// "set to this string" and deserves to be expressible.
inline constexpr uint16_t kPortalReset      = 710;
inline constexpr uint16_t kPortalUser       = 712;  // 8 registers, 16 bytes

/**
 * 720-729 — RESERVED, and deliberately not reused.
 *
 * kPortalPassword lived at 720 and claimed 16 registers for its 32 bytes. It did not have them: kApply
 * sits at 730, so the field's declared span ran straight through the apply protocol and past the end of
 * the block. The consequence was a WRITE WINDOW OF 20 BYTES — registers 720-729 staged bytes 0..19, 730
 * diverted to the apply handler (so a master aiming at password bytes 20/21 could COMMIT the block
 * instead of writing to it), and 731/732 were silently ignored as read-only. The store, the portal and
 * `netFieldCapacity` all said 32 bytes, and the portal really does accept 32, so a 32-byte password
 * could be set from the web form and never over RS485 — against the standing principle that RS485 is
 * the source of truth for everything.
 *
 * The field moved above the apply protocol rather than the apply protocol moving, because 730/731/732
 * are the three addresses WiFi_MQTT_Connectivity.md §5 tabulates and a master may already be written
 * against them. This window is left empty instead: a master built against the old prose writes here and
 * is ignored, which is what §5.1 says a non-writable address in the block does, rather than landing in
 * the middle of some other field.
 */
inline constexpr uint16_t kPortalPasswordReserved = 720;  // 10 registers, formerly kPortalPassword

// ── Apply protocol, mirroring REG_LINK_APPLY ──────────────────────────────────────
inline constexpr uint16_t kApply      = 730;
inline constexpr uint16_t kRevision   = 731;  // read-only
inline constexpr uint16_t kLastError  = 732;  // read-only

// 733-735 left free so the apply protocol can gain a word without moving a text field again.
inline constexpr uint16_t kPortalPassword   = 736;  // 16 registers, 32 bytes, write-only
inline constexpr uint16_t kEnd        = 752;  // one past the last register

// A master can only read what RegisterBank will answer for. Extending this block past the bank would
// make the new registers return ILLEGAL_DATA_ADDRESS — the exact silent-unreachability that left the
// whole block dead until now — so the two are reconciled at compile time.
static_assert(kEnd <= plc::kHoldingRegisterSpace,
              "the network block must fit inside the holding-register space RegisterBank serves");

/** The magic that commits a staged block — the same value REG_LINK_APPLY uses. */
inline constexpr uint16_t kApplyMagic = 0x5AA5;

/** Registers a text field of `bytes` occupies: two characters each, rounded up. */
constexpr uint16_t textRegisters(std::size_t bytes) {
  return static_cast<uint16_t>((bytes + 1) / 2);
}

/** Where each text field starts, so the mapping lives in one place. */
constexpr uint16_t textBase(NetField field) {
  switch (field) {
    case NetField::WifiSsid:            return kWifiSsid;
    case NetField::WifiPsk:             return kWifiPsk;
    case NetField::MqttHost:            return kMqttHost;
    case NetField::MqttUser:            return kMqttUser;
    case NetField::MqttPassword:        return kMqttPassword;
    case NetField::MqttBaseTopic:       return kMqttBaseTopic;
    case NetField::MqttDiscoveryPrefix: return kMqttPrefix;
    case NetField::PortalUser:          return kPortalUser;
    case NetField::PortalPassword:      return kPortalPassword;
    case NetField::Count:               return 0;
  }
  return 0;
}

/**
 * THE LAYOUT CHECKS. A text field that overlaps something is now a BUILD FAILURE.
 *
 * kPortalPassword overlapped kApply, kRevision and kLastError and ran past kEnd, and every one of the
 * three places that stated its span — this header, `netFieldCapacity`, and the wiki generator — agreed
 * with each other while disagreeing with the arithmetic. Nothing computed the sum. The audit that found
 * it had to check all ten fields by hand and reported "the layout has zero slack anywhere", which is
 * exactly the kind of fact that should be the compiler's job: this block is hand-allocated, it is full,
 * and the next field added to it will be placed by somebody reading a table.
 *
 * A wrong ADDRESS still compiles, of course — this cannot know where a field was meant to go. What it
 * refuses is the silent part: a field that overlaps another field, overlaps a scalar or fixed range, or
 * runs off the end of the block.
 */
namespace layout_check {

struct Range {
  uint16_t start;
  uint16_t count;
};

/** Where one text field lands, from the same two functions the packing code uses. */
constexpr Range textRange(NetField field) {
  return Range{textBase(field), textRegisters(netFieldCapacity(field))};
}

constexpr bool overlaps(Range a, Range b) {
  return a.start < b.start + b.count && b.start < a.start + a.count;
}

/**
 * Everything in the block that is NOT a NetField text field: the scalars, the read-only AP strings the
 * device fills in itself, and the reserved window the portal password vacated.
 */
constexpr Range kFixed[] = {
    {kWifiEnabled, 1},   {kWifiState, 1},        {kWifiRssi, 1},    {kWifiIp, 2},
    {kWifiMac, 3},       {kMqttEnabled, 1},      {kMqttState, 1},   {kMqttPort, 1},
    {kMqttPeriodS, 1},   {kMqttFlags, 1},        {kMqttLastCmdResult, 1},
    {kPortalRemainingS, 1}, {kApSsid, 16},       {kApPassword, 16}, {kApIp, 2},
    {kPortalReset, 1},   {kPortalPasswordReserved, 10},
    {kApply, 1},         {kRevision, 1},         {kLastError, 1}};

constexpr bool everyTextFieldIsInsideTheBlock() {
  for (std::size_t i = 0; i < static_cast<std::size_t>(NetField::Count); ++i) {
    const Range r = textRange(static_cast<NetField>(i));
    if (r.start < kBase || r.start + r.count > kEnd) {
      return false;
    }
  }
  return true;
}

constexpr bool noTextFieldOverlapsAnother() {
  for (std::size_t i = 0; i < static_cast<std::size_t>(NetField::Count); ++i) {
    for (std::size_t j = i + 1; j < static_cast<std::size_t>(NetField::Count); ++j) {
      if (overlaps(textRange(static_cast<NetField>(i)), textRange(static_cast<NetField>(j)))) {
        return false;
      }
    }
  }
  return true;
}

constexpr bool noTextFieldOverlapsAFixedRegister() {
  for (std::size_t i = 0; i < static_cast<std::size_t>(NetField::Count); ++i) {
    const Range field = textRange(static_cast<NetField>(i));
    for (const Range& fixed : kFixed) {
      if (overlaps(field, fixed)) {
        return false;
      }
    }
  }
  return true;
}

static_assert(everyTextFieldIsInsideTheBlock(),
              "a text field's registers run outside the network block — its capacity in "
              "netFieldCapacity() needs as many registers as textBase() gives it");
static_assert(noTextFieldOverlapsAnother(),
              "two text fields share registers, so writing one would corrupt the other");
static_assert(noTextFieldOverlapsAFixedRegister(),
              "a text field overlaps a scalar, an AP string or the reserved window — this is the "
              "kPortalPassword/kApply collision, which cost 12 bytes of a 32-byte credential");

}  // namespace layout_check

}  // namespace net_reg

/** Why the last apply failed, reported at `kLastError`. */
enum class NetApplyError : uint16_t {
  None = 0,
  NothingStaged,
  BadMagic,
  InvalidValue
};

/**
 * Translates the register block to and from NetSettings.
 *
 * Separate from NetSettings so the packing convention is testable without the settings semantics,
 * and so a change to one cannot quietly alter the other.
 */
class NetRegisterMap {
 public:
  /** True for an address inside the block. */
  static bool contains(uint16_t address) {
    return address >= net_reg::kBase && address < net_reg::kEnd;
  }

  /**
   * True when a master may WRITE this address.
   *
   * A read-only address is not an error to write — §5.1 requires a block write across the region to
   * succeed rather than except — it is simply ignored.
   */
  static bool isWritable(uint16_t address);

  /**
   * True when reading this address must return zeros regardless of what is stored (§5.1).
   *
   * The secrets. Note the deliberate asymmetry of R5.3: the AP password is NOT here, because it
   * describes an access point the device is broadcasting rather than an operator secret it was
   * given.
   */
  static bool readsAsZero(uint16_t address);

  /** Stages one register write. False when the address is not writable or the value is refused. */
  static bool stageWrite(NetSettings& settings, uint16_t address, uint16_t value);

  /**
   * Handles a write to `kApply`. Returns the resulting error, `None` on success.
   *
   * Split from stageWrite so the caller can tell "staged" from "committed" without inspecting the
   * address itself.
   */
  static NetApplyError applyWrite(NetSettings& settings, uint16_t value);

  /** Packs the live settings into `out`, which must span the whole block. */
  static void publish(const NetSettings& settings, uint16_t* out, std::size_t count);

  /**
   * Packs the LIVE STATUS into the same block, and must run AFTER `publish`.
   *
   * A second function rather than an argument to `publish` for two reasons: `publish` is called from
   * tests that have no snapshot to hand, and additive means no existing caller changes. It writes
   * only the eight read-only fields that describe what the device is doing right now — 501, 502,
   * 503-504, 505-507, 675, 676-691, 692-707, 708-709 — and touches nothing `publish` owns.
   *
   * Every one of those read 0 forever until this existed, because `publish` zeroes the whole block
   * and packs only settings. The cost was not a missing convenience: `WiFi.md` documents the Modbus
   * route as "the fully remote path, and currently the reliable one" and ends it with three
   * verification reads, of which only `kRevision` worked. A master could not tell a successful
   * association from a refused apply.
   *
   * Byte and word order are the published contract in `gen-registers.mjs`, not a local choice:
   * `ipv4` is "2 registers, high word first", `mac` is "3 registers, 2 bytes each, high byte first",
   * and text is "2 characters per register, high byte first, NUL-padded".
   */
  static void publishStatus(const NetStatusSnapshot& status, uint16_t* out, std::size_t count);

  /**
   * The current `kMqttFlags` word, assembled from the LIVE settings.
   *
   * The read half of the read-modify-write that register 564 forces on anyone changing one of its
   * three booleans. Exposed rather than left to callers because reconstructing the bit layout at
   * each call site is how bit 2 would eventually get cleared by a write meant only for bit 0 — the
   * shared-register bug this method exists to make unlikely.
   */
  static uint16_t mqttFlags(const NetSettings& settings) {
    return static_cast<uint16_t>((settings.mqttHaDiscovery() ? kFlagHaDiscovery : 0u) |
                                 (settings.mqttQos() != 0 ? kFlagQos1 : 0u));
  }

  /**
   * The same word assembled from the STAGED settings — the correct read half of the RMW.
   *
   * `mqttFlags` above serves `publish()`, which must report what is in force. A caller about to
   * modify one bit needs the other bits as they will be after the next apply, not as they are now:
   * rebuilding from live would drop any flag a Modbus master had staged and not yet committed,
   * which is the shared-register bug one level up from the one `mqttFlags` prevents.
   */
  static uint16_t mqttFlagsStaged(const NetSettings& settings) {
    return static_cast<uint16_t>((settings.stagedMqttHaDiscovery() ? kFlagHaDiscovery : 0u) |
                                 (settings.stagedMqttQos() != 0 ? kFlagQos1 : 0u));
  }

  /** Bit masks within `kMqttFlags`, so no caller writes the literals. */
  static constexpr uint16_t kFlagHaDiscovery = 0x01u;
  static constexpr uint16_t kFlagQos1        = 0x02u;
  /** Reserved, formerly TLS. See the note on kMqttFlags — deliberately not reused. */
  static constexpr uint16_t kFlagReservedBit2 = 0x04u;

  /** Two characters per register, high byte first. */
  static uint16_t packChars(char high, char low) {
    return static_cast<uint16_t>((static_cast<uint16_t>(static_cast<uint8_t>(high)) << 8) |
                                 static_cast<uint8_t>(low));
  }
};

}  // namespace plc
