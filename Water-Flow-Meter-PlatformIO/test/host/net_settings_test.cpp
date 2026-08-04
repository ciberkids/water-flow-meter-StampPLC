// Host tests for the network settings store and its register packing.
//
// This is the storage half of SettingKind::Text, and it is exercised the way a Modbus master
// actually behaves: registers written in arbitrary order, block writes across read-only regions,
// partial fields, and applies that should be refused. None of that is convenient to provoke on a
// bench, and all of it is what a master will do on its first afternoon.
#include "net/net_register_map.h"
#include "net/net_settings.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const char* what) {
  ++checks;
  std::printf("  %-70s %s\n", what, condition ? "ok" : "FAIL");
  if (!condition) ++failures;
}

void checkStr(const char* actual, const char* expected, const char* what) {
  const bool same = std::strcmp(actual, expected) == 0;
  ++checks;
  std::printf("  %-70s %s\n", what, same ? "ok" : "FAIL");
  if (!same) {
    std::printf("      expected \"%s\"\n      actual   \"%s\"\n", expected, actual);
    ++failures;
  }
}

using plc::NetApplyError;
using plc::NetField;
using plc::NetRegisterMap;
using plc::NetSettings;

std::string liveOf(const NetSettings& s, NetField f) {
  char buffer[NetSettings::kMaxValueBytes + 1] = {};
  s.get(f, buffer, sizeof(buffer));
  return buffer;
}

std::string stagedOf(const NetSettings& s, NetField f) {
  char buffer[NetSettings::kMaxValueBytes + 1] = {};
  s.getStaged(f, buffer, sizeof(buffer));
  return buffer;
}

/** Writes a whole string through the register block, as a master would. */
void writeText(NetSettings& s, NetField field, const std::string& value) {
  const uint16_t base = plc::net_reg::textBase(field);
  const uint16_t span = plc::net_reg::textRegisters(plc::netFieldCapacity(field));
  for (uint16_t r = 0; r < span; ++r) {
    const std::size_t at = static_cast<std::size_t>(r) * 2;
    const char high = at < value.size() ? value[at] : '\0';
    const char low = at + 1 < value.size() ? value[at + 1] : '\0';
    NetRegisterMap::stageWrite(s, static_cast<uint16_t>(base + r),
                               NetRegisterMap::packChars(high, low));
  }
}

void defaultsTests() {
  std::printf("[defaults — §7.9a wants a known login, not an empty one]\n");

  NetSettings s;
  checkStr(liveOf(s, NetField::PortalUser).c_str(), "admin", "the portal user defaults to admin");
  checkStr(liveOf(s, NetField::PortalPassword).c_str(), "admin", "and so does the password");
  check(s.portalPasswordIsDefault(), "which the device knows, so it can nag about it (§7.9a)");
  checkStr(liveOf(s, NetField::MqttDiscoveryPrefix).c_str(), "homeassistant",
           "the HA discovery prefix has the conventional default");
  check(!s.wifiEnabled() && !s.mqttEnabled(),
        "both radios are off — nothing switches itself on (R3.1.1)");
  check(s.mqttPort() == 1883, "the MQTT port defaults to 1883");
  check(!s.wifiConfigured() && !s.mqttConfigured(), "and nothing is configured yet");
}

void stagingTests() {
  std::printf("\n[staging — nothing takes effect until apply]\n");

  NetSettings s;
  check(s.stage(NetField::WifiSsid, "MyNetwork"), "an SSID stages");
  check(s.dirty(), "which shows as an unsaved edit");
  check(liveOf(s, NetField::WifiSsid).empty(),
        "and is NOT live — a half-written config must never be observed");
  check(!s.wifiConfigured(), "so the configured guard is still false");

  check(s.apply(), "apply commits it");
  checkStr(liveOf(s, NetField::WifiSsid).c_str(), "MyNetwork", "and the value is live");
  check(s.wifiConfigured(), "the configured guard flips (R7.12)");
  check(s.revision() == 1, "the revision bumped, so a master can tell");
  check(!s.dirty(), "and nothing is pending");

  check(!s.apply(), "a second apply with nothing staged returns false, not silently true");
  check(s.revision() == 1, "and does not bump the revision");

  s.stage(NetField::WifiSsid, "Discarded");
  s.revert();
  checkStr(liveOf(s, NetField::WifiSsid).c_str(), "MyNetwork", "revert restores the live value");
  check(!s.dirty(), "and clears the pending state");
}

