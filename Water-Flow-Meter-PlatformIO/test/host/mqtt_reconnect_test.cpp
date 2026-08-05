// Host tests for the MQTT reconnection policy (WiFi_MQTT_Connectivity R4.1.2 → §3.1.2).
//
// The entire content of R4.1.2 is a sequence of DURATIONS ending in a five-minute rung, so a bench
// cannot check it: reaching the ceiling takes ~9 minutes of an unplugged broker per run, and the
// jitter that R4.1.2 asks for is invisible to a single device. Here the clock is a parameter and
// the whole ladder runs in a few milliseconds of simulated hours.
//
// ── The blocker these tests were extended to catch ────────────────────────────────
//
// The first version of this policy armed its ladder from a CONNECTED → DISCONNECTED edge and sat
// in an `Idle` phase until one arrived. A broker that is unreachable from boot never produces that
// edge, so on the one device R4.1.2 is written for the ladder stayed at rest — and because
// `disable_auto_reconnect = true` had already switched esp-mqtt's own retry off, nothing retried at
// all. The old suite PASSED, because it asserted that resting behaviour on purpose
// ("the policy never starts a session of its own").
//
// So the sections below now begin from a cold start, which is the production path, and the
// never-connected case is measured as a ladder rather than asserted as silence.
//
// Two rules this file follows deliberately, both because this project has shipped checks that
// passed for the wrong reason:
//
//   1. every delay is MEASURED by ticking a clock until the policy asks for an attempt, and only
//      then compared with the delay it reported. Reading `retryDelayMs()` back proves nothing — a
//      policy that reported a perfect ladder and reconnected immediately would satisfy it;
//   2. every expectation is a LITERAL from §3.1.2, never derived from MqttReconnect's own
//      constants. A derived expectation cannot fail when the constant changes, which is exactly how
//      a five-minute ceiling turned into twenty minutes with wifi_manager_test still green.
#include "net/mqtt_reconnect.h"

// For the constants only, so the two ladders can be compared here as well as by the static_asserts
// in mqtt_reconnect.cpp. Nothing from WifiManager is instantiated or called, so this adds no object
// file to the link.
#include "net/wifi_manager.h"

#include <cstdio>

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const char* what) {
  ++checks;
  std::printf("  %-74s %s\n", what, condition ? "ok" : "FAIL");
  if (!condition) ++failures;
}

using plc::MqttReconnect;
using plc::WifiManager;

/** The simulated tick. 50 ms is finer than the shortest interval any assertion below cares about. */
constexpr uint32_t kTickMs = 50;

const uint8_t kMacA[6] = {0x24, 0x6F, 0x28, 0x1A, 0x2B, 0x3C};
const uint8_t kMacB[6] = {0x24, 0x6F, 0x28, 0x99, 0x88, 0x77};

/**
 * §3.1.2's ladder, written out. 1 s doubling, clamped at the five-minute ceiling from the tenth
 * rung on (1000 << 9 = 512000 is the first step past it).
 *
 * A literal table, not a loop over MqttReconnect::kInitialBackoffMs — see the note at the top.
 */
constexpr int kRungs = 12;
constexpr uint32_t kLadderMs[kRungs] = {1000,  2000,   4000,   8000,   16000,  32000,
                                        64000, 128000, 256000, 300000, 300000, 300000};

/** The rung an attempt following `failures` consecutive failures must use. Literal beyond the
 *  table's end, because §3.1.2's ceiling holds for every rung after the tenth. */
uint32_t rungFor(int consecutiveFailures) {
  return consecutiveFailures < kRungs ? kLadderMs[consecutiveFailures] : 300000u;
}

/** The bottom of the subtractive jitter band for a rung: `base - base/8`. Literal divisor. */
uint32_t bandFloor(uint32_t base) { return base - base / 8; }

/**
 * Ticks the policy until it asks for an attempt, and returns the simulated ms from `fromMs` to
 * that moment — 0 if it never asked within ~2000 s of simulated time.
 *
 * Unsigned subtraction, so this stays correct across the millis wrap the rollover test provokes.
 */
