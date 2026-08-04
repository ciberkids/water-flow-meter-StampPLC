// Host tests for the configuration portal's HTTP task placement policy (WiFi §7.6, R2.1.0, R2.1.4).
//
// There is no state machine here to exercise — the deliverable is a handful of numbers. So what is
// worth asserting is not "the numbers are the numbers", which would pass at rest and prove nothing.
// It is the two things that actually go wrong:
//
//   1. that the predicates DISCRIMINATE — that they reject esp_http_server's shipped defaults
//      (priority 5, tskNO_AFFINITY) and accept the chosen ones, rather than being vacuously true;
//   2. that the policy survives the core-layout swap the core-affinity review is contemplating,
//      which is why every predicate takes the polling core / priority as a parameter and is driven
//      here with a layout the firmware does not currently have.
//
// The header also carries static_asserts on its own constants. Those are a compile-time gate, not
// checks — mutating kTaskPriority breaks the BUILD, which is the intent, but it is not evidence that
// anything below can fail. The predicate cases are what carry that burden.
#include "net/httpd_task_policy.h"

#include <cstdio>

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const char* what) {
  ++checks;
  std::printf("  %-78s %s\n", what, condition ? "ok" : "FAIL");
  if (!condition) ++failures;
}

namespace pol = plc::httpd_policy;

/**
 * The reason the header exists: HTTPD_DEFAULT_CONFIG() is unsafe on this device.
 *
 * These are the negative cases that give every other check meaning. A predicate that could not
 * reject priority 5 or tskNO_AFFINITY would be documentation, not a test.
 */
void sdkDefaultsAreUnsafeTests() {
  std::printf("esp_http_server's shipped defaults, measured against the policy\n");

  check(pol::canDelayPolling(pol::kSdkDefaultTaskPriority),
        "HTTPD_DEFAULT_CONFIG's priority 5 can delay the priority-2 polling task");
  check(!pol::priorityIsAdmissible(pol::kSdkDefaultTaskPriority),
        "...so priority 5 is outside the admissible band");
  check(pol::violatesCoreDedication(pol::kSdkDefaultCoreId),
        "HTTPD_DEFAULT_CONFIG's tskNO_AFFINITY violates core dedication (R2.1.0)");

  // The raw value is spelled out HERE rather than read from the header, so what is under test is
  // whether the header still recognises IDF 4.4's tskNO_AFFINITY. Comparing the header's constant
  // against itself would pass however wrong it became — and it can become wrong: IDF 5.x redefines
  // tskNO_AFFINITY to -1, so a toolchain bump is a live way for this to break.
  check(pol::violatesCoreDedication(0x7FFFFFFF),
        "IDF 4.4's literal tskNO_AFFINITY 0x7FFFFFFF is still the value the policy rejects");

  // The unpinned case must be caught by VALUE, not by "is it not 0 or 1". A policy that only tested
  // for the polling core's number would wave tskNO_AFFINITY straight through.
  check(pol::violatesCoreDedication(pol::kNoAffinity, /*pollingCore=*/1),
        "tskNO_AFFINITY is rejected whichever core polling sits on");

  // lwIP's tiT, priority 18 (§2.1.3) — the task this project already concluded it cannot rescue.
  // Present so the upper bound is exercised well beyond the boundary.
  check(pol::canDelayPolling(18), "lwIP tiT's priority 18 can delay polling — §2.1.3's known gap");
  std::printf("\n");
}

/** The chosen numbers, checked against the band rather than against themselves. */
void chosenPolicyTests() {
  std::printf("The policy this header fixes\n");

  check(!pol::canDelayPolling(pol::kTaskPriority),
        "the chosen HTTP priority cannot delay polling on either core");
  check(pol::priorityIsAdmissible(pol::kTaskPriority),
        "the chosen HTTP priority is inside the admissible band");
  check(!pol::violatesCoreDedication(pol::kTaskCore),
        "the chosen core is not the polling core and is not tskNO_AFFINITY");

  // R2.1.4: below Modbus. Expressed by reusing the same predicate with Modbus as the thing being
  // protected, so there is one definition of "cannot delay" rather than two.
  check(!pol::canDelayPolling(pol::kTaskPriority, pol::kModbusServerPriority),
        "the HTTP task cannot delay the priority-8 Modbus server (R2.1.4)");
  check(pol::canDelayPolling(pol::kSdkDefaultTaskPriority, pol::kIdlePriority + 1),
        "...whereas the SDK default would, against any priority it exceeds");
  std::printf("\n");
}