void validationTests() {
  std::printf("\n[refusing values that cannot work]\n");

  NetSettings s;
  check(!s.stageMqttPort(0), "port 0 is refused rather than stored");
  check(!s.stageMqttPublishPeriodS(0), "a zero publish period is refused");
  check(!s.stageMqttPublishPeriodS(3601), "and one over an hour");
  check(s.stageMqttPublishPeriodS(3600), "3600 s exactly is allowed");
  check(!s.stageMqttQos(2), "QoS 2 is refused — §4.2 offers 0 and 1 and 2 is not implemented");
  check(s.stageMqttQos(1), "QoS 1 is allowed");

  // Truncation rather than rejection: a master writing a full block sends trailing padding.
  const std::string tooLong(200, 'x');
  check(s.stage(NetField::WifiSsid, tooLong.c_str()), "an over-long SSID is accepted");
  s.apply();
  check(liveOf(s, NetField::WifiSsid).size() == plc::netFieldCapacity(NetField::WifiSsid),
        "and truncated to the field's capacity, not rejected");
}

void registerRoundTripTests() {
  std::printf("\n[the register block, driven as a master drives it]\n");

  NetSettings s;
  writeText(s, NetField::MqttHost, "homeassistant.local");
  check(NetRegisterMap::applyWrite(s, plc::net_reg::kApplyMagic) == NetApplyError::None,
        "a full text field written register by register applies");
  checkStr(liveOf(s, NetField::MqttHost).c_str(), "homeassistant.local",
           "and round-trips exactly");

  // Odd length: the last register carries one character and a NUL.
  NetSettings odd;
  writeText(odd, NetField::MqttUser, "bob");
  odd.apply();
  checkStr(liveOf(odd, NetField::MqttUser).c_str(), "bob",
           "an odd-length string survives the two-chars-per-register packing");

  // ORDER MUST NOT MATTER. A master may write high registers first, or only the ones that changed.
  NetSettings reversed;
  {
    const std::string value = "reverse.example.com";
    const uint16_t base = plc::net_reg::textBase(NetField::MqttHost);
    const uint16_t span = plc::net_reg::textRegisters(plc::netFieldCapacity(NetField::MqttHost));
    for (int r = static_cast<int>(span) - 1; r >= 0; --r) {
      const std::size_t at = static_cast<std::size_t>(r) * 2;
      const char high = at < value.size() ? value[at] : '\0';
      const char low = at + 1 < value.size() ? value[at + 1] : '\0';
      NetRegisterMap::stageWrite(reversed, static_cast<uint16_t>(base + r),
                                 NetRegisterMap::packChars(high, low));
    }
    reversed.apply();
  }
  checkStr(liveOf(reversed, NetField::MqttHost).c_str(), "reverse.example.com",
           "registers written in REVERSE order produce the same string");

  // Numerics and flags.
  NetSettings nums;
  NetRegisterMap::stageWrite(nums, plc::net_reg::kMqttPort, 8883);
  NetRegisterMap::stageWrite(nums, plc::net_reg::kWifiEnabled, 1);
  NetRegisterMap::stageWrite(nums, plc::net_reg::kMqttFlags, 0x03);
  nums.apply();
  check(nums.mqttPort() == 8883, "a port written over Modbus lands");
  check(nums.wifiEnabled(), "so does the enable flag");
  check(nums.mqttHaDiscovery() && nums.mqttQos() == 1, "and the flags bitfield unpacks");
}

void applyProtocolTests() {
  std::printf("\n[the apply magic is what stops a stray write committing]\n");

  NetSettings s;
  s.stage(NetField::WifiSsid, "Staged");
  check(NetRegisterMap::applyWrite(s, 1) == NetApplyError::BadMagic,
        "any value but 0x5AA5 is refused");
  check(liveOf(s, NetField::WifiSsid).empty(), "so the staged value stays staged");
  check(NetRegisterMap::applyWrite(s, 0x5AA5) == NetApplyError::None, "the magic commits");
  check(NetRegisterMap::applyWrite(s, 0x5AA5) == NetApplyError::NothingStaged,
        "and applying nothing reports NothingStaged rather than success");

  check(!NetRegisterMap::stageWrite(s, plc::net_reg::kApply, 0x5AA5),
        "stageWrite refuses the apply register — committing is a separate operation");
}