uint32_t msUntilAttempt(MqttReconnect& policy, uint32_t& nowMs, uint32_t fromMs) {
  for (uint32_t i = 0; i < 40000; ++i) {  // 40000 * 50 ms, well past the 300 s ceiling
    nowMs += kTickMs;
    policy.update(nowMs);
    if (policy.shouldAttemptNow()) return nowMs - fromMs;
  }
  return 0;
}

/**
 * Ticks `totalMs` and reports whether the policy asked for an attempt at any point.
 *
 * Needed because `shouldAttemptNow()` is true for one tick: an absence can only be observed by
 * watching every tick, and every check below of the form "this must ask for nothing" is about an
 * absence.
 */
bool anyAttemptWithin(MqttReconnect& policy, uint32_t& nowMs, uint32_t totalMs, uint32_t stepMs) {
  for (uint32_t elapsed = 0; elapsed < totalMs; elapsed += stepMs) {
    nowMs += stepMs;
    policy.update(nowMs);
    if (policy.shouldAttemptNow()) return true;
  }
  return false;
}

/** Climbs `count` rungs, discarding the measurements. Used to get a policy into a deep state. */
void climb(MqttReconnect& policy, uint32_t& nowMs, int count) {
  for (int i = 0; i < count; ++i) {
    const uint32_t startedAt = nowMs;
    msUntilAttempt(policy, nowMs, startedAt);
    policy.noteAttemptFailed();
  }
}

/** How an unreachable broker's refusal reaches the policy. Both happen on a real device. */
enum class Refusal {
  Disconnected, /**< esp-mqtt raised MQTT_EVENT_DISCONNECTED, as it does from a failed CONNECT. */
  Silent        /**< Nothing was reported at all — the case `kAttemptTimeoutMs` exists for. */
};

/**
 * Drives the whole production loop against a broker that never accepts: tick, answer the due edge,
 * refuse, repeat. Fills `gaps` with the interval between consecutive attempts and returns how many
 * attempts were actually asked for — a policy that stalls returns fewer than `count`.
 */
int collectGaps(MqttReconnect& policy, uint32_t& nowMs, uint32_t* gaps, int count, Refusal how) {
  int seen = 0;
  uint32_t previous = nowMs;
  for (int i = 0; i < count; ++i) {
    const uint32_t took = msUntilAttempt(policy, nowMs, previous);
    if (took == 0) return seen;  // it stopped asking
    gaps[seen++] = took;
    previous = nowMs;
    if (how == Refusal::Disconnected) policy.noteDisconnected();
  }
  return seen;
}

// ── Cold start ────────────────────────────────────────────────────────────────────

void coldStartTests() {
  std::printf("[cold start — a broker unreachable from boot is a rung, not a resting place]\n");

  MqttReconnect policy(kMacA);
  uint32_t now = 1000;

  check(policy.attempts() == 0 && policy.retryDelayMs() == 0,
        "a fresh policy has no failures and no wait");

  const uint32_t bootedAt = now;
  now += kTickMs;
  policy.update(now);
  check(policy.shouldAttemptNow(), "the FIRST tick asks for an attempt: nothing waits to be armed");
  // The check the blocker turns on. A ladder that armed on a disconnect NOTIFICATION could not
  // start here at all — `connected_` is false at boot and stays false, so there is no edge to
  // notify — and with `disable_auto_reconnect = true` the library will not retry either.
  check(now - bootedAt <= kTickMs && now - bootedAt < 875u,
        "and it is NOT delayed by the ladder: R4.1.2 rations retries, not the first try");

  policy.noteDisconnected();  // MQTT_EVENT_DISCONNECTED — esp-mqtt reports a failed CONNECT this way
  check(policy.attempts() == 1, "a broker that refuses the first attempt puts it on the counter");
  check(policy.retryDelayMs() >= 875u && policy.retryDelayMs() <= 1000u,
        "and the next attempt is scheduled one second out (§3.1.2's first rung)");

  // Twenty-two attempts of the real loop, from a boot that never once succeeds. This is the
  // scenario R4.1.2 is about, and the one the old contract could not reach.
  constexpr int kAttempts = 22;
  uint32_t gaps[kAttempts] = {};
  MqttReconnect cold(kMacA);
  uint32_t coldNow = 1000;
  const int seen = collectGaps(cold, coldNow, gaps, kAttempts, Refusal::Disconnected);

  bool everyGapIsItsRung = true;
  bool ceilingNeverExceeded = true;
  for (int i = 1; i < seen; ++i) {
    const uint32_t expected = rungFor(i - 1);
    if (gaps[i] < bandFloor(expected) || gaps[i] > expected + kTickMs) everyGapIsItsRung = false;
    if (gaps[i] > 300000u + kTickMs) ceilingNeverExceeded = false;
  }
  std::printf("      first gaps (ms): ");
  for (int i = 0; i < 6 && i < seen; ++i) std::printf("%u ", static_cast<unsigned>(gaps[i]));
  std::printf("...\n");

  check(seen == kAttempts,
        "it keeps asking for the whole run — §9 forbids giving up on an unreachable broker");
  check(seen > 0 && gaps[0] <= kTickMs, "the first of them is immediate");
  check(everyGapIsItsRung,
        "and each later one waits its own rung: 1,2,4,8,16,32,64,128,256 s then 300 s");
  check(ceilingNeverExceeded, "never longer than five minutes, however long the broker stays down");
  // One refusal was reported per attempt, including the last, so the counter is the number of
  // refusals and not, say, the number of rungs the ladder happens to have.
  check(cold.attempts() == kAttempts,
        "every refusal counted, so the ladder's position is the failure count, not a guess");
}

