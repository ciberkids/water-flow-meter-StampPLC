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

}  // namespace

int main() {
  std::printf("plc::NetSettings — text settings storage and register packing\n\n");
  defaultsTests();
  stagingTests();
  validationTests();
  registerRoundTripTests();
  applyProtocolTests();
  secrecyTests();
  blockWriteTests();
  std::printf("\n%s (%d checks, %d failures)\n", failures == 0 ? "ALL PASSED" : "FAILURES", checks,
              failures);
  return failures == 0 ? 0 : 1;
}
