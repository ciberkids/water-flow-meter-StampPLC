// Host tests for the core layout and the configuration portal's HTTP task placement policy
// (WiFi_MQTT_Connectivity §7.6, §2.1.3, R2.1.0, R2.1.4, R4.1.5).
//
// ── What is worth asserting here, and what is not ────────────────────────────────────
//
// There is no state machine to exercise — the deliverable is a handful of numbers and four
// predicates. The previous version of this file got that wrong in a way worth recording, because it
// is the same wrong turn the project keeps taking:
//
//   * Nine of its thirty checks re-stated an expression an identical `static_assert` in the header
//     already gated — `check(priorityIsAdmissible(kTaskPriority))` next to
//     `static_assert(priorityIsAdmissible(kTaskPriority))`. Every mutation that could redden the
//     check stops the build first, so the check could not report FAIL in any binary that existed.
//     They are gone. Anything a `static_assert` over the header's own constants can state belongs to
//     the compiler, not to a test.
//   * One check counted: "of all 25 FreeRTOS priorities exactly one is admissible today". That is the
//     project's fifth count-not-contract assertion — it would have failed the first time the layout
//     legitimately changed, and it says nothing about the rule. It is replaced by the rule: the band
//     is the open interval (idle, polling), swept over every layout the scheduler admits.
//   * Two checks were tautologies dressed as tests. `kTaskCore == otherCoreThan(kPollingTaskCore)` is
//     how `kTaskCore` is *defined*, and its label claimed the core was "not written as a literal",
//     which no runtime check can see: `otherCoreThan(0)` and `1` are the same value. Gone, with the
//     reason written into the header at `kTaskCore` instead. `violatesCoreDedication(kNoAffinity, 1)`
//     compared the constant with itself; it now passes the raw `0x7FFFFFFF`, which is the thing that
//     can actually drift.
//
// What is left is the three things that can genuinely go wrong:
//
//   1. THE QUOTED FACTS DRIFT. The header quotes numbers out of the installed SDK and out of
//      eModbus. A toolchain bump can change them (IDF 5.x redefines `tskNO_AFFINITY` to -1). Pinned
//      against literals here, with the file and line each was read from.
//   2. A PREDICATE STOPS DISCRIMINATING. Driven over the whole scheduler range and both core
//      layouts, as universally-quantified contracts — not at the one point today's constants happen
//      to occupy, which is where the `static_assert`s already stand.
//   3. THE LAYOUT AND THE FIRMWARE DRIFT APART. `core_layout.h` says the polling task is priority 2
//      on core 0; `firmware.cpp` is what actually creates it. This file reads `src/firmware.cpp` and
//      reconciles the two by execution, which is the only thing that closes the hole review found:
//      mutating the polling core used to leave the entire suite green.
#include "core_layout.h"           // the layout under test, and the scheduler constants it quotes
#include "net/httpd_task_policy.h"  // the policy derived from it

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const char* what) {
  ++checks;
  std::printf("  %-78s %s\n", what, condition ? "ok" : "FAIL");
  if (!condition) ++failures;
}

namespace pol = plc::httpd_policy;
namespace lay = plc::core_layout;

// ── The facts quoted out of the toolchain ──────────────────────────────────────────
//
// Each of these was re-read in the installed SDK on 2026-08-05 at the line named in its label. They
// are pinned against literals written HERE rather than against the header's own constants, because a
// constant compared with itself passes however wrong it has become — and these are exactly the
// numbers a toolchain bump changes underneath us.
void quotedSdkFactTests() {
  std::printf("The facts the policy is quoted from, pinned against the installed toolchain\n");

  check(pol::kSdkDefaultTaskPriority == 5,
        "esp_http_server.h:27 — HTTPD_DEFAULT_CONFIG's task_priority is tskIDLE_PRIORITY+5, i.e. 5");
  check(lay::kNoAffinity == 0x7FFFFFFF && pol::kSdkDefaultCoreId == 0x7FFFFFFF,
        "task.h:70 / esp_http_server.h:29 — tskNO_AFFINITY is 0x7FFFFFFF, not IDF 5.x's -1");
  check(lay::kIdlePriority == 0 && lay::kMaxPriorities == 25,
        "task.h:189 tskIDLE_PRIORITY 0, FreeRTOSConfig.h:81 configMAX_PRIORITIES 25");

  // Not ours to choose, unlike the core: eModbus writes the priority as a literal in both branches of
  // ModbusServerRTU::doBegin(), with no parameter reaching it. R2.1.4 makes it the ceiling every
  // network task in this firmware is placed under, so it is pinned to the library it came from rather
  // than remembered.
  check(lay::kModbusServerPriority == 8,
        "eModbus ModbusServerRTU.cpp:100-101 — the RTU server task is hardcoded at priority 8");
  std::printf("\n");
}

