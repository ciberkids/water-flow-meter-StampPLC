/**
 * The MQTT command router: §4.4.1's three topics and R4.4.2's safeguards.
 *
 * Arduino-free, because every rule here is a safety decision on a metering device and none of them is
 * observable without a broker, a clock and an hour. The owner's principle is the one being tested: a reset
 * failing remotely is not a breaking thing, the device entering a reset loop is — so every ambiguous case
 * must fail towards swallowing the command.
 */
#include <cstdio>

#include "net/mqtt_command_router.h"

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const char* what) {
  ++checks;
  std::printf("  %-76s %s\n", what, condition ? "ok" : "FAIL");
  if (!condition) failures++;
}

constexpr const char* kBase = "water/meter1";
/** 2026-08-20T10:00:00Z. */
constexpr uint32_t kEpoch = 1787220000u;

using plc::MqttCommand;
using plc::MqttCommandResult;
using plc::MqttCommandRouter;

void topicsMapToCommands() {
  std::printf("\n[the three topics of §4.4.1, and nothing else]\n");

  check(MqttCommandRouter::commandFor("water/meter1/cmd/reset-session", kBase) ==
            MqttCommand::ResetSession,
        "reset-session maps");
  check(MqttCommandRouter::commandFor("water/meter1/cmd/reset-totals", kBase) == MqttCommand::ResetTotals,
        "reset-totals maps");
  check(MqttCommandRouter::commandFor("water/meter1/cmd/republish", kBase) == MqttCommand::Republish,
        "republish maps");

  bool underPrefix = false;
  check(MqttCommandRouter::commandFor("water/meter1/cmd/reset-everything", kBase, &underPrefix) ==
            MqttCommand::None,
        "an unknown name under /cmd/ maps to nothing");
  check(underPrefix, "but is reported as being under the prefix, so a typo is visible rather than silent");

  MqttCommandRouter::commandFor("water/meter1/state/flow", kBase, &underPrefix);
  check(!underPrefix, "while an unrelated topic is not under the prefix at all");
  check(MqttCommandRouter::commandFor("other/meter/cmd/reset-totals", kBase) == MqttCommand::None,
        "and another device's base is never ours — a shared broker is the normal case");
  check(MqttCommandRouter::commandFor(nullptr, kBase) == MqttCommand::None, "a null topic is safe");
}

