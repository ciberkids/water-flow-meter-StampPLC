#pragma once

// The esp-mqtt half of slice N5. NOT host-compiled: it includes <mqtt_client.h>, so it must never
// appear in test/host/run.sh. Header-only for that reason too — there is no .cpp for a future
// run.sh edit to pick up by accident.
//
// Everything decidable without a broker lives in mqtt_publisher.{h,cpp} and is tested there. What
// is left here is configuration and an event callback, which is exactly the part a host test could
// only lie about.

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include <mqtt_client.h>

#include "net/mqtt_publisher.h"

namespace plc {

/**
 * `MqttSink` over ESP-IDF's own esp-mqtt client (§4.1.1 — already present and already linked, so
 * `lib_deps` does not grow).
 *
 * ⚠️ `esp_mqtt_client_config_t` in IDF v4.4.7 is a **FLAT** struct. Every ESP-IDF 5.x example
 * writes `.broker.address.uri`, `.credentials.username`, `.session.keepalive`; those members do not
 * exist here. Fields used below were read out of the installed header:
 * `tools/sdk/esp32s3/include/mqtt/esp-mqtt/include/mqtt_client.h`.
 */
class EspMqttTransport : public MqttSink {
 public:
  /** R4.1.5 — priority 1. Below the priority-2 polling task and far below the priority-8 Modbus
   *  server, which is what makes R2.1.0 and R2.1.4 hold for MQTT.
   *
   *  This is the whole reason the client's task affinity does not matter: esp-mqtt's task is
   *  created with `tskNO_AFFINITY` (CONFIG_MQTT_TASK_CORE_SELECTION_ENABLED is not set in the
   *  shipped sdkconfig) and CANNOT be pinned, so the scheduler may place it on core 0. A
   *  priority-1 task cannot preempt the priority-2 poller on either core, which turns an
   *  unpinnable task into a harmless one. See §2.1.3. Not optional. */
  static constexpr int kTaskPriority = 1;

  /** 6144 is esp-mqtt's own default. Named rather than left implicit because setting `task_prio`
   *  means the struct is no longer all-defaults, and a zero here would be read as "use default"
   *  only by luck of the library's initialiser. */
  static constexpr int kTaskStackBytes = 6144;

  /** R4.1.2 / §3.1.2. See the honesty note on `begin()` — this is a FIXED interval, not a ramp. */
  static constexpr int kReconnectTimeoutMs = 10000;

  static constexpr int kNetworkTimeoutMs = 10000;
  static constexpr int kDefaultKeepaliveS = 60;

  /** Buffer sizes. `out_buffer_size` is R4.1.6 and shares its constant with the publisher, so
   *  N6's R4.4.8 payload-length test measures against the value actually configured here. */
  static constexpr int kInBufferBytes = 1024;

  struct Options {
    /** Full broker URI, e.g. `mqtt://192.168.1.10:1883`. Always plain `mqtt://` — TLS is out of
     *  scope per Q3/R8.3, and the setting that used to select it has been removed rather than left
     *  as a toggle implying protection that is not there. (NetSettings::mqttTls
     *  is on. Built by the caller because the scheme/host/port split lives in NetSettings. */
    const char* uri = nullptr;
    const char* username = nullptr;  /**< nullptr or empty for an anonymous broker. */
    const char* password = nullptr;
    const char* clientId = nullptr;  /**< R4.1.4 — `mqttClientId()` output. */
    const char* lwtTopic = nullptr;  /**< R4.5.1 — `MqttPublisher::availabilityTopic()`. */
    int lwtQos = 0;
    int keepaliveS = kDefaultKeepaliveS;
  };

  /** Called from esp-mqtt's own task. Kept to the two transitions the policy half reacts to. */
  using StateCallback = void (*)(void* context, bool connected);

  /** Called from esp-mqtt's own task with the topic and payload of an inbound message — §4.4.7's
   *  `homeassistant/status` subscription, and §4.4.1's command topics. Neither is
   *  NUL-terminated by the library, hence the explicit lengths. */
  /**
   * `retained` is what makes R4.4.2c implementable at all: a retained command message must be discarded
   * unconditionally, and a callback that cannot see the flag cannot discard it. It was absent until the
   * command topics were built, because the only subscription was Home Assistant's birth message, where the
   * flag does not matter.
   */
  using DataCallback = void (*)(void* context, const char* topic, std::size_t topicLength,
                               const char* data, std::size_t dataLength, bool retained);

  void setListener(void* context, StateCallback onState, DataCallback onData) {
    context_ = context;
    onState_ = onState;
    onData_ = onData;
  }