void secrecyTests() {
  std::printf("\n[what the device will and will not hand back — §5.1, R5.3]\n");

  NetSettings s;
  writeText(s, NetField::WifiPsk, "hunter2hunter2");
  writeText(s, NetField::WifiSsid, "MyNetwork");
  s.apply();

  // Stored, because the radio needs it.
  checkStr(liveOf(s, NetField::WifiPsk).c_str(), "hunter2hunter2",
           "the PSK is stored in full — the radio needs it");

  // But never published.
  std::vector<uint16_t> block(plc::net_reg::kEnd - plc::net_reg::kBase, 0xFFFF);
  NetRegisterMap::publish(s, block.data(), block.size());

  const auto readAt = [&](uint16_t address) {
    return block[address - plc::net_reg::kBase];
  };
  bool pskAllZero = true;
  for (uint16_t r = 0; r < plc::net_reg::textRegisters(plc::netFieldCapacity(NetField::WifiPsk)); ++r) {
    if (readAt(static_cast<uint16_t>(plc::net_reg::kWifiPsk + r)) != 0) pskAllZero = false;
  }
  check(pskAllZero, "but reads back as zeros over Modbus, every register of it");
  check(readAt(plc::net_reg::kWifiSsid) != 0, "while the SSID reads back normally");

  check(NetRegisterMap::readsAsZero(plc::net_reg::kWifiPsk), "the PSK is marked secret");
  check(NetRegisterMap::readsAsZero(plc::net_reg::kMqttPassword), "so is the MQTT password");
  check(NetRegisterMap::readsAsZero(plc::net_reg::kPortalPassword), "and the portal login");
  // R5.3's deliberate asymmetry — this is the one that would look like a bug without the reasoning.
  check(!NetRegisterMap::readsAsZero(plc::net_reg::kApPassword),
        "the AP password is NOT secret: the device broadcasts that network, and a remote operator "
        "needs it (R5.3)");
}

void blockWriteTests() {
  std::printf("\n[a block write across read-only registers must not fail — §5.1]\n");

  NetSettings s;
  std::size_t refused = 0;
  // Sweep the entire block, as a master doing one big write-multiple-registers would.
  for (uint16_t address = plc::net_reg::kBase; address < plc::net_reg::kEnd; ++address) {
    if (address == plc::net_reg::kApply) continue;  // committing is deliberate, not incidental
    if (!NetRegisterMap::stageWrite(s, address, 0x4141)) {
      ++refused;
    }
  }
  std::printf("      %zu of %u addresses declined the write\n", refused,
              static_cast<unsigned>(plc::net_reg::kEnd - plc::net_reg::kBase - 1));
  check(refused > 0, "read-only and unassigned addresses decline");
  // The point of §5.1: declining is not excepting. Nothing above should have crashed or corrupted
  // the store, and an apply should still work.
  check(s.dirty(), "the writable addresses still staged");
  check(s.apply(), "and the block still applies afterwards");
}