// ── A working broker ──────────────────────────────────────────────────────────────

void promptConnectTests() {
  std::printf("\n[a working broker — the ladder must not stand between a boot and a CONNACK]\n");

  MqttReconnect policy(kMacA);
  uint32_t now = 1000;
  const uint32_t bootedAt = now;

  const uint32_t waited = msUntilAttempt(policy, now, bootedAt);
  check(waited > 0 && waited <= kTickMs,
        "the attempt is due on the first tick after boot, not one second later");

  policy.noteConnected();  // MQTT_EVENT_CONNECTED
  check(policy.attempts() == 0 && policy.retryDelayMs() == 0,
        "a session that comes up leaves no ladder behind it");
  check(!anyAttemptWithin(policy, now, 600000, 1000),
        "and ten minutes with the session up asks for no reconnect at all");
}

// ── The client that says nothing ──────────────────────────────────────────────────

void silentClientTests() {
  std::printf("\n[a silent client — with the library's own retry disabled, WE are the only retry]\n");

  // Not one notification, ever: no CONNECTED, no DISCONNECTED, no ERROR. A broker that accepts the
  // TCP connection and black-holes the CONNECT looks like this, and so does a firmware whose event
  // wiring is wrong. With `disable_auto_reconnect = true` there is nothing else to fall back on, so
  // a policy that waits for a verdict here leaves MQTT down forever and reports nothing.
  constexpr int kAttempts = 16;
  uint32_t gaps[kAttempts] = {};
  MqttReconnect policy(kMacA);
  uint32_t now = 1000;
  const int seen = collectGaps(policy, now, gaps, kAttempts, Refusal::Silent);

  bool everyGapAllowsTheDeadline = true;
  bool everyGapIsDeadlinePlusRung = true;
  for (int i = 1; i < seen; ++i) {
    const uint32_t expected = 20000u + rungFor(i - 1);  // literal 20 s, then §3.1.2's rung
    if (gaps[i] < 20000u) everyGapAllowsTheDeadline = false;
    if (gaps[i] < bandFloor(expected) || gaps[i] > expected + kTickMs) {
      everyGapIsDeadlinePlusRung = false;
    }
  }

  check(seen == kAttempts, "the ladder runs anyway: an unanswered attempt is a failed attempt");
  check(everyGapAllowsTheDeadline,
        "no attempt is abandoned before its 20 s deadline, so nothing storms the broker");
  check(everyGapIsDeadlinePlusRung,
        "and the interval is that deadline plus the rung — the same ladder, one deadline slower");
  check(policy.attempts() == kAttempts - 1,
        "the failure count climbs without a single event, so §9 can still report the fault");
}

// ── The ladder ────────────────────────────────────────────────────────────────────