void destructiveCommandsDemandTheMagic() {
  std::printf("\n[R4.4.1: a destructive command requires the exact payload]\n");

  MqttCommandRouter router;
  check(router.evaluate(MqttCommand::ResetTotals, "RESET", false, 1'000, kEpoch) ==
            MqttCommandResult::Accepted,
        "RESET is accepted");

  MqttCommandRouter fresh;
  check(fresh.evaluate(MqttCommand::ResetTotals, "reset", false, 1'000, kEpoch) ==
            MqttCommandResult::BadPayload,
        "lower case is not the magic — this is the 0x5AA5 idiom, not a courtesy");
  check(fresh.evaluate(MqttCommand::ResetTotals, "", false, 1'000, kEpoch) == MqttCommandResult::BadPayload,
        "an EMPTY payload is refused, which is what a stray publish looks like");
  check(fresh.evaluate(MqttCommand::ResetTotals, nullptr, false, 1'000, kEpoch) ==
            MqttCommandResult::BadPayload,
        "and so is no payload at all");
  check(fresh.evaluate(MqttCommand::ResetTotals, "RESET ", false, 1'000, kEpoch) ==
            MqttCommandResult::BadPayload,
        "a trailing space is not RESET — exact, because 'nearly' is how a loop gets in");

  // republish destroys nothing, so §4.4.1 gives it "payload: anything".
  check(fresh.evaluate(MqttCommand::Republish, "", false, 1'000, kEpoch) == MqttCommandResult::Accepted,
        "republish takes any payload, including empty");
}

void retainedIsDiscardedBeforeAnythingElse() {
  std::printf("\n[R4.4.2c: a retained command is a fault at the broker, not a request]\n");

  MqttCommandRouter router;
  check(router.evaluate(MqttCommand::ResetTotals, "RESET", true, 1'000, kEpoch) ==
            MqttCommandResult::RetainedIgnored,
        "a retained RESET is discarded — otherwise it wipes the totals on every reconnect, forever");
  check(router.evaluate(MqttCommand::ResetTotals, "RESET", false, 1'000, kEpoch) ==
            MqttCommandResult::Accepted,
        "and discarding it did not consume the rate limit: the same command still works when sent live");

  MqttCommandRouter other;
  check(other.evaluate(MqttCommand::ResetTotals, "nonsense", true, 1'000, kEpoch) ==
            MqttCommandResult::RetainedIgnored,
        "retained is checked BEFORE the payload, so the operator is told to clear the broker");
  check(other.evaluate(MqttCommand::Republish, "x", true, 1'000, kEpoch) ==
            MqttCommandResult::RetainedIgnored,
        "and it applies to republish too, which would otherwise fire on every reconnect");
}

void theRateLimitIsPerKindAndOnMillis() {
  std::printf("\n[R4.4.2a: 60 s per kind, measured on millis() because an NTP sync moves the wall clock]\n");

  MqttCommandRouter router;
  check(router.evaluate(MqttCommand::ResetSession, "RESET", false, 10'000, kEpoch) ==
            MqttCommandResult::Accepted,
        "the first session reset is accepted");
  check(router.evaluate(MqttCommand::ResetSession, "RESET", false, 10'001, kEpoch) ==
            MqttCommandResult::RateLimited,
        "a second one a millisecond later is rate-limited");
  check(router.evaluate(MqttCommand::ResetSession, "RESET", false, 69'999, kEpoch) ==
            MqttCommandResult::RateLimited,
        "and one a millisecond early still is");

  // PER KIND: a totals reset is a different decision and must not be blocked by a session reset.
  check(router.evaluate(MqttCommand::ResetTotals, "RESET", false, 10'002, kEpoch) ==
            MqttCommandResult::Accepted,
        "but resetting the TOTALS is a separate kind and is not blocked by it");

  check(router.evaluate(MqttCommand::ResetSession, "RESET", false, 70'000, kEpoch + 60) ==
            MqttCommandResult::Accepted,
        "and after the full minute the session reset is accepted again");

  // republish is not rate-limited: the worst a loop does is make the device chatty.
  MqttCommandRouter chatty;
  chatty.evaluate(MqttCommand::Republish, "", false, 1'000, kEpoch);
  check(chatty.evaluate(MqttCommand::Republish, "", false, 1'001, kEpoch) == MqttCommandResult::Accepted,
        "republish is not rate-limited — it destroys nothing");
}

void thePersistedGuardSurvivesAReboot() {
  std::printf("\n[R4.4.2b: the loop that matters most reboots, and millis() starts again]\n");

  // A fresh router is exactly what a reboot produces: millis() back at zero, nothing accepted this boot.
  MqttCommandRouter afterReboot;
  afterReboot.seedLastAcceptedEpoch(MqttCommand::ResetTotals, kEpoch);
  check(afterReboot.evaluate(MqttCommand::ResetTotals, "RESET", false, 500, kEpoch + 5) ==
            MqttCommandResult::RateLimited,
        "a redelivered command five seconds after the reboot is refused — millis() alone would accept it");
  check(afterReboot.evaluate(MqttCommand::ResetTotals, "RESET", false, 900, kEpoch + 59) ==
            MqttCommandResult::RateLimited,
        "and at 59 s it still is");
  check(afterReboot.evaluate(MqttCommand::ResetTotals, "RESET", false, 1'000, kEpoch + 60) ==
            MqttCommandResult::Accepted,
        "at 60 s of WALL clock it is accepted, because the loop has demonstrably stopped");

  // The honest limit, asserted rather than left to be discovered: with no clock the persisted guard cannot
  // fire, and only the millis() guard is left.
  MqttCommandRouter noClock;
  noClock.seedLastAcceptedEpoch(MqttCommand::ResetTotals, kEpoch);
  check(noClock.evaluate(MqttCommand::ResetTotals, "RESET", false, 500, 0) == MqttCommandResult::Accepted,
        "with an UNSET clock the persisted guard cannot fire — the limit, stated not hidden");

  // And the persisted value only moves on acceptance, so a looping command does not loop NVS writes.
  MqttCommandRouter writes;
  writes.evaluate(MqttCommand::ResetTotals, "RESET", false, 1'000, kEpoch);
  const uint32_t afterAccept = writes.lastAcceptedEpoch(MqttCommand::ResetTotals);
  writes.evaluate(MqttCommand::ResetTotals, "RESET", false, 1'100, kEpoch + 1);
  writes.evaluate(MqttCommand::ResetTotals, "nope", false, 1'200, kEpoch + 2);
  check(writes.lastAcceptedEpoch(MqttCommand::ResetTotals) == afterAccept,
        "rejections do not move the persisted epoch, so a loop cannot become flash wear");
}

void resultsHaveWireWords() {
  std::printf("\n[R4.4.2d: a refusal must be visible, and say what to do about it]\n");

  check(std::strcmp(plc::mqttCommandResultText(MqttCommandResult::Accepted), "accepted") == 0, "accepted");
  check(std::strcmp(plc::mqttCommandResultText(MqttCommandResult::RateLimited), "rate-limited") == 0,
        "rate-limited");
  check(std::strcmp(plc::mqttCommandResultText(MqttCommandResult::RetainedIgnored), "retained-ignored") == 0,
        "retained-ignored — which names the fix: clear it at the broker");
  check(std::strcmp(plc::mqttCommandResultText(MqttCommandResult::BadPayload), "bad-payload") == 0,
        "bad-payload");
  check(std::strcmp(plc::mqttCommandResultText(MqttCommandResult::Idle), "idle") == 0,
        "and idle, which a fresh device reports rather than claiming a success it never had");

  // Every word fits the MQTT info page's 40 columns and a Home Assistant template.
  for (uint8_t value = 0; value <= 5; ++value) {
    const char* text = plc::mqttCommandResultText(static_cast<MqttCommandResult>(value));
    check(std::strlen(text) <= 20, text);
  }
}

void theWrapDoesNotReArmTheLimit() {
  std::printf("\n[the 49.7-day millis wrap must not hand out a free reset]\n");

  MqttCommandRouter router;
  constexpr uint32_t kNearWrap = 0xFFFFFFFFu - 1'000u;
  check(router.evaluate(MqttCommand::ResetTotals, "RESET", false, kNearWrap, kEpoch) ==
            MqttCommandResult::Accepted,
        "a reset just before the wrap is accepted");
  // 500 ms later in real time, but a smaller number: a magnitude comparison would read this as 49 days.
  check(router.evaluate(MqttCommand::ResetTotals, "RESET", false, kNearWrap + 500, kEpoch) ==
            MqttCommandResult::RateLimited,
        "and 500 ms later, across the wrap, it is still rate-limited");
}


void commandsMapToTheRegisterAMasterWouldWrite() {
  std::printf("\n[R4.4.3 — a reset from HA is the reset a Modbus master performs, not a private path]\n");

  // The whole value of R4.4.3 is that there is ONE reset implementation. The way that goes wrong is
  // not a wrong branch, it is a right-looking mapping to the wrong register — `reset-totals` wired to
  // the session register would pass every other test in this file, clear the wrong thing on a real
  // device, and read correctly to anyone who did not open register_map.h.
  check(MqttCommandRouter::registerFor(MqttCommand::ResetSession) == plc::REG_MASTER_RESET_ALL_SESSION,
        "reset-session writes register 22, the session reset");
  check(MqttCommandRouter::registerFor(MqttCommand::ResetTotals) == plc::REG_MASTER_RESET_ALL_MEASURED,
        "reset-totals writes register 21, which takes the lifetime total with it");
  check(plc::REG_MASTER_RESET_ALL_SESSION != plc::REG_MASTER_RESET_ALL_MEASURED,
        "and those are two different registers, so the check above means something");

  // republish is not a measurement command and must not reach the reset path at all.
  check(MqttCommandRouter::registerFor(MqttCommand::Republish) == 0,
        "republish maps to no register — it re-sends discovery, which no master can ask for");
  check(MqttCommandRouter::registerFor(MqttCommand::None) == 0, "and None maps to none");

  // The value is 1, not the 0x5AA5 the newer destructive registers use. Asserted because the magic is
  // the project's idiom and assuming it here would produce a reset that silently never happens:
  // `applyHoldingWrite` acts on `value == 1` for registers 20-23 and ignores everything else.
  check(MqttCommandRouter::kRegisterValue == 1,
        "the value written is 1 — registers 20-23 predate the 0x5AA5 idiom");
  check(MqttCommandRouter::kRegisterValue != 0x5AA5, "and is explicitly not the magic");
}

}  // namespace

int main() {
  std::printf("mqtt_command_router — §4.4.1's topics and R4.4.2's safeguards\n");
  topicsMapToCommands();
  destructiveCommandsDemandTheMagic();
  retainedIsDiscardedBeforeAnythingElse();
  theRateLimitIsPerKindAndOnMillis();
  thePersistedGuardSurvivesAReboot();
  resultsHaveWireWords();
  theWrapDoesNotReArmTheLimit();
  commandsMapToTheRegisterAMasterWouldWrite();
  if (failures > 0) {
    std::printf("\nFAILURES (%d of %d)\n", failures, checks);
    return 1;
  }
  std::printf("\nALL PASSED (%d checks)\n", checks);
  return 0;
}