// ────────────────────────────────────────────────────────────────────────────────────
// One base-topic validator.
//
// HaDiscovery and MqttPublisher each carried a rule for the same operator-supplied field, and the
// rules disagreed. A topic one accepts and the other refuses yields ZERO Home Assistant entities
// while the MQTT state register still reads connected: nothing logged at either end, and it presents
// as a broker fault. Every refusal below names the divergence it came from, with the two tests that
// used to assert opposite answers about it.
//
// Breaks if: the charset widens back to include 0x20 or 0x7f, the leading/trailing-slash rule is
// dropped, `//` stops counting as an empty level, or the capacity bound moves in either direction.
// ────────────────────────────────────────────────────────────────────────────────────
void baseTopicValidatorTests() {
  std::printf("\n[one base-topic validator — the cases the two modules disagreed about]\n");

  // ── The divergences: each had two answers, and now has one, the strict one ──
  check(!NetSettings::isValidBaseTopic("watermeter/a1b2c3/"),
        "trailing '/': MqttPublisher::configure stripped and accepted, HaDiscovery refused");
  check(!NetSettings::isValidBaseTopic("watermeter/a1b2c3///"),
        "and several of them, which MqttPublisher stripped without telling anybody");
  check(!NetSettings::isValidBaseTopic("/watermeter/a1"),
        "leading '/': MqttPublisher stripped only TRAILING slashes, HaDiscovery refused");
  check(!NetSettings::isValidBaseTopic("water meter/a1"),
        "a space: MqttPublisher's 0x20-0x7e let it through, HaDiscovery refused it");
  check(!NetSettings::isValidBaseTopic("watermeter\x7f/a1"),
        "DEL: HaDiscovery only refused below 0x20, so it took what MqttPublisher rejected");
  check(!NetSettings::isValidBaseTopic("watermeter\xc3\xa9/a1"),
        "and non-ASCII, refused by the publisher and accepted by discovery — §4.6 forbids it");
  check(!NetSettings::isValidBaseTopic("watermeter//a1b2c3"),
        "an empty middle level, which BOTH accepted: it publishes where nobody subscribes");

  // ── What must stay accepted, so the fix is not simply "refuse more" ──
  check(NetSettings::isValidBaseTopic("watermeter/a1b2c3"), "§4.2's own default shape is accepted");
  check(NetSettings::isValidBaseTopic("watermeter/plant-3/inlet"),
        "so is §7.6's worked example, three levels deep");
  check(NetSettings::isValidBaseTopic("wm"),
        "and a single level with no separator, which mqtt_publisher_test uses throughout");
  check(NetSettings::isValidBaseTopic("w/\"><script>&'"),
        "and a topic of HTML metacharacters: legal in MQTT, and escaping is the renderer's job "
        "(portal_form_test stores exactly this topic)");
  check(NetSettings::isValidBaseTopic("<a>~b"),
        "and a '~', which the portal decodes from %7E — so 0x7e is in the charset");

  check(!NetSettings::isValidBaseTopic("watermeter/+/a") &&
            !NetSettings::isValidBaseTopic("watermeter/#"),
        "wildcards stay refused — illegal in a topic a client publishes to (MQTT 3.1.1 §4.7.1)");

  // ── The edges ──
  check(!NetSettings::isValidBaseTopic(nullptr) && !NetSettings::isValidBaseTopic(""),
        "null and empty are not topics");
  check(!NetSettings::isValidBaseTopic("/"), "nor is a lone separator");
  const std::string atCap(plc::netFieldCapacity(NetField::MqttBaseTopic), 'b');
  const std::string overCap(plc::netFieldCapacity(NetField::MqttBaseTopic) + 1, 'b');
  // ha_discovery_test's R4.4.8 worst-case input is a base topic at exactly this cap, so the bound
  // has to be inclusive or the buffer proof loses its worst case.
  check(NetSettings::isValidBaseTopic(atCap.c_str()),
        "a topic at exactly the field's capacity is accepted");
  check(!NetSettings::isValidBaseTopic(overCap.c_str()),
        "one byte over is not — the store cannot hold it, so nothing may pretend it did");
}