void ladderTests() {
  std::printf("\n[the ladder — R4.1.2 via §3.1.2: 1 s to a 5 min ceiling, with jitter]\n");

  MqttReconnect policy(kMacA);
  uint32_t now = 5000;
  policy.update(now);  // cold: this is the first attempt, and the caller answers it
  check(policy.shouldAttemptNow(), "the cold policy asks for its own first attempt");

  const uint32_t armedAt = now;
  policy.noteDisconnected();  // esp-mqtt's MQTT_EVENT_DISCONNECTED: the production failure edge.
  check(policy.attempts() == 1, "the first refusal puts one failure on the counter");
  check(policy.retryDelayMs() >= 875u && policy.retryDelayMs() <= 1000u,
        "and schedules the first rung inside 1 s's jitter band (§3.1.2)");

  // Each rung is both READ from the policy and MEASURED by ticking. Reading alone cannot prove the
  // policy waits; measuring alone cannot see the ceiling exactly, because a ticked measurement is
  // only accurate to one tick.
  uint32_t reported[kRungs] = {};
  uint32_t measured[kRungs] = {};
  for (int i = 0; i < kRungs; ++i) {
    reported[i] = policy.retryDelayMs();
    const uint32_t startedAt = i == 0 ? armedAt : now;
    measured[i] = msUntilAttempt(policy, now, startedAt);
    policy.noteAttemptFailed();
  }

  bool everyRungFired = true;
  bool everyRungWaited = true;
  bool matchesLadder = true;
  bool neverExceedsCeiling = true;
  bool growsBelowCeiling = true;
  bool jitterIsPresent = false;
  for (int i = 0; i < kRungs; ++i) {
    if (measured[i] == 0) everyRungFired = false;
    if (measured[i] < reported[i] || measured[i] > reported[i] + kTickMs) everyRungWaited = false;
    if (reported[i] > kLadderMs[i] || reported[i] < bandFloor(kLadderMs[i])) matchesLadder = false;
    if (reported[i] > 300000u) neverExceedsCeiling = false;
    if (reported[i] != kLadderMs[i]) jitterIsPresent = true;
    // Rung 9 (300000, floor 262500) still clears rung 8 (256000, ceiling 256000), so growth is
    // strict up to and including it. From rung 10 on every rung sits in the ceiling's band and two
    // neighbours may legitimately compare either way.
    if (i > 0 && i <= 9 && measured[i] <= measured[i - 1]) growsBelowCeiling = false;
  }
  std::printf("      rungs (ms): ");
  for (int i = 0; i < kRungs; ++i) std::printf("%u ", static_cast<unsigned>(reported[i]));
  std::printf("\n");

  check(everyRungFired, "every rung eventually asks for a reconnect — §9 forbids giving up");
  check(everyRungWaited, "and the delay the policy REPORTS is the delay it actually waits out");
  check(matchesLadder,
        "measured against §3.1.2's literal ladder: 1,2,4,8,16,32,64,128,256 s then 300 s");
  check(growsBelowCeiling, "the measured wait GROWS on every rung below the ceiling");
  check(neverExceedsCeiling, "and never exceeds five minutes (§3.1.2)");
  check(jitterIsPresent, "at least one rung is jittered off the bare doubling (R4.1.2)");
  check(measured[kRungs - 1] >= 262500u,
        "at the ceiling the wait is inside 300 s's jitter band, not below it");
}

// ── The one-tick edge ─────────────────────────────────────────────────────────────

