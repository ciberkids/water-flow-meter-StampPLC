#include "net/mqtt_command_router.h"

namespace plc {

/**
 * The wording published on register 565 and the status topic (R4.4.2d).
 *
 * Short, lower-case and hyphenated because a Home Assistant template renders it verbatim and the MQTT info
 * page has 40 columns. `retained-ignored` says what to DO about it — clear the retained message at the
 * broker — which a bare "ignored" would not.
 */
const char* mqttCommandResultText(MqttCommandResult result) {
  switch (result) {
    case MqttCommandResult::Idle:
      return "idle";
    case MqttCommandResult::Accepted:
      return "accepted";
    case MqttCommandResult::RateLimited:
      return "rate-limited";
    case MqttCommandResult::RetainedIgnored:
      return "retained-ignored";
    case MqttCommandResult::BadPayload:
      return "bad-payload";
    case MqttCommandResult::UnknownCommand:
      return "unknown-command";
  }
  return "idle";
}

}  // namespace plc
