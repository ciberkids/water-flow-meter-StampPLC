#pragma once

#include <cstddef>

/**
 * The one definition of the MQTT client's output buffer size.
 *
 * R4.4.8 requires that "the buffer value and the test share one constant". It existed twice: this
 * value was defined independently in `ha_discovery.h` (where the worst-case-payload test measures
 * against it) and in `mqtt_publisher.h` (where the esp-mqtt transport actually reads it, via
 * `MqttPublisher::kOutBufferBytes`). Both headers carried a comment asserting they were the single
 * source of truth, and both were wrong.
 *
 * That is not a tidiness complaint. `esp_mqtt_client_publish` returns -1 when a payload exceeds the
 * configured buffer, and the message is simply not sent: no log line, no entity in Home Assistant,
 * nothing to notice. With two copies, halving the transport's buffer left the discovery test
 * measuring against a 2048 the client no longer used — a green suite over a silently broken
 * integration, which is the precise failure R4.4.8 was written to prevent.
 *
 * A separate header rather than one including the other, because neither direction is right:
 * discovery has no business depending on the publisher, and the publisher has none depending on
 * discovery. They are peers that share a hardware-shaped constant.
 */
namespace plc {

/**
 * What `esp_mqtt_client_config_t.out_buffer_size` is set to (§4.1.1, R4.1.6).
 *
 * Twice esp-mqtt's 1024-byte default. The default is not enough: a discovery payload carrying the
 * full device block sits in the same order of magnitude, and being close to a silent cliff is not a
 * margin.
 *
 * Changing this number changes what the device can send AND what the R4.4.8 test proves, together,
 * which is the entire point.
 */
inline constexpr std::size_t kMqttOutBufferSize = 2048;

}  // namespace plc