// ── canDelayPolling, over the whole scheduler range ────────────────────────────────
//
// The rule is three-part and each part is asserted for EVERY layout FreeRTOS admits, not for the one
// pair of numbers the firmware currently uses. That is what makes these fail on a semantic edit:
// flipping `>=` to `>` in the header reddens the equal-priority case below, and none of the header's
// static_asserts notice, because priority 5 still outranks 2 either way.
void canDelayPollingTests() {
  std::printf("canDelayPolling — the rule, across all %u FreeRTOS priorities\n",
              lay::kMaxPriorities);

  bool higherAlwaysDelays = true;
  for (unsigned pp = 0; pp < lay::kMaxPriorities; ++pp) {
    for (unsigned p = pp + 1; p < lay::kMaxPriorities; ++p) {
      if (!pol::canDelayPolling(p, pp)) higherAlwaysDelays = false;
    }
  }
  check(higherAlwaysDelays,
        "a strictly higher priority preempts polling, at every polling priority");

  // The case the `>=` exists for. An equal priority does not preempt — it round-robins under
  // configUSE_TIME_SLICING, costing the polling loop about half its scheduling opportunities, which
  // for a loop whose job is observing edges in real time differs in degree and not in kind.
  bool equalDelays = true;
  for (unsigned pp = 0; pp < lay::kMaxPriorities; ++pp) {
    if (!pol::canDelayPolling(pp, pp)) equalDelays = false;
  }
  check(equalDelays, "an EQUAL priority counts as delaying: time slicing halves polling's turns");

  bool lowerNeverDelays = true;
  for (unsigned pp = 0; pp < lay::kMaxPriorities; ++pp) {
    for (unsigned p = 0; p < pp; ++p) {
      if (pol::canDelayPolling(p, pp)) lowerNeverDelays = false;
    }
  }
  check(lowerNeverDelays, "a strictly lower priority never does — which is §2.1.3's whole lever");

  // A sweep that covered nothing would pass. This is the witness that it covered the case the header
  // exists for: HTTPD_DEFAULT_CONFIG's 5 sits strictly inside the "higher always delays" region under
  // the layout the firmware actually has.
  check(pol::kSdkDefaultTaskPriority > lay::kPollingTaskPriority &&
            pol::kSdkDefaultTaskPriority < lay::kMaxPriorities,
        "...and the sweep covered HTTPD_DEFAULT_CONFIG's priority 5 against the polling task's");
  std::printf("\n");
}

// ── violatesCoreDedication, over both core layouts ─────────────────────────────────
//
// Every case is driven with an explicit `pollingCore`, including the layout the firmware does not
// have, because the core-affinity review is contemplating that swap and a policy that only worked for
// today's layout would be found out by the review rather than by this file.
void coreDedicationTests() {
  std::printf("violatesCoreDedication — R2.1.0, under either core layout\n");

  bool pollingCoreRefused = true;
  bool otherCoreAllowed = true;
  bool unpinnedRefused = true;
  for (int pollingCore = 0; pollingCore <= 1; ++pollingCore) {
    if (!pol::violatesCoreDedication(pollingCore, pollingCore)) pollingCoreRefused = false;
    if (pol::violatesCoreDedication(lay::otherCoreThan(pollingCore), pollingCore)) {
      otherCoreAllowed = false;
    }
    // The literal, not lay::kNoAffinity: comparing the header's constant against itself is a
    // tautology, and the drift this guards against is precisely that constant changing (IDF 5.x).
    if (!pol::violatesCoreDedication(0x7FFFFFFF, pollingCore)) unpinnedRefused = false;
  }
  check(pollingCoreRefused, "pinning to the polling core is refused, whichever core that is");
  check(otherCoreAllowed, "...and the other core is allowed, whichever core that is");
  check(unpinnedRefused,
        "IDF 4.4's raw tskNO_AFFINITY 0x7FFFFFFF is refused on either layout — 'the scheduler may "
        "put it there' is the failure, not a defence");

  check(lay::otherCoreThan(0) == 1 && lay::otherCoreThan(1) == 0,
        "otherCoreThan swaps the two cores — the derivation kTaskCore is built on");
  std::printf("\n");
}

