/**
 * The NTP policy: when the device asks the network for the time (R7.13).
 *
 * Arduino-free on purpose, like `device_clock_test.cpp` beside it. "Sync on connect" reads as one line and
 * is four decisions — only while associated, once per association, retry a failure without hammering, and
 * re-sync before drift matters — and none of them is observable on a bench without waiting six hours.
 */
#include <cstdio>

#include "time/ntp_policy.h"

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const char* what) {
  ++checks;
  std::printf("  %-74s %s\n", what, condition ? "ok" : "FAIL");
  if (!condition) failures++;
}

void nothingHappensBeforeAssociation() {
  std::printf("\n[no association, no request — SNTP against a powered-down radio spends the ladder]\n");

  plc::NtpPolicy policy;
  check(!policy.consumeDueRequest(0), "a cold policy asks for nothing");
  check(!policy.consumeDueRequest(60'000), "and still nothing a minute later");
  check(!policy.everSucceeded(), "and has never succeeded");
}

void associationAsksImmediatelyAndOnce() {
  std::printf("\n[the association edge asks at once, and exactly once]\n");

  plc::NtpPolicy policy;
  policy.noteAssociated(5'000);
  check(policy.consumeDueRequest(5'000),
        "the request is due the instant the link comes up — the first chance a dead RTC has");
  check(policy.requestInFlight(), "and is marked in flight");
  check(!policy.consumeDueRequest(5'001),
        "a second call gets nothing: the logic task runs this hundreds of times a second");
  check(!policy.consumeDueRequest(500'000), "and nothing later either, while the first is unanswered");
}

void aFailureRetriesLaterNotSooner() {
  std::printf("\n[a failure waits two minutes — DNS that did not answer will not answer faster]\n");

  plc::NtpPolicy policy;
  policy.noteAssociated(1'000);
  check(policy.consumeDueRequest(1'000), "the first request goes out");
  policy.noteFailed(2'000);
  check(!policy.requestInFlight(), "the failure clears the in-flight flag");
  check(!policy.consumeDueRequest(2'000), "and nothing is due immediately");
  check(!policy.consumeDueRequest(121'000), "nor one millisecond early");
  check(policy.consumeDueRequest(122'000), "but at two minutes the retry goes out");
  check(!policy.everSucceeded(), "and the policy still reports no success — the panel must not claim one");
}

void aSuccessSchedulesTheNextSync() {
  std::printf("\n[a success schedules the next sync six hours out, because the RTC drifts]\n");

  plc::NtpPolicy policy;
  policy.noteAssociated(1'000);
  policy.consumeDueRequest(1'000);
  policy.noteSucceeded(3'000);
  check(policy.everSucceeded(), "success is recorded");
  check(policy.lastSuccessMs() == 3'000, "with when it happened");
  check(!policy.consumeDueRequest(3'000), "nothing is due straight after");
  check(!policy.consumeDueRequest(3'000 + plc::NtpPolicy::kResyncIntervalMs - 1),
        "nor a millisecond before the interval");
  check(policy.consumeDueRequest(3'000 + plc::NtpPolicy::kResyncIntervalMs),
        "and at six hours the device asks again — a clock synced once at commissioning drifts for years");
}

void losingTheLinkAbandonsTheRequest() {
  std::printf("\n[losing the link abandons the request rather than answering it against a dead radio]\n");

  plc::NtpPolicy policy;
  policy.noteAssociated(1'000);
  check(policy.consumeDueRequest(1'000), "a request is in flight");
  policy.noteDisassociated();
  check(!policy.requestInFlight(), "dropping the link clears it");
  check(!policy.consumeDueRequest(2'000), "and nothing is due while disassociated");

  // Re-association is a fresh opportunity, not a resumption: the device may have moved networks.
  policy.noteAssociated(10'000);
  check(policy.consumeDueRequest(10'000), "re-associating asks again at once");
}

void silenceBecomesAFailure() {
  std::printf("\n[silence for fifteen seconds is a failure — SNTP answers on its own schedule or not at all]\n");

  plc::NtpPolicy policy;
  policy.noteAssociated(1'000);
  check(policy.consumeDueRequest(1'000), "a request goes out");
  check(!policy.consumeTimeout(1'000), "and is not timed out at once");
  check(!policy.consumeTimeout(1'000 + plc::NtpPolicy::kRequestTimeoutMs - 1),
        "nor a millisecond early");
  check(policy.consumeTimeout(1'000 + plc::NtpPolicy::kRequestTimeoutMs),
        "but at fifteen seconds the silence counts as a failure");
  check(!policy.requestInFlight(), "which clears the in-flight flag");
  check(!policy.consumeTimeout(500'000),
        "and cannot fire twice — one decision, expressed once, so the retry cadence is not doubled");

  // The timeout schedules the retry itself, so a caller cannot express half the decision.
  check(!policy.consumeDueRequest(1'000 + plc::NtpPolicy::kRequestTimeoutMs),
        "the retry is not due immediately after the timeout");
  check(policy.consumeDueRequest(1'000 + plc::NtpPolicy::kRequestTimeoutMs + plc::NtpPolicy::kRetryMs),
        "but is due two minutes later, on the failure cadence");
}

void theScheduleSurvivesTheMillisWrap() {
  std::printf("\n[the 49.7-day millis wrap, which this device is meant to outlive]\n");

  plc::NtpPolicy policy;
  // Associate just before the wrap and succeed there, so the next due time overflows past zero.
  constexpr uint32_t kNearWrap = 0xFFFFFFFFu - 1'000u;
  policy.noteAssociated(kNearWrap);
  check(policy.consumeDueRequest(kNearWrap), "a request goes out just before the wrap");
  policy.noteSucceeded(kNearWrap);

  const uint32_t due = kNearWrap + plc::NtpPolicy::kResyncIntervalMs;  // wraps
  check(due < kNearWrap, "the next due time really has wrapped past zero");
  check(!policy.consumeDueRequest(due - 1), "and is not due early on the far side of the wrap");
  check(policy.consumeDueRequest(due),
        "but is due on time — the comparison is signed-difference, not a magnitude test");
}

}  // namespace

int main() {
  std::printf("ntp_policy — when the device asks the network for the time (R7.13)\n");
  nothingHappensBeforeAssociation();
  associationAsksImmediatelyAndOnce();
  aFailureRetriesLaterNotSooner();
  aSuccessSchedulesTheNextSync();
  losingTheLinkAbandonsTheRequest();
  silenceBecomesAFailure();
  theScheduleSurvivesTheMillisWrap();
  if (failures > 0) {
    std::printf("\nFAILURES (%d of %d)\n", failures, checks);
    return 1;
  }
  std::printf("\nALL PASSED (%d checks)\n", checks);
  return 0;
}