// ────────────────────────────────────────────────────────────────────────────────────
// The whole-value surfaces: the display editor and the web portal.
//
// Breaks if: stage() stops consulting the validator, starts truncating the topic instead of
// refusing it, half-writes pending before refusing, or loses the empty-clears-the-field case.
// ────────────────────────────────────────────────────────────────────────────────────
void baseTopicStageTests() {
  std::printf("\n[an invalid topic cannot become live through a whole-value write]\n");

  NetSettings s;
  check(s.stage(NetField::MqttBaseTopic, "watermeter/good") && s.apply(),
        "a valid topic stages and applies");
  checkStr(liveOf(s, NetField::MqttBaseTopic).c_str(), "watermeter/good", "and is live");
  const uint16_t revision = s.revision();

  check(!s.stage(NetField::MqttBaseTopic, "watermeter/bad/"),
        "a trailing-slash topic is REFUSED at stage time, neither repaired nor stored");
  checkStr(stagedOf(s, NetField::MqttBaseTopic).c_str(), "watermeter/good",
           "pending still holds the previous topic — nothing was half-written before refusing");
  check(!s.dirty() && !s.apply() && s.revision() == revision,
        "so there is nothing to apply and the revision does not move");

  // The deliberate exception to stage()'s truncate-don't-reject rule, with the contrast that shows
  // it IS an exception rather than a change of policy.
  const std::string overCap(plc::netFieldCapacity(NetField::MqttBaseTopic) + 1, 'b');
  check(!s.stage(NetField::MqttBaseTopic, overCap.c_str()),
        "an over-long TOPIC is refused: truncating gives a valid topic nobody subscribes to");
  const std::string longSsid(200, 'x');
  check(s.stage(NetField::WifiSsid, longSsid.c_str()),
        "while an over-long SSID is still ACCEPTED and truncated, as the block write needs");
  check(s.apply(), "and that apply lands");
  check(liveOf(s, NetField::WifiSsid).size() == plc::netFieldCapacity(NetField::WifiSsid),
        "with the SSID at the field's capacity");
  checkStr(liveOf(s, NetField::MqttBaseTopic).c_str(), "watermeter/good",
           "and the refused topic never reaching live");

  check(s.stage(NetField::MqttBaseTopic, ""), "clearing the topic is allowed");
  check(s.apply() && liveOf(s, NetField::MqttBaseTopic).empty(),
        "empty means 'not configured', where §4.2's MAC-derived default takes over");

  // Named on its own because this protection is easy to lose: gating apply() on isValidBaseTopic()
  // alone would refuse EVERY apply on a factory-fresh device, whose topic field is empty.
  NetSettings fresh;
  check(fresh.stagedBaseTopicCommittable(),
        "a fresh device's empty topic is committable even though it is not a valid topic");
  check(fresh.stage(NetField::WifiSsid, "PlantFloor") && fresh.apply(),
        "so an empty base topic does not block an unrelated apply — NetSettings has no MAC and "
        "cannot spell §4.2's default, therefore empty must stay committable");
}