void edgeTests() {
  std::printf("\n[the edge — one tick only, and an edge nobody answers must not stall]\n");

  MqttReconnect policy(kMacA);
  uint32_t now = 1000;
  policy.update(now);
  policy.noteDisconnected();
  const uint32_t armedAt = now;

  const uint32_t waited = msUntilAttempt(policy, now, armedAt);
  check(waited >= 875u && waited <= 1000u + kTickMs, "the second attempt comes due after ~1 s");
  check(policy.shouldAttemptNow(), "and the policy says so on the tick it came due");
  check(policy.retryDelayMs() == 0, "the wait is over, so no wait is reported any more");

  const uint32_t edgeAt = now;
  now += kTickMs;
  policy.update(now);
  check(!policy.shouldAttemptNow(),
        "for exactly ONE tick — a sticky edge would re-issue reconnect() every pass of the loop");
  check(policy.attempts() == 1,
        "and the ladder has not moved: the attempt is outstanding, not yet failed");

  // Nobody called esp_mqtt_client_reconnect(), and no event will ever arrive. Without a deadline of
  // its own the policy would sit in Attempting forever: an MQTT link that is never retried again
  // and never says so.
  uint32_t timedOutAfter = 0;
  for (uint32_t i = 0; i < 2000; ++i) {
    now += kTickMs;
    policy.update(now);
    if (policy.attempts() == 2) {
      timedOutAfter = now - edgeAt;
      break;
    }
  }
  check(timedOutAfter >= 20000u && timedOutAfter <= 20000u + kTickMs,
        "an unanswered attempt counts as failed 20 s later, so the ladder keeps running");
  check(MqttReconnect::kAttemptTimeoutMs == 20000u,
        "which is this class's documented 20 s — twice the transport's 10 s network timeout");

  const uint32_t startedAt = now;
  const uint32_t second = msUntilAttempt(policy, now, startedAt);
  check(second >= 1750u && second <= 2000u + kTickMs,
        "and it resumes at the SECOND rung, 2 s, rather than starting the climb over");
}

// ── Duplicated and paired events ──────────────────────────────────────────────────

void duplicateEventTests() {
  std::printf("\n[duplicate events — esp-mqtt's task repeats itself; the ladder must not]\n");

  MqttReconnect policy(kMacA);
  uint32_t now = 1000;
  policy.update(now);
  const uint32_t armedAt = now;
  policy.noteDisconnected();
  check(policy.attempts() == 1, "the first refusal arms the ladder at one failure");

  // Repeats arrive from another task. One rung per EVENT would put a flaky minute at the ceiling.
  policy.noteDisconnected();
  policy.update(now += kTickMs);
  policy.noteDisconnected();
  policy.noteDisconnected();
  check(policy.attempts() == 1,
        "three more while the rung is still being waited out change nothing");

  const uint32_t waited = msUntilAttempt(policy, now, armedAt);
  check(waited >= 875u && waited <= 1000u + kTickMs,
        "so the wait is still the FIRST rung's second, measured from the first event");

  // MQTT_EVENT_ERROR then MQTT_EVENT_DISCONNECTED is the ORDINARY pair for a transport fault and
  // for a refused CONNACK alike. A handler that decodes `connect_return_code` may report the first
  // as noteAttemptFailed(); the DISCONNECTED that follows must not cost a second rung.
  MqttReconnect paired(kMacA);
  uint32_t pairedNow = 1000;
  paired.update(pairedNow);  // the cold attempt, now outstanding
  paired.noteAttemptFailed();
  paired.noteDisconnected();
  check(paired.attempts() == 1, "ERROR then DISCONNECTED for one failure is ONE rung, not two");
  const uint32_t pairWait = msUntilAttempt(paired, pairedNow, pairedNow);
  check(pairWait >= 875u && pairWait <= 1000u + kTickMs, "and the retry is 1 s out, not 2 s");
}

// ── Recovery ──────────────────────────────────────────────────────────────────────

void recoveryTests() {
  std::printf("\n[recovery — a connection that works forgets the climb it took to get there]\n");

  MqttReconnect policy(kMacA);
  uint32_t now = 1000;
  policy.update(now);
  policy.noteDisconnected();
  climb(policy, now, 12);
  check(policy.retryDelayMs() >= 262500u,
        "an unreachable broker leaves the ladder at the five-minute ceiling");

  policy.noteConnected();
  check(policy.attempts() == 0, "a successful connection clears the failure counter");
  check(policy.retryDelayMs() == 0, "and ends the wait");
  check(!anyAttemptWithin(policy, now, 600000, 1000),
        "ten minutes with the session up asks for no reconnect at all");

  // The measurement that matters, and the one that fails if a 'success' left the counter alone:
  // R3.1.3's reasoning applied to the broker — a session that demonstrably worked a moment ago must
  // not inherit the ceiling that an earlier misconfiguration climbed to.
  const uint32_t droppedAt = now;
  policy.noteDisconnected();
  const uint32_t waited = msUntilAttempt(policy, now, droppedAt);
  check(waited >= 875u && waited <= 1000u + kTickMs,
        "a session lost afterwards retries in ONE second, not in the 300 it had reached");
}

