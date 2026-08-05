#pragma once

// NVS persistence for NetSettings.
//
// NOT HOST-COMPILED: it touches Preferences. Everything with semantics — validation, the staged/apply
// protocol, secret masking — lives in NetSettings and is covered by 117 host checks. This file only
// moves bytes, which is why it can afford to be untested.
//
// Without it the device forgets its network on every power cycle: NetSettings was a plain global with
// no load and no save, so an operator who provisioned WiFi over RS485 or through the portal would
// find the device unconfigured after the first brownout. That is not a missing nicety on a
// wall-mounted meter — it is the difference between commissioning once and commissioning forever.

#include <Preferences.h>

#include <cstddef>
#include <cstdint>

#include "net/net_register_map.h"  // the flag bit masks and the one mqttFlags assembler
#include "net/net_settings.h"

namespace plc {

namespace net_nvs {

/**
 * NVS key per text field.
 *
 * Indexed by NetField, so the table and the enum cannot drift apart — adding a field without a key
 * is a missing initialiser rather than a silent skip. NVS caps a key at 15 characters, hence the
 * abbreviations; they are internal and never shown to anyone.
 */
inline constexpr const char* kTextKeys[static_cast<std::size_t>(NetField::Count)] = {
    "n_ssid",    // WifiSsid
    "n_psk",     // WifiPsk
    "n_mhost",   // MqttHost
    "n_muser",   // MqttUser
    "n_mpass",   // MqttPassword
    "n_mtopic",  // MqttBaseTopic
    "n_mpfx",    // MqttDiscoveryPrefix
    "n_puser",   // PortalUser
    "n_ppass",   // PortalPassword
};

inline constexpr const char* kWifiEnabled = "n_wen";
inline constexpr const char* kMqttEnabled = "n_men";
inline constexpr const char* kMqttPort = "n_mport";
inline constexpr const char* kMqttPeriod = "n_mper";
inline constexpr const char* kMqttFlags = "n_mflags";

}  // namespace net_nvs

/**
 * Loads persisted network settings, staging then applying so the live block changes once.
 *
 * Absent keys are left at their constructor defaults rather than overwritten with empties — that is
 * what makes a first boot, and a boot after a partial write, both land on something sane. The portal
 * login in particular must stay `admin`/`admin` rather than becoming blank, which would lock nobody
 * out (§7.9a).
 *
 * Values that fail validation are simply not staged. A stored base topic that the current firmware
 * would refuse — an older build with a laxer rule, say — is dropped rather than carried forward, and
 * §4.2's default takes over. Silently correcting it would be worse: the device would publish to a
 * topic nobody configured.
 */
inline void loadNetSettings(Preferences& prefs, NetSettings& settings) {
  char buffer[NetSettings::kMaxValueBytes + 1] = {};

  for (std::size_t i = 0; i < static_cast<std::size_t>(NetField::Count); ++i) {
    const char* key = net_nvs::kTextKeys[i];
    if (!prefs.isKey(key)) {
      continue;
    }
    const std::size_t read = prefs.getString(key, buffer, sizeof(buffer));
    if (read == 0 && buffer[0] != '\0') {
      continue;  // a read that reported nothing but left the buffer dirty is not trustworthy
    }
    settings.stage(static_cast<NetField>(i), buffer);
  }

  if (prefs.isKey(net_nvs::kWifiEnabled)) {
    settings.stageWifiEnabled(prefs.getBool(net_nvs::kWifiEnabled, false));
  }
  if (prefs.isKey(net_nvs::kMqttEnabled)) {
    settings.stageMqttEnabled(prefs.getBool(net_nvs::kMqttEnabled, false));
  }
  if (prefs.isKey(net_nvs::kMqttPort)) {
    settings.stageMqttPort(prefs.getUShort(net_nvs::kMqttPort, 1883));
  }
  if (prefs.isKey(net_nvs::kMqttPeriod)) {
    settings.stageMqttPublishPeriodS(prefs.getUShort(net_nvs::kMqttPeriod, 10));
  }
  if (prefs.isKey(net_nvs::kMqttFlags)) {
    const uint16_t flags = prefs.getUShort(net_nvs::kMqttFlags, NetRegisterMap::kFlagHaDiscovery);
    settings.stageMqttHaDiscovery((flags & NetRegisterMap::kFlagHaDiscovery) != 0);
    settings.stageMqttQos((flags & NetRegisterMap::kFlagQos1) != 0 ? 1 : 0);
  }

  // One promotion for the whole restore, so nothing observes a half-loaded configuration — the same
  // reason §5.5 gives for the staged/apply protocol in the first place.
  settings.apply();
}

/**
 * Writes the LIVE block to NVS.
 *
 * Called after an apply, not on every staged keystroke: NVS is flash, and §2.1.3 records that a
 * flash write suspends the other core's scheduler with cache disabled — which stops the pulse
 * sampler outright. Writing once per committed change rather than once per edit keeps those stalls
 * as rare as the operator's actual decisions.
 *
 * Secrets are written in clear because NVS is not encrypted in this build. That is not a new
 * exposure — §8.1 already accepts it for the WiFi passphrase, since the radio needs the plaintext —
 * but it is worth stating where the bytes land rather than leaving it to be discovered.
 */
inline void saveNetSettings(Preferences& prefs, const NetSettings& settings) {
  char buffer[NetSettings::kMaxValueBytes + 1] = {};
  for (std::size_t i = 0; i < static_cast<std::size_t>(NetField::Count); ++i) {
    if (!settings.get(static_cast<NetField>(i), buffer, sizeof(buffer))) {
      continue;
    }
    prefs.putString(net_nvs::kTextKeys[i], buffer);
  }
  prefs.putBool(net_nvs::kWifiEnabled, settings.wifiEnabled());
  prefs.putBool(net_nvs::kMqttEnabled, settings.mqttEnabled());
  prefs.putUShort(net_nvs::kMqttPort, settings.mqttPort());
  prefs.putUShort(net_nvs::kMqttPeriod, settings.mqttPublishPeriodS());
  prefs.putUShort(net_nvs::kMqttFlags, NetRegisterMap::mqttFlags(settings));
}

}  // namespace plc