// ────────────────────────────────────────────────────────────────────────────────────
// The register surface, where a topic arrives one byte at a time.
//
// Breaks if: validation migrates into stageByte() (the block write becomes impossible), the apply()
// gate is removed (an invalid topic goes live over RS485), or trailing NUL padding is mistaken for
// a malformed topic.
// ────────────────────────────────────────────────────────────────────────────────────
void baseTopicRegisterTests() {
  std::printf("\n[the register path — one byte at a time, so apply() is the gate]\n");

  const uint16_t base = plc::net_reg::textBase(NetField::MqttBaseTopic);
  const uint16_t span = plc::net_reg::textRegisters(plc::netFieldCapacity(NetField::MqttBaseTopic));

  // "wfm/plant3" puts its '/' at index 3 — an ODD offset, so it lands as the LOW byte of the second
  // register and the field is transiently "wfm/", a trailing slash. Not contrived: every topic whose
  // separator sits at an odd offset passes through that state during an in-order block write.
  NetSettings s;
  check(NetRegisterMap::stageWrite(s, base, NetRegisterMap::packChars('w', 'f')),
        "the first register of a topic stages");
  check(NetRegisterMap::stageWrite(s, static_cast<uint16_t>(base + 1),
                                   NetRegisterMap::packChars('m', '/')),
        "and so does the second, which is what a byte-level validator would have refused");
  checkStr(stagedOf(s, NetField::MqttBaseTopic).c_str(), "wfm/",
           "the intermediate state really is the invalid one, so the check above means something");
  check(!s.stagedBaseTopicCommittable(),
        "an apply landing HERE is refused rather than committing half a topic");
  check(!s.apply() && liveOf(s, NetField::MqttBaseTopic).empty(), "and it is refused");
  check(NetRegisterMap::stageWrite(s, static_cast<uint16_t>(base + 2),
                                   NetRegisterMap::packChars('p', 'l')) &&
            NetRegisterMap::stageWrite(s, static_cast<uint16_t>(base + 3),
                                       NetRegisterMap::packChars('a', 'n')) &&
            NetRegisterMap::stageWrite(s, static_cast<uint16_t>(base + 4),
                                       NetRegisterMap::packChars('t', '3')),
        "the master finishes the field, every byte accepted along the way");
  check(NetRegisterMap::applyWrite(s, plc::net_reg::kApplyMagic) == NetApplyError::None,
        "and NOW the apply lands");
  checkStr(liveOf(s, NetField::MqttBaseTopic).c_str(), "wfm/plant3", "with the topic intact");

  // §5's documented write sequence: a master writes the WHOLE field, so a short topic arrives with a
  // long tail of NUL registers. That must apply — refusing padding would break the sequence.
  NetSettings padded;
  const std::string topic = "watermeter/a1b2c3";
  std::size_t refused = 0;
  std::size_t padding = 0;
  for (uint16_t r = 0; r < span; ++r) {
    const std::size_t at = static_cast<std::size_t>(r) * 2;
    const char high = at < topic.size() ? topic[at] : '\0';
    const char low = at + 1 < topic.size() ? topic[at + 1] : '\0';
    if (high == '\0' && low == '\0') ++padding;
    if (!NetRegisterMap::stageWrite(padded, static_cast<uint16_t>(base + r),
                                    NetRegisterMap::packChars(high, low))) {
      ++refused;
    }
  }
  std::printf("      %u registers written, %zu of them pure NUL padding\n",
              static_cast<unsigned>(span), padding);
  check(padding >= 15, "a 17-byte topic really does arrive with a tail of NUL registers");
  check(refused == 0, "every register of the block write is accepted, padding included");
  check(NetRegisterMap::applyWrite(padded, plc::net_reg::kApplyMagic) == NetApplyError::None,
        "the padded block write applies — padding is not mistaken for a malformed topic");
  checkStr(liveOf(padded, NetField::MqttBaseTopic).c_str(), "watermeter/a1b2c3",
           "and the stored topic carries none of it");

  // An invalid topic arriving over RS485 alongside a valid SSID.
  NetSettings viaBus;
  viaBus.stage(NetField::WifiSsid, "PlantFloor");
  const std::string bad = "watermeter/a1/";
  for (uint16_t r = 0; r < span; ++r) {
    const std::size_t at = static_cast<std::size_t>(r) * 2;
    const char high = at < bad.size() ? bad[at] : '\0';
    const char low = at + 1 < bad.size() ? bad[at + 1] : '\0';
    NetRegisterMap::stageWrite(viaBus, static_cast<uint16_t>(base + r),
                               NetRegisterMap::packChars(high, low));
  }
  const NetApplyError result = NetRegisterMap::applyWrite(viaBus, plc::net_reg::kApplyMagic);
  check(liveOf(viaBus, NetField::MqttBaseTopic).empty(),
        "a trailing-slash topic written over RS485 does not become live");
  check(liveOf(viaBus, NetField::WifiSsid).empty() && viaBus.revision() == 0,
        "and NOTHING else in the block does either — a partial apply is the silent failure this "
        "validator exists to remove");
  check(viaBus.dirty(), "pending is kept, so correcting the topic alone is enough to re-apply");
  // The error code reported to the master is WRONG today, and net_register_map.cpp is not this
  // slice's to edit: apply() returns false, and applyWrite() reads that as NothingStaged. Pinned as
  // it actually behaves so the wiring fix — return the already-defined, currently-unused
  // NetApplyError::InvalidValue — flips a named check rather than passing silently.
  check(result == NetApplyError::NothingStaged,
        "TODO(net_register_map): the master is told NothingStaged; this must become InvalidValue");

  // Correcting the one offending byte is enough: '/' sits at index 13, the low byte of register 6.
  NetRegisterMap::stageWrite(viaBus, static_cast<uint16_t>(base + 6),
                             NetRegisterMap::packChars('1', '\0'));
  check(NetRegisterMap::applyWrite(viaBus, plc::net_reg::kApplyMagic) == NetApplyError::None,
        "clearing the trailing slash lets the same staged block apply");
  checkStr(liveOf(viaBus, NetField::MqttBaseTopic).c_str(), "watermeter/a1",
           "with the corrected topic live");
  checkStr(liveOf(viaBus, NetField::WifiSsid).c_str(), "PlantFloor",
           "and the SSID staged before the bad topic goes live with it, not lost to the refusal");
}

}  // namespace

/**
 * Portal-login recovery (R8.2a).
 *
 * The portal password has no panel row and is changed through the portal itself, so losing it used
 * to mean a factory reset — totals and calibration included. Two recovery paths now exist, and what
 * matters is that they restore ONLY the login.
 */