// ── The admissible band, and the predicate that claims to know when it is empty ─────
void admissibleBandTests() {
  std::printf("priorityIsAdmissible — the band is (idle, polling), exclusive at both ends\n");

  bool insideIsAdmissible = true;
  for (unsigned pp = 0; pp < lay::kMaxPriorities; ++pp) {
    for (unsigned p = lay::kIdlePriority + 1; p < pp; ++p) {
      if (!pol::priorityIsAdmissible(p, pp)) insideIsAdmissible = false;
    }
  }
  check(insideIsAdmissible, "every priority strictly between idle and polling is admissible");

  bool atOrAbovePollingRefused = true;
  for (unsigned pp = 0; pp < lay::kMaxPriorities; ++pp) {
    for (unsigned p = pp; p < lay::kMaxPriorities; ++p) {
      if (pol::priorityIsAdmissible(p, pp)) atOrAbovePollingRefused = false;
    }
  }
  check(atOrAbovePollingRefused,
        "nothing at or above the polling priority is — the SDK's 5 included (R2.1.0, R2.1.1)");

  bool idleRefused = true;
  for (unsigned pp = 0; pp < lay::kMaxPriorities; ++pp) {
    if (pol::priorityIsAdmissible(lay::kIdlePriority, pp)) idleRefused = false;
  }
  check(idleRefused,
        "the idle priority never is: starving idle on a WDT-checked core is a panic, not a delay");

  // The replacement for the old count assertion. layoutAdmitsAnyPriority() is written as an
  // independent statement about a layout rather than by searching the band, so holding it against the
  // band it describes — at every polling priority — reconciles two implementations of one claim by
  // execution. Under today's layout the band is the single value {1}, which is what "no slack at all"
  // means; that is a consequence of the rule below, not a tally to maintain.
  bool emptinessAgrees = true;
  for (unsigned pp = 0; pp < lay::kMaxPriorities; ++pp) {
    bool bandHasAMember = false;
    for (unsigned p = 0; p < lay::kMaxPriorities; ++p) {
      if (pol::priorityIsAdmissible(p, pp)) bandHasAMember = true;
    }
    if (pol::layoutAdmitsAnyPriority(pp) != bandHasAMember) emptinessAgrees = false;
  }
  check(emptinessAgrees,
        "layoutAdmitsAnyPriority agrees with whether the band is really non-empty, per layout");

  // Stated explicitly because it is a design decision rather than an arithmetic accident: with
  // polling at 2 the band is one value wide, so lowering polling to 1 leaves the portal nowhere legal
  // to run at all. Not a number to nudge.
  check(!pol::layoutAdmitsAnyPriority(lay::kIdlePriority + 1),
        "polling at priority 1 would leave NO admissible priority for the portal at all");
  std::printf("\n");
}

// ══════════════════════════════════════════════════════════════════════════════════════
//  The firmware ↔ layout reconciliation. This is the check whose absence was the MAJOR.
// ══════════════════════════════════════════════════════════════════════════════════════
//
// `core_layout.h` states the polling task's core and priority; `firmware.cpp`'s
// `xTaskCreatePinnedToCore` is what actually creates it. Review found that mutating the (then copied)
// polling core from 0 to 1 left all 1283 checks green, because nothing compared the two.
//
// So compare them, by reading the source the way `pack_test.cpp` reads the emitter's fixture — two
// implementations of one fact reconciled by execution rather than by review. The contract asserted is
// deliberately "names the constant OR is a literal equal to it", which is what lets it survive the
// firmware.cpp edit that is still outstanding: today the arguments are literals and must agree; once
// firmware.cpp includes core_layout.h and names them there is no literal left to drift, and the check
// still refuses a wrong literal reintroduced later.
//
// A missing or unparseable firmware.cpp FAILS. It must not skip: a check that can be skipped away is
// a check that cannot fail.

