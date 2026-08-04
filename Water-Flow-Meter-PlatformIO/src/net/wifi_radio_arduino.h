#pragma once

// The Arduino/ESP-IDF implementation of WifiManager's injected radio.
//
// NOT HOST-COMPILED, and that is the whole point of the split: WifiManager's state machine — the
// backoff ladder, the AP window, the provisioning hand-off — is Arduino-free and carries 154 host
// checks against a fake radio. Everything that genuinely cannot be tested without hardware is
// concentrated here, in as thin a layer as possible, so the untestable surface is small enough to
// review by eye.
//
// Every SDK fact below was verified against the installed arduino-esp32 2.0.17 headers rather than
// recalled. Two would have been wrong from memory:
//   * `WL_STOPPED` does not exist in this core. WiFiType.h:43-50 defines only NO_SHIELD(255),
//     IDLE_STATUS(0), NO_SSID_AVAIL(1), SCAN_COMPLETED(2), CONNECTED(3), CONNECT_FAILED(4),
//     CONNECTION_LOST(5), DISCONNECTED(6).
//   * the disconnect reason lives at `info.wifi_sta_disconnected.reason` (WiFiGeneric.h:84); the
//     field has been renamed across core versions, so the name is version-specific.

#include <WiFi.h>
#include <esp_mac.h>
#include <esp_wifi_types.h>

#include <cstdint>
#include <cstring>

#include "net/wifi_manager.h"