void portalResetTests() {
  std::printf("\n[portal-login recovery — R8.2a]\n");
  plc::NetSettings net;

  // Establish a configured device with a changed login and real measurement-adjacent config.
  net.stage(plc::NetField::PortalUser, "plantops");
  net.stage(plc::NetField::PortalPassword, "correct-horse");
  net.stage(plc::NetField::WifiSsid, "PlantFloor");
  net.stage(plc::NetField::MqttHost, "192.168.1.10");
  net.stageMqttPort(8883);
  check(net.apply(), "a configured device applies");
  check(!net.portalPasswordIsDefault(), "and its portal password is no longer the shipped default");
  const uint16_t revisionBefore = net.revision();

  // ── Path 1: the direct call, which the menu confirm screen uses ────────────────
  net.resetPortalCredentials();
  char buf[80] = {};
  net.get(plc::NetField::PortalUser, buf, sizeof(buf));
  check(std::strcmp(buf, "admin") == 0, "the user is back to admin");
  net.get(plc::NetField::PortalPassword, buf, sizeof(buf));
  check(std::strcmp(buf, "admin") == 0, "the password is back to admin");
  check(net.portalPasswordIsDefault(), "so the §7.9a nag re-raises itself");
  check(net.revision() > revisionBefore, "the revision bumped, so a polling master sees it landed");

  // The whole point: nothing else moved.
  net.get(plc::NetField::WifiSsid, buf, sizeof(buf));
  check(std::strcmp(buf, "PlantFloor") == 0, "the WiFi SSID is untouched");
  net.get(plc::NetField::MqttHost, buf, sizeof(buf));
  check(std::strcmp(buf, "192.168.1.10") == 0, "the broker is untouched");
  check(net.mqttPort() == 8883, "the port is untouched");

  // Live AND pending together — a recovery step must not need a follow-up apply.
  check(!net.dirty(), "nothing is left staged, so no apply is required to finish");
  net.getStaged(plc::NetField::PortalPassword, buf, sizeof(buf));
  check(std::strcmp(buf, "admin") == 0, "pending matches live rather than holding the old password");

  // ── Path 2: over RS485 ────────────────────────────────────────────────────────
  plc::NetSettings viaBus;
  viaBus.stage(plc::NetField::PortalPassword, "hunter2");
  viaBus.apply();
  check(!viaBus.portalPasswordIsDefault(), "a second device has a changed login");

  check(plc::NetRegisterMap::isWritable(plc::net_reg::kPortalReset),
        "the reset register is writable");
  check(plc::NetRegisterMap::stageWrite(viaBus, plc::net_reg::kPortalReset, plc::net_reg::kApplyMagic),
        "writing the magic is accepted");
  check(viaBus.portalPasswordIsDefault(),
        "and acts IMMEDIATELY — no apply needed, which is what a locked-out operator needs");

  // ── The magic is required, so a block write cannot reset the login by accident ──
  plc::NetSettings sweep;
  sweep.stage(plc::NetField::PortalPassword, "hunter2");
  sweep.apply();
  for (uint16_t v : {static_cast<uint16_t>(0), static_cast<uint16_t>(1),
                     static_cast<uint16_t>(0xFFFF), static_cast<uint16_t>(0x5AA4)}) {
    plc::NetRegisterMap::stageWrite(sweep, plc::net_reg::kPortalReset, v);
  }
  check(!sweep.portalPasswordIsDefault(),
        "every non-magic value is ignored, so a sweep across the block leaves the login alone");

  // And a real block write across the whole region must still not trip it.
  plc::NetSettings block;
  block.stage(plc::NetField::PortalPassword, "hunter2");
  block.apply();
  for (uint16_t a = plc::net_reg::kBase; a < plc::net_reg::kEnd; ++a) {
    if (a == plc::net_reg::kApply) continue;
    plc::NetRegisterMap::stageWrite(block, a, 0x0000);
  }
  check(!block.portalPasswordIsDefault(),
        "a zero-fill of all 233 registers does not reset the login either");
}

int main() {
  std::printf("plc::NetSettings — text settings storage and register packing\n\n");
  defaultsTests();
  stagingTests();
  validationTests();
  registerRoundTripTests();
  applyProtocolTests();
  secrecyTests();
  blockWriteTests();
  baseTopicValidatorTests();
  baseTopicStageTests();
  baseTopicRegisterTests();
  portalResetTests();
  std::printf("\n%s (%d checks, %d failures)\n", failures == 0 ? "ALL PASSED" : "FAILURES", checks,
              failures);
  return failures == 0 ? 0 : 1;
}