std::string loadFirmwareSource(const char* override_path) {
  const char* candidates[] = {
      // argv[1], so a mutation proof can point this at a modified COPY rather than editing the real
      // firmware.cpp. run.sh passes nothing and gets the path below.
      override_path,
      "src/firmware.cpp",
      "Water-Flow-Meter-PlatformIO/src/firmware.cpp",
      "../src/firmware.cpp",
  };
  for (const char* path : candidates) {
    if (path == nullptr) continue;
    if (std::FILE* f = std::fopen(path, "rb")) {
      std::string text;
      char buffer[4096];
      std::size_t read = 0;
      while ((read = std::fread(buffer, 1, sizeof buffer, f)) > 0) text.append(buffer, read);
      std::fclose(f);
      if (!text.empty()) return text;
    }
  }
  return {};
}

// Drop surrounding whitespace and any block comment, so an argument annotated in place — a literal
// 2 followed by a bracketed remark pointing at core_layout.h — is still recognised as the literal.
std::string trimmed(const std::string& s) {
  std::string out;
  for (std::size_t i = 0; i < s.size();) {
    if (s.compare(i, 2, "/*") == 0) {
      const std::size_t end = s.find("*/", i + 2);
      if (end == std::string::npos) break;
      i = end + 2;
      continue;
    }
    out.push_back(s[i]);
    ++i;
  }
  const std::size_t first = out.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) return {};
  const std::size_t last = out.find_last_not_of(" \t\r\n");
  return out.substr(first, last - first + 1);
}

/**
 * Split the argument list opening at `open`, honouring nesting and string/character literals so a
 * comma inside `"PollingTask"` cannot be mistaken for a separator. Returns empty if unterminated —
 * the caller reports that as a failure rather than guessing.
 */
std::vector<std::string> splitArguments(const std::string& src, std::size_t open) {
  std::vector<std::string> args;
  std::string current;
  int depth = 1;
  for (std::size_t i = open + 1; i < src.size(); ++i) {
    const char c = src[i];
    if (c == '"' || c == '\'') {
      current.push_back(c);
      for (++i; i < src.size(); ++i) {
        current.push_back(src[i]);
        if (src[i] == '\\') {
          if (++i < src.size()) current.push_back(src[i]);
          continue;
        }
        if (src[i] == c) break;
      }
      continue;
    }
    if (c == '(' || c == '[' || c == '{') ++depth;
    if (c == ')' || c == ']' || c == '}') {
      --depth;
      if (depth == 0) {
        args.push_back(trimmed(current));
        return args;
      }
    }
    if (c == ',' && depth == 1) {
      args.push_back(trimmed(current));
      current.clear();
      continue;
    }
    current.push_back(c);
  }
  return {};
}

/** Every `xTaskCreatePinnedToCore(entryFunction, ...)` in `src`, and the first one's arguments. */
struct TaskCreateCall {
  int occurrences = 0;
  std::vector<std::string> args;
};

TaskCreateCall findTaskCreate(const std::string& src, const char* entryFunction) {
  TaskCreateCall found;
  const std::string needle = "xTaskCreatePinnedToCore";
  for (std::size_t at = src.find(needle); at != std::string::npos;
       at = src.find(needle, at + needle.size())) {
    const std::size_t open = src.find('(', at + needle.size());
    if (open == std::string::npos) continue;
    const std::vector<std::string> args = splitArguments(src, open);
    if (args.empty() || args[0] != entryFunction) continue;
    ++found.occurrences;
    if (found.occurrences == 1) found.args = args;
  }
  return found;
}

/**
 * Does `arg`, as written at the call site, denote `value`?
 *
 * Two acceptable forms, and the distinction is the whole point:
 *   - it NAMES the constant (`name` appears in it) — then there is no second copy and no drift is
 *     possible, which is the state firmware.cpp should end up in;
 *   - it is a decimal literal equal to `value` — the state firmware.cpp is in today, permitted only
 *     while it agrees, which is what this check enforces.
 * Anything else — a different literal, an unrelated identifier, an expression — fails, because
 * guessing what it means is how a check stops checking.
 */