  /**
   * Creates and starts the client. False if the configuration is unusable or the library refused.
   *
   * ── Honesty note on R4.1.2 ────────────────────────────────────────────────────────
   * §4.1.1 of the requirement says the §3.1.2 backoff comes "for free" from
   * `reconnect_timeout_ms`. It does not, and the installed header says so: "Reconnect to the
   * broker after this value in milliseconds if auto reconnect is not disabled (defaults to 10s)."
   * That is a FIXED retry interval. It satisfies the part of R4.1.2 that matters most — no
   * reconnect storm against an unreachable broker — but it is NOT the exponential 1 s → 5 min
   * ramp with jitter that §3.1.2 describes. Implementing that ramp needs
   * `esp_mqtt_set_config(client, &cfg)` from the DISCONNECTED event with a growing value, and it is
   * NOT implemented in this slice.
   */
  bool begin(const Options& options) {
    if (client_ != nullptr) return false;
    if (options.uri == nullptr || options.uri[0] == '\0') return false;
    if (options.clientId == nullptr || options.clientId[0] == '\0') return false;

    // Copy every string the config points at. IDF 4.4 is believed to strdup these internally, but
    // "believed" is not a lifetime guarantee, and the failure it would produce — a client
    // dereferencing a caller's expired buffer on reconnect, hours later — is the worst kind to
    // debug on a device with no debugger attached. The copies cost ~350 bytes.
    copy(uri_, sizeof(uri_), options.uri);
    copy(clientId_, sizeof(clientId_), options.clientId);
    copy(username_, sizeof(username_), options.username);
    copy(password_, sizeof(password_), options.password);
    copy(lwtTopic_, sizeof(lwtTopic_), options.lwtTopic);

    esp_mqtt_client_config_t cfg = {};  // IDF 4.4 — FLAT struct, see the header note above.
    cfg.uri = uri_;
    cfg.client_id = clientId_;
    cfg.username = username_[0] != '\0' ? username_ : nullptr;
    cfg.password = password_[0] != '\0' ? password_ : nullptr;
    cfg.protocol_ver = MQTT_PROTOCOL_V_3_1_1;  // CONFIG_MQTT_PROTOCOL_311=y; stated, not assumed.

    // R4.5.1 — availability by configuration rather than by a state machine we would have to test.
    // The retained `offline` will is the only thing that reports a device whose power was cut, and
    // §4.5's reason for caring is that a flow meter reading zero and a flow meter that is switched
    // off look identical in a dashboard.
    if (lwtTopic_[0] != '\0') {
      cfg.lwt_topic = lwtTopic_;
      cfg.lwt_msg = MqttPublisher::kOfflinePayload;
      cfg.lwt_msg_len = static_cast<int>(std::strlen(MqttPublisher::kOfflinePayload));
      cfg.lwt_qos = options.lwtQos;
      cfg.lwt_retain = 1;
    }

    cfg.task_prio = kTaskPriority;      // R4.1.5 — the invariant this whole slice hangs on.
    cfg.task_stack = kTaskStackBytes;
    cfg.buffer_size = kInBufferBytes;
    cfg.out_buffer_size = MqttPublisher::kOutBufferBytes;  // R4.1.6
    cfg.reconnect_timeout_ms = kReconnectTimeoutMs;
    cfg.network_timeout_ms = kNetworkTimeoutMs;
    cfg.keepalive = options.keepaliveS;

    client_ = esp_mqtt_client_init(&cfg);
    if (client_ == nullptr) return false;

    if (esp_mqtt_client_register_event(client_, MQTT_EVENT_ANY, &EspMqttTransport::onEvent, this) !=
        ESP_OK) {
      esp_mqtt_client_destroy(client_);
      client_ = nullptr;
      return false;
    }
    if (esp_mqtt_client_start(client_) != ESP_OK) {
      esp_mqtt_client_destroy(client_);
      client_ = nullptr;
      return false;
    }
    return true;
  }

  void end() {
    if (client_ == nullptr) return;
    esp_mqtt_client_stop(client_);
    esp_mqtt_client_destroy(client_);
    client_ = nullptr;
    connected_ = false;
  }

  /** Subscribes. Returns the message id, or -1 — used for `homeassistant/status` (R4.4.7). */
  int subscribe(const char* topic, int qos) {
    if (client_ == nullptr || topic == nullptr) return -1;
    return esp_mqtt_client_subscribe(client_, topic, qos);
  }