// ── reset() ───────────────────────────────────────────────────────────────────────

void resetTests() {
  std::printf("\n[reset — a client restarted because WiFi returned owes the broker nothing]\n");

  MqttReconnect policy(kMacA);
  uint32_t now = 1000;
  policy.update(now);
  policy.noteDisconnected();
  climb(policy, now, 12);
  check(policy.attempts() == 13 && policy.retryDelayMs() >= 262500u,
        "the ladder is deep: thirteen failures against an unreachable broker");

  policy.reset();
  check(policy.attempts() == 0 && policy.retryDelayMs() == 0, "reset() forgets all of it");

  // Not "asks for nothing until the next disconnect", which is what this used to assert and what
  // made the boot case unreachable. reset() puts the policy back where a fresh one starts, and a
  // fresh one attempts at once — otherwise the five-minute hole reset() exists to close is still
  // there, just now measured from the WiFi edge.
  const uint32_t resetAt = now;
  const uint32_t waited = msUntilAttempt(policy, now, resetAt);
  check(waited > 0 && waited <= kTickMs,
        "and attempts on the very next tick: WiFi came back, the broker owes us nothing");

  policy.noteDisconnected();
  const uint32_t next = msUntilAttempt(policy, now, now);
  check(next >= 875u && next <= 1000u + kTickMs,
        "so the next real failure starts the ladder again at one second");

  // A late DISCONNECTED from the session reset() abandoned arrives in another task's time, i.e.
  // after the reset. It describes a client this policy no longer owns and must not delay the fresh
  // attempt by a rung.
  MqttReconnect stale(kMacA);
  uint32_t staleNow = 1000;
  stale.update(staleNow);
  stale.noteDisconnected();
  climb(stale, staleNow, 4);
  stale.reset();
  stale.noteDisconnected();
  check(stale.attempts() == 0, "a stale disconnect landing after reset() is not a new failure");
  const uint32_t afterStale = msUntilAttempt(stale, staleNow, staleNow);
  check(afterStale > 0 && afterStale <= kTickMs, "and does not stand in the way of the next try");
}

// ── Notifications nobody asked for ────────────────────────────────────────────────

void strayNotificationTests() {
  std::printf("\n[stray notifications — a claim about an attempt that was never made]\n");

  // noteAttemptFailed() means "the attempt I asked for failed". With no attempt outstanding there
  // is nothing it could truthfully be about, so it must neither count nor delay anything.
  MqttReconnect stray(kMacA);
  uint32_t now = 1000;
  stray.noteAttemptFailed();
  check(stray.attempts() == 0, "noteAttemptFailed() before any attempt is ignored");
  const uint32_t waited = msUntilAttempt(stray, now, now);
  check(waited > 0 && waited <= kTickMs, "and the first attempt is still immediate");

  // The same claim from a connected policy: the publisher's view of the session and this one's must
  // not be allowed to disagree because a caller invented a failure.
  MqttReconnect live(kMacA);
  uint32_t liveNow = 1000;
  live.update(liveNow);
  live.noteConnected();
  live.noteAttemptFailed();
  check(live.attempts() == 0, "and from a live session it is ignored too");
  check(!anyAttemptWithin(live, liveNow, 600000, 1000),
        "so an invented failure cannot reconnect a client that is up");
}

// ── The figures themselves ────────────────────────────────────────────────────────

void figureTests() {
  std::printf("\n[the figures — §3.1.2's numbers, and ONE backoff behaviour rather than two]\n");

  // Pinned as literals. Every measured check above compares against kLadderMs for the same reason:
  // an expectation derived from the class's own constants cannot fail when those constants change,
  // and that is precisely how a five-minute ceiling became twenty minutes with the WiFi suite green.
  check(MqttReconnect::kInitialBackoffMs == 1000u, "the ladder starts at one second (§3.1.2)");
  check(MqttReconnect::kMaxBackoffMs == 300000u, "and its ceiling IS five minutes (§3.1.2)");
  check(MqttReconnect::kBackoffJitterDivisor == 8u, "with a one-eighth subtractive jitter band");

  // R4.1.2 does not say "a backoff", it says "the SAME backoff policy as §3.1.2". mqtt_reconnect.cpp
  // static_asserts this too, so drift is a build failure; this makes it visible in the run as well.
  check(MqttReconnect::kInitialBackoffMs == WifiManager::kInitialBackoffMs &&
            MqttReconnect::kMaxBackoffMs == WifiManager::kMaxBackoffMs &&
            MqttReconnect::kBackoffJitterDivisor == WifiManager::kBackoffJitterDivisor,
        "and all three agree with WifiManager's, so the device has one backoff, not two");
}

