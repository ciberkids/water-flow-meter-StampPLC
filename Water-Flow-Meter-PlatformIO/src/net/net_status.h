#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>

#include "net/wifi_manager.h"

/**
 * A flat snapshot of network state for the display and the register block.
 *
 * The display cannot reach WifiManager directly, and should not: UiRenderContext is a value the
 * renderer reads without locking, so anything it exposes has to be a copy taken at a known moment
 * rather than a live object another task is mutating. This is that copy.
 *
 * Deliberately plain and Arduino-free so ui_bindings can be host-tested against it.
 */
namespace plc {

struct NetStatusSnapshot {
  // ── Station ────────────────────────────────────────────────────────────────────
  WifiState wifiState = WifiState::Disabled;
  WifiError wifiError = WifiError::None;
  /** Negative dBm. Only meaningful while associated. */
  int16_t rssiDbm = 0;
  /** `(a << 24) | (b << 16) | (c << 8) | d`, or 0 before DHCP. */
  uint32_t ipAddress = 0;
  char ssid[33] = {};

  // ── The provisioning AP (§5.2, R7.5, R7.5a) ────────────────────────────────────
  char apSsid[33] = {};
  /**
   * Shown in clear on the AP info page and readable over RS485.
   *
   * That asymmetry is R5.3, not an oversight: this describes an access point the device is
   * BROADCASTING, which anyone in radio range can already see, and a remote operator needs it to
   * direct somebody standing at the panel. The WiFi passphrase the operator GAVE us is a different
   * thing and never reads back.
   */
  char apPassword[33] = {};
  uint32_t apIpAddress = 0;
  uint16_t portalRemainingS = 0;

  // ── MQTT ───────────────────────────────────────────────────────────────────────
  /**
   * Whether the broker connection is up.
   *
   * Hard-wired false until N5 constructs a client. That is not a placeholder standing in for real
   * state — it is TRUE: there is no MQTT client in the image, so MQTT is not connected, and the
   * display saying so is accurate rather than provisional. When the client lands this field starts
   * carrying its answer and nothing else changes.
   */
  bool mqttConnected = false;
  /** True once a broker host is configured, so "off" and "configured but down" can differ. */
  bool mqttConfigured = false;
  bool mqttEnabled = false;
  /**
   * R4.4.2d — the outcome of the last §4.4.1 command, for the panel and register 565.
   *
   * Carried as the enum's underlying value rather than the enum, because this header is included by
   * the UI layer and `net_status.h` deliberately depends on nothing in `src/net` beyond the two
   * managers it builds from. `mqttCommandResultText` turns it back into words at both ends.
   *
   * Filled by `firmware.cpp` after the snapshot is built, not by `netStatusFrom`: the result lives in
   * the router, and the router is not one of the two things this file knows how to read.
   */
  uint8_t mqttLastCommandResult = 0;  // MqttCommandResult::Idle
};

/**
 * What register 561 carries, and what the panel's MQTT indicator says — ONE definition.
 *
 * 561 was declared read-only, documented in the register wiki as "broker connection state" and
 * written by nothing, so a Modbus master read 0 forever. Giving it a value meant naming the states,
 * and the panel had already named them: `mqttStateText` distinguishes exactly these four, because
 * "no broker configured" and "configured and down" are the difference between "finish setting it up"
 * and "go look at the broker". Two spellings of that decision would let the panel and the bus
 * disagree about the same device, which is the one thing the RS485-is-the-source-of-truth principle
 * forbids — so the panel now derives from this too.
 *
 * APPEND-ONLY, like every other wire enum here (I2): an integrator's template reads these numbers.
 */
enum class MqttLinkState : uint8_t {
  Off = 0,    /**< The client is disabled. Nothing is wrong. */
  Unset = 1,  /**< Enabled, but no broker host is configured — commissioning is unfinished. */
  Down = 2,   /**< Configured and not connected. Something to look at. */
  Ok = 3      /**< Connected. */
};

/** The state from the three facts that decide it. Ordered as the panel decides them. */
inline MqttLinkState mqttLinkState(bool enabled, bool configured, bool connected) {
  if (!enabled) return MqttLinkState::Off;
  if (connected) return MqttLinkState::Ok;
  return configured ? MqttLinkState::Down : MqttLinkState::Unset;
}

/** The panel's four words. ASCII and =<6 characters, for §4.6's Font0 budget. */
inline const char* mqttLinkStateText(MqttLinkState state) {
  switch (state) {
    case MqttLinkState::Off:   return "OFF";
    case MqttLinkState::Unset: return "UNSET";
    case MqttLinkState::Down:  return "DOWN";
    case MqttLinkState::Ok:    return "OK";
  }
  return "OFF";
}

namespace net_status_detail {
/** Bounded copy — the snapshot's buffers are fixed and the sources are NUL-terminated. */
inline void copyField(char* dest, std::size_t size, const char* src) {
  if (dest == nullptr || size == 0) {
    return;
  }
  std::memset(dest, 0, size);
  if (src == nullptr) {
    return;
  }
  std::size_t i = 0;
  while (src[i] != '\0' && i + 1 < size) {
    dest[i] = src[i];
    ++i;
  }
}
}  // namespace net_status_detail

/** Builds the snapshot from the live manager and settings. */
inline NetStatusSnapshot netStatusFrom(const WifiManager& wifi, const NetSettings& settings) {
  NetStatusSnapshot out;
  out.wifiState = wifi.state();
  out.wifiError = wifi.lastError();
  out.rssiDbm = wifi.rssiDbm();
  out.ipAddress = wifi.ipAddress();
  out.apIpAddress = wifi.apIpAddress();
  out.portalRemainingS = wifi.portalRemainingS();
  net_status_detail::copyField(out.ssid, sizeof(out.ssid), wifi.ssid());
  net_status_detail::copyField(out.apSsid, sizeof(out.apSsid), wifi.apSsid());
  net_status_detail::copyField(out.apPassword, sizeof(out.apPassword), wifi.apPassword());
  out.mqttEnabled = settings.mqttEnabled();
  out.mqttConfigured = settings.mqttConfigured();
  return out;
}

}  // namespace plc