bool argDenotes(const std::string& arg, long value, const char* name) {
  // "Names the constant" means the argument IS the identifier, bare or namespace-qualified. Not a
  // substring match: that would wave `kPollingTaskPriority + 1` through, which is a different number
  // wearing the right name — the exact species of near-miss this whole file exists to refuse.
  const std::size_t lastColon = arg.rfind("::");
  if ((lastColon == std::string::npos ? arg : arg.substr(lastColon + 2)) == name) return true;
  std::size_t i = 0;
  if (i < arg.size() && arg[i] == '+') ++i;
  const std::size_t digitsStart = i;
  while (i < arg.size() && arg[i] >= '0' && arg[i] <= '9') ++i;
  if (i == digitsStart) return false;
  const long parsed = std::strtol(arg.c_str() + digitsStart, nullptr, 10);
  while (i < arg.size() && (arg[i] == 'u' || arg[i] == 'U' || arg[i] == 'l' || arg[i] == 'L')) ++i;
  if (i != arg.size()) return false;
  return parsed == value;
}

// xTaskCreatePinnedToCore(fn, name, stack, param, priority, handle, coreID) — 7, fixed by FreeRTOS.
constexpr std::size_t kTaskCreateArgc = 7;
constexpr std::size_t kPriorityArg = 4;
constexpr std::size_t kCoreArg = 6;

void firmwareAgreementTests(const std::string& src) {
  std::printf("firmware.cpp is reconciled against core_layout.h, not merely described by it\n");

  check(!src.empty(),
        "src/firmware.cpp was found and read — a layout claim with nothing to check is not a check");
  if (src.empty()) {
    std::printf("\n");
    return;
  }

  struct Subject {
    const char* entry;
    unsigned priority;
    const char* priorityName;
    int core;
    const char* coreName;
    const char* label;
  };
  const Subject subjects[] = {
      {"pollingTaskCode", lay::kPollingTaskPriority, "kPollingTaskPriority", lay::kPollingTaskCore,
       "kPollingTaskCore", "the polling task"},
      {"logicTaskCode", lay::kLogicTaskPriority, "kLogicTaskPriority", lay::kLogicTaskCore,
       "kLogicTaskCore", "the logic task"},
  };

  for (const Subject& s : subjects) {
    const TaskCreateCall call = findTaskCreate(src, s.entry);
    char label[192];

    // Uniqueness, not a tally: two call sites would mean two placements, and reconciling one of them
    // would prove nothing about the other.
    std::snprintf(label, sizeof label,
                  "exactly one xTaskCreatePinnedToCore(%s, ...) — one placement to reconcile",
                  s.entry);
    check(call.occurrences == 1, label);

    std::snprintf(label, sizeof label,
                  "...spelled with FreeRTOS's %zu parameters, so priority and core are where they "
                  "are read from",
                  kTaskCreateArgc);
    check(call.args.size() == kTaskCreateArgc, label);
    if (call.args.size() != kTaskCreateArgc) continue;

    std::snprintf(label, sizeof label,
                  "%s runs at core_layout::%s (%u) — named there, or a literal that agrees",
                  s.label, s.priorityName, s.priority);
    check(argDenotes(call.args[kPriorityArg], static_cast<long>(s.priority), s.priorityName), label);

    std::snprintf(label, sizeof label, "%s is pinned to core_layout::%s (%d) — R2.1.0's whole content",
                  s.label, s.coreName, s.core);
    check(argDenotes(call.args[kCoreArg], s.core, s.coreName), label);
  }
  std::printf("\n");
}

}  // namespace

int main(int argc, char** argv) {
  std::printf(
      "plc::core_layout + plc::httpd_policy — where tasks run, and where the §7.6 portal may\n\n");
  quotedSdkFactTests();
  canDelayPollingTests();
  coreDedicationTests();
  admissibleBandTests();
  firmwareAgreementTests(loadFirmwareSource(argc > 1 ? argv[1] : nullptr));
  std::printf("%s (%d checks, %d failures)\n", failures == 0 ? "ALL PASSED" : "FAILURES", checks,
              failures);
  return failures == 0 ? 0 : 1;
}