// ── Jitter ────────────────────────────────────────────────────────────────────────

void collectRungs(const uint8_t mac[6], uint32_t* out, int count) {
  MqttReconnect policy(mac);
  uint32_t now = 1000;
  policy.update(now);
  policy.noteDisconnected();
  for (int i = 0; i < count; ++i) {
    out[i] = policy.retryDelayMs();
    const uint32_t startedAt = now;
    msUntilAttempt(policy, now, startedAt);
    policy.noteAttemptFailed();
  }
}

void jitterTests() {
  std::printf("\n[jitter — one power cut must not point a whole site at the broker in step]\n");

  uint32_t rungsA[kRungs] = {};
  uint32_t rungsB[kRungs] = {};
  collectRungs(kMacA, rungsA, kRungs);
  collectRungs(kMacB, rungsB, kRungs);

  bool devicesDiffer = false;
  bool bothInBand = true;
  for (int i = 0; i < kRungs; ++i) {
    if (rungsA[i] != rungsB[i]) devicesDiffer = true;
    if (rungsA[i] > kLadderMs[i] || rungsA[i] < bandFloor(kLadderMs[i])) bothInBand = false;
    if (rungsB[i] > kLadderMs[i] || rungsB[i] < bandFloor(kLadderMs[i])) bothInBand = false;
  }
  check(devicesDiffer,
        "two MACs draw different jitter, so two meters do not reconnect in lockstep (R4.1.2)");
  check(bothInBand, "and both stay inside the band of every literal rung, never above it");

  // The three ceiling rungs share one base, so if they are all equal the jitter was drawn once and
  // reused — which at the ceiling is where R4.1.2's storm actually happens.
  check(!(rungsA[9] == rungsA[10] && rungsA[10] == rungsA[11]),
        "jitter is re-drawn per rung, so meters that saturate together still spread out");

  // The subtractive direction, stated as its own check: additive jitter at the ceiling would have to
  // either exceed §3.1.2's five minutes or be clamped flat exactly where spreading matters most.
  const bool ceilingNeverAbove =
      rungsA[9] <= 300000u && rungsA[10] <= 300000u && rungsA[11] <= 300000u;
  const bool ceilingReallyJittered =
      rungsA[9] < 300000u || rungsA[10] < 300000u || rungsA[11] < 300000u;
  check(ceilingNeverAbove && ceilingReallyJittered,
        "and SUBTRACTIVE: the ceiling rungs come off five minutes, never past it");
}

// ── Rollover ──────────────────────────────────────────────────────────────────────

void rolloverTests() {
  std::printf("\n[millis rollover — 49.7 days in, a rung must not park for another 49.7]\n");

  MqttReconnect policy(kMacA);
  uint32_t now = 0xFFFFFE00u;  // ~512 ms before the counter wraps, so the first rung straddles it
  policy.update(now);
  const uint32_t armedAt = now;
  policy.noteDisconnected();

  const uint32_t waited = msUntilAttempt(policy, now, armedAt);
  check(now < 0xFFFFFE00u, "the clock has wrapped past zero");
  check(waited >= 875u && waited <= 1000u + kTickMs,
        "and the rung still fires on time: an unsigned compare would wait 49.7 days");
}

}  // namespace

int main() {
  std::printf("plc::MqttReconnect — R4.1.2's exponential backoff, measured\n\n");
  coldStartTests();
  promptConnectTests();
  silentClientTests();
  ladderTests();
  edgeTests();
  duplicateEventTests();
  recoveryTests();
  resetTests();
  strayNotificationTests();
  figureTests();
  jitterTests();
  rolloverTests();
  std::printf("\n%s (%d checks, %d failures)\n", failures == 0 ? "ALL PASSED" : "FAILURES", checks,
              failures);
  return failures == 0 ? 0 : 1;
}
