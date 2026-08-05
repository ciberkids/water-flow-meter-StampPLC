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
};

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