/** The boundaries. Both sides of every one, because only one side is interesting per operator. */
void bandBoundaryTests() {
  std::printf("The admissible band, at its edges\n");

  check(pol::canDelayPolling(pol::kPollingTaskPriority),
        "EQUAL priority counts as delaying: time slicing halves the polling loop's turns");
  check(!pol::canDelayPolling(pol::kPollingTaskPriority - 1),
        "one below the polling priority does not");
  check(!pol::priorityIsAdmissible(pol::kPollingTaskPriority),
        "the polling priority itself is not admissible");
  check(pol::priorityIsAdmissible(pol::kPollingTaskPriority - 1),
        "one below it is");

  check(!pol::priorityIsAdmissible(pol::kIdlePriority),
        "the idle priority is excluded: starving idle on a WDT-checked core is a panic");

  // The band is exclusive at BOTH ends, so with polling at 2 exactly one value fits. Swept across
  // the WHOLE scheduler range rather than a range chosen to make the answer come out right — the
  // claim in the label is about every priority FreeRTOS offers, so the loop has to be too.
  int admissible = 0;
  for (unsigned p = 0; p < pol::kMaxPriorities; ++p) {
    if (pol::priorityIsAdmissible(p)) ++admissible;
  }
  check(admissible == 1,
        "of all 25 FreeRTOS priorities exactly one is admissible today — no slack at all");
  std::printf("\n");
}

/** Would the policy still be correct if the review moved the polling task to core 1? */
void layoutSwapTests() {
  std::printf("Robustness to the core-layout swap under review\n");

  check(pol::otherCoreThan(0) == 1, "otherCoreThan(0) is 1");
  check(pol::otherCoreThan(1) == 0, "otherCoreThan(1) is 0");
  check(pol::otherCoreThan(pol::otherCoreThan(0)) == 0, "otherCoreThan is its own inverse at 0");
  check(pol::otherCoreThan(pol::otherCoreThan(1)) == 1, "otherCoreThan is its own inverse at 1");
  check(pol::kTaskCore == pol::otherCoreThan(pol::kPollingTaskCore),
        "the HTTP core is derived from the polling core, not written as a literal");

  // Drive the swapped layout explicitly: polling on core 1.
  check(pol::violatesCoreDedication(1, /*pollingCore=*/1),
        "with polling on core 1, pinning the HTTP task to core 1 violates dedication");
  check(!pol::violatesCoreDedication(0, /*pollingCore=*/1),
        "...and core 0 becomes the correct place for it");
  check(!pol::violatesCoreDedication(pol::otherCoreThan(1), /*pollingCore=*/1),
        "the derived core is right under the swapped layout too");

  // And a swapped PRIORITY layout: the band has to move with it, or the assert is decoration.
  check(pol::layoutAdmitsAnyPriority(pol::kPollingTaskPriority),
        "today's polling priority leaves at least one admissible HTTP priority");
  check(!pol::layoutAdmitsAnyPriority(1),
        "lowering polling to 1 would leave NO admissible priority — a design decision, not a nudge");
  check(!pol::layoutAdmitsAnyPriority(pol::kIdlePriority),
        "nor would polling at the idle priority");
  check(pol::priorityIsAdmissible(4, /*pollingPriority=*/5),
        "raise polling to 5 and priority 4 becomes admissible: the band tracks the layout");
  check(!pol::priorityIsAdmissible(4, /*pollingPriority=*/2),
        "...and is still refused under the layout the firmware actually has");
  std::printf("\n");
}

}  // namespace

int main() {
  std::printf("plc::httpd_policy — where the §7.6 portal's HTTP task may run\n\n");
  sdkDefaultsAreUnsafeTests();
  chosenPolicyTests();
  bandBoundaryTests();
  layoutSwapTests();
  std::printf("%s (%d checks, %d failures)\n", failures == 0 ? "ALL PASSED" : "FAILURES", checks,
              failures);
  return failures == 0 ? 0 : 1;
}