  /**
   * `MqttSink::publish`. Non-negative on success (0 at QoS 0), -1 on failure.
   *
   * `esp_mqtt_client_enqueue`, not `esp_mqtt_client_publish`. The header is explicit that
   * `_publish` "might block for several seconds, either due to network timeout (10s) or if
   * publishing payloads longer than internal buffer", in the CALLER's task — and the caller here is
   * the priority-1 logic/UI task on core 1. R4.1.1 requires publishing be fire-and-forget from the
   * UI's point of view, so the blocking variant is disqualified regardless of how rarely it blocks.
   * `_enqueue` stores the packet in the outbox and sends it from esp-mqtt's own task, which
   * `task_prio = 1` has already made harmless.
   *
   * `store = true` is required for QoS 0 to be enqueued at all; without it the QoS-0 case silently
   * falls back to the caller's context. Verified present in the shipped archive:
   * `nm libmqtt.a` lists `esp_mqtt_client_enqueue` and the symbol is declared in the installed
   * header.
   *
   * ── What R4.1.6 rests on here, precisely ──────────────────────────────────────────
   * §4.4.7's silent failure is a payload larger than `out_buffer_size`. The -1 the publisher counts
   * covers it for `esp_mqtt_client_publish`, which is where that behaviour was verified. Whether
   * `_enqueue` ALSO returns -1 in that case, rather than accepting the message and discarding it
   * later in its own task, was NOT verified against the library.
   *
   * It does not need to be: `static_assert(kMaxPayloadBytes <= kOutBufferBytes)` in
   * mqtt_publisher.h makes an oversize payload unreachable by construction, because the queue
   * cannot hold one. The oversize case is closed at the queue's door rather than by the return
   * value, and the return value is still checked for every other failure the client reports.
   */
  int publish(const char* topic, const char* payload, int qos, bool retain) override {
    if (client_ == nullptr || topic == nullptr || payload == nullptr) return -1;
    // Length 0 lets the library measure the NUL-terminated payload.
    return esp_mqtt_client_enqueue(client_, topic, payload, 0, qos, retain ? 1 : 0, true);
  }

  bool connected() const override { return connected_; }

 private:
  static void copy(char* dest, std::size_t size, const char* source) {
    if (size == 0) return;
    if (source == nullptr) {
      dest[0] = '\0';
      return;
    }
    std::snprintf(dest, size, "%s", source);
  }

  static void onEvent(void* handlerArgs, esp_event_base_t, int32_t eventId, void* eventData) {
    auto* self = static_cast<EspMqttTransport*>(handlerArgs);
    if (self == nullptr) return;
    const auto* event = static_cast<esp_mqtt_event_t*>(eventData);

    // Runs in esp-mqtt's task. Nothing here may block or touch the UI: it only flips a flag and
    // hands the two interesting events to the listener, which is expected to do the same.
    switch (static_cast<esp_mqtt_event_id_t>(eventId)) {
      case MQTT_EVENT_CONNECTED:
        self->connected_ = true;
        if (self->onState_ != nullptr) self->onState_(self->context_, true);
        break;
      case MQTT_EVENT_DISCONNECTED:
        self->connected_ = false;
        if (self->onState_ != nullptr) self->onState_(self->context_, false);
        break;
      case MQTT_EVENT_DATA:
        if (self->onData_ != nullptr && event != nullptr) {
          self->onData_(self->context_, event->topic, static_cast<std::size_t>(event->topic_len),
                        event->data, static_cast<std::size_t>(event->data_len), event->retain != 0);
        }
        break;
      case MQTT_EVENT_ERROR:
        // Deliberately not treated as a disconnect. esp-mqtt raises ERROR for refused credentials
        // and for transport faults alike, and DISCONNECTED follows when the session is really gone;
        // clearing the flag here as well would make the publisher republish everything on a
        // transient error.
        break;
      case MQTT_EVENT_ANY:
      case MQTT_EVENT_SUBSCRIBED:
      case MQTT_EVENT_UNSUBSCRIBED:
      case MQTT_EVENT_PUBLISHED:
      case MQTT_EVENT_BEFORE_CONNECT:
      case MQTT_EVENT_DELETED:
        break;
    }
  }

  esp_mqtt_client_handle_t client_ = nullptr;

  /** Written in esp-mqtt's task, read from the logic task. `volatile` on a single aligned byte is
   *  the same guarantee the sensor counters already rely on (`SensorData::pulseCount`). */
  volatile bool connected_ = false;

  void* context_ = nullptr;
  StateCallback onState_ = nullptr;
  DataCallback onData_ = nullptr;

  char uri_[128] = {};
  char clientId_[24] = {};
  char username_[33] = {};
  char password_[33] = {};
  char lwtTopic_[MqttPublisher::kMaxTopicBytes] = {};
};

}  // namespace plc