namespace plc {

/**
 * Drives the real radio for WifiManager.
 *
 * One instance, constructed before WifiManager and outliving it.
 */
class ArduinoWifiRadio final : public WifiRadio {
 public:
  ArduinoWifiRadio() {
    // Captured once so a disconnect REASON is available to status(). Without it, telling "wrong
    // passphrase" from "AP out of range" would rest on wl_status_t alone, and that mapping is not
    // dependable: arduino-esp32 reports WL_DISCONNECTED for an auth failure as often as
    // WL_CONNECT_FAILED. §9's failure table requires the two to be distinguishable — a device that
    // says "check your password" when the AP is simply absent sends an operator to the wrong place.
    // Captures `this`: onEvent's WiFiEventFuncCb overload is a std::function
    // (WiFiGeneric.h:108), not the bare function pointer WiFiEventCb (:107), so instance state is
    // reachable and no file-scope or function-local slot is needed.
    WiFi.onEvent(
        [this](arduino_event_id_t, arduino_event_info_t info) {
          lastDisconnectReason_ = info.wifi_sta_disconnected.reason;
        },
        ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFi.onEvent([this](arduino_event_id_t, arduino_event_info_t) { lastDisconnectReason_ = 0; },
                 ARDUINO_EVENT_WIFI_STA_CONNECTED);
  }

  bool begin() override {
    if (!WiFi.mode(WIFI_STA)) {
      return false;
    }
    // WifiManager owns retry timing (R4.1.2 / §3.1.2). Leaving the driver's own auto-reconnect on
    // would give the device two retry policies racing each other: the ladder would compute a 30 s
    // wait while the driver quietly retried every second, so the measured backoff would be the
    // driver's and the ladder would be decoration.
    WiFi.setAutoReconnect(false);
    WiFi.persistent(false);  // credentials live in NetSettings/NVS, not in the driver's own store
    return true;
  }

  void end() override {
    // DOWN, not merely disconnected — the interface is explicit about this. WIFI_OFF is what
    // actually stops the radio drawing current and stops lwIP being handed packets on core 0.
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    apUp_ = false;
  }

  bool connect(const char* ssid, const char* psk) override {
    if (ssid == nullptr || ssid[0] == '\0') {
      return false;
    }
    lastDisconnectReason_ = 0;
    // An empty passphrase is an OPEN network, which is legitimate. WiFi.begin(ssid, "") is not
    // reliably treated as open across core versions, so the two cases are called out separately.
    const bool open = (psk == nullptr || psk[0] == '\0');
    const wl_status_t status = open ? WiFi.begin(ssid) : WiFi.begin(ssid, psk);
    // begin() returns the CURRENT status, not a success flag. Anything other than an outright
    // refusal means the attempt is under way; WifiManager's connect timeout decides the rest.
    return status != WL_CONNECT_FAILED && status != WL_NO_SHIELD;
  }

  void disconnect() override {
    WiFi.setAutoReconnect(false);  // belt and braces: the driver must not resume behind us
    WiFi.disconnect(false);        // false = leave the radio powered; end() is what powers it down
  }

  bool startAp(const char* ssid, const char* password) override {
    if (ssid == nullptr || ssid[0] == '\0') {
      return false;
    }
    // AP_STA rather than AP: §5.2's remote-assisted flow has an operator reading NET_AP_SSID over
    // RS485 while the device is trying to associate, and R7.6 tears the AP down "on success" — both
    // need the station side alive at the same time as the AP.
    if (!WiFi.mode(WIFI_AP_STA)) {
      return false;
    }
    // WPA2 needs at least 8 characters; softAP silently falls back to an OPEN network for a shorter
    // one, which would quietly defeat R7.5's "the AP is not open". Refuse instead.
    if (password == nullptr || std::strlen(password) < 8) {
      return false;
    }
    apUp_ = WiFi.softAP(ssid, password);
    return apUp_;
  }

  void stopAp() override {
    WiFi.softAPdisconnect(true);
    apUp_ = false;
    // Back to station-only so the AP stops beaconing and stops costing airtime (§2.1.1).
    WiFi.mode(WIFI_STA);
  }

  RadioLink status() override {
    switch (WiFi.status()) {
      case WL_CONNECTED:
        return RadioLink::Up;
      case WL_NO_SSID_AVAIL:
        return RadioLink::ApNotFound;
      case WL_CONNECT_FAILED:
        return classifyFailure();
      case WL_IDLE_STATUS:
      case WL_SCAN_COMPLETED:
        return RadioLink::Connecting;
      case WL_DISCONNECTED:
      case WL_CONNECTION_LOST:
        // Ambiguous by construction: arduino-esp32 reports WL_DISCONNECTED both while an association
        // is in progress and after one fails. The reason code resolves it when there is one, and
        // WifiManager's kConnectTimeoutMs resolves it when there is not — which is exactly why the
        // interface's comment on RadioLink::Down says "while Connecting this is ambiguous".
        return classifyFailure();
      case WL_NO_SHIELD:
        return RadioLink::Down;
    }
    return RadioLink::Down;
  }

  int16_t rssi() override { return static_cast<int16_t>(WiFi.RSSI()); }

  uint32_t ipAddress() override { return pack(WiFi.localIP()); }
  uint32_t apIpAddress() override { return pack(WiFi.softAPIP()); }

  void macAddress(uint8_t out[6]) override {
    if (out == nullptr) {
      return;
    }
    // esp_read_mac, NOT WiFi.macAddress(): the interface's own comment says why, and it is right.
    // This is called before begin(), and WiFi.macAddress() is not dependable before a mode has been
    // selected. ESP_MAC_WIFI_SOFTAP is used rather than STA because R7.5a derives the AP identity
    // from it and that identity must be stable across reboots and across radio modes.
    std::memset(out, 0, 6);
    esp_read_mac(out, ESP_MAC_WIFI_SOFTAP);
  }

 private:
  /**
   * Packs an IPAddress as (a << 24) | (b << 16) | (c << 8) | d, per the interface.
   *
   * Built from the OCTETS rather than cast from the underlying uint32_t. IPAddress stores its bytes
   * in network order, so `(uint32_t)ip` yields the octets reversed relative to what the interface
   * asks for — a cast would put 192.168.1.50 on the wire as 50.1.168.192 and the register block
   * would report a plausible-looking wrong address. This exact hazard was flagged in review.
   */
  static uint32_t pack(const IPAddress& ip) {
    return (static_cast<uint32_t>(ip[0]) << 24) | (static_cast<uint32_t>(ip[1]) << 16) |
           (static_cast<uint32_t>(ip[2]) << 8) | static_cast<uint32_t>(ip[3]);
  }

  /** Turns the captured disconnect reason into the distinction §9 needs. */
  RadioLink classifyFailure() const {
    switch (lastDisconnectReason_) {
      case WIFI_REASON_AUTH_FAIL:
      case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
      case WIFI_REASON_HANDSHAKE_TIMEOUT:
      case WIFI_REASON_IE_IN_4WAY_DIFFERS:
        return RadioLink::AuthFailed;
      case WIFI_REASON_NO_AP_FOUND:
        return RadioLink::ApNotFound;
      default:
        return RadioLink::Down;
    }
  }

  /**
   * The last STA disconnect reason, or 0 when none.
   *
   * `volatile` because it is written from the Arduino event task and read from whichever task calls
   * status(). It is a single byte and the reads are advisory — used only to refine an already-failed
   * state into AuthFailed vs ApNotFound — so a torn read is not reachable and a lock would buy
   * nothing.
   */
  volatile uint8_t lastDisconnectReason_ = 0;

  bool apUp_ = false;
};

}  // namespace plc
