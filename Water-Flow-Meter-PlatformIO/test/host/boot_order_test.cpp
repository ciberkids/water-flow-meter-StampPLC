/**
 * The boot ordering in `setup()` that nothing protected — N-d2's software half.
 *
 * `plc::readRtcVoltageLowFlag()` MUST run before `M5StamPLC.begin()`. The library's `begin()` clears
 * the RX8130CE's whole flag register, VLF included, and exposes no reader for it — so that single line
 * is the only moment in the device's life when "did the clock run across the last power cut" can be
 * known. Move it below `begin()` and it reports a healthy clock unconditionally: a device whose RTC had
 * lost power would then publish a confident year-2000 timestamp to the panel, to Modbus and to MQTT,
 * and every timestamp downstream would be wrong in a way that looks fine.
 *
 * ── WHY THIS IS A SOURCE-TEXT TEST AND NOT AN ASSERT ─────────────────────────────────────
 *
 * The invariant is about the ORDER OF TWO LINES IN A FILE. A C++ assertion cannot see that: at runtime
 * both calls have happened and nothing distinguishes "probed first" from "probed second" — the flag
 * register reads clean either way, which is precisely the failure. A runtime latch could catch
 * `noteBootTrust` being called with no probe at all, but not the probe being reordered below `begin()`,
 * which is the mistake a refactor actually makes.
 *
 * So this reads `src/firmware.cpp` as text, the same technique `tools/wiki/gen-actions.mjs` uses to
 * scan the action catalogue. It is a coarse instrument and that is acceptable here: the thing being
 * protected is a comment's claim about line order, and a coarse check that runs on every push beats an
 * elegant one that needs hardware.
 *
 * WHAT IT CANNOT SEE, stated so the coverage is not overclaimed: it does not parse C++. A probe moved
 * inside a conditional, or into a function called from `setup()` after `begin()`, would satisfy every
 * assertion below. What it catches is the ordinary refactor — a line moved, or a new RTC read inserted
 * above the probe — which is the one the register describes.
 *
 * The other half of N-d2 needs a board: whether the RX8130CE survives power loss at all is unknown,
 * because the chip has a backup-supply pin and M5Stack does not say whether a cell or supercap is
 * populated. Set the time, pull power for a minute, boot, read VLF. Nothing here can answer that.
 */
#include <cstdio>
#include <string>

#include "firmware_source_text.h"

namespace {

using firmware_source::lineOf;
using firmware_source::loadFirmwareSource;
using firmware_source::offsetsOf;
using firmware_source::withoutCommentsOrStrings;

int failures = 0;
int checks = 0;

void check(bool condition, const char* what) {
  ++checks;
  std::printf("  %-74s %s\n", what, condition ? "ok" : "FAIL");
  if (!condition) ++failures;
}

void theProbeComesFirst() {
  std::printf("\n[N-d2 — the VLF probe must precede M5StamPLC.begin(), or it reads a cleared flag]\n");

  const std::string raw = loadFirmwareSource();
  // Comments blanked FIRST, so the paragraph explaining this invariant cannot break it.
  const std::string source = withoutCommentsOrStrings(raw);
  // A missing file must FAIL, not skip. A gate that quietly passes when it cannot find what it guards
  // is worse than no gate: it reports green for a file it never opened.
  check(!raw.empty(), "src/firmware.cpp is readable from the test's working directory");
  if (raw.empty()) return;

  const auto probes = offsetsOf(source, "readRtcVoltageLowFlag()");
  const auto begins = offsetsOf(source, "M5StamPLC.begin()");
  const auto epochReads = offsetsOf(source, "readRtcEpoch()");

  // Exactly one CALL, plus however many times the name appears in comments. Counting call syntax
  // specifically — the trailing `()` — keeps the prose free to discuss it, which it does at length.
  check(probes.size() == 1, "there is exactly one call to readRtcVoltageLowFlag()");
  check(begins.size() == 1, "and exactly one call to M5StamPLC.begin()");
  if (probes.empty() || begins.empty()) return;

  const std::size_t probeLine = lineOf(source, probes.front());
  const std::size_t beginLine = lineOf(source, begins.front());
  std::printf("      probe at line %zu, M5StamPLC.begin() at line %zu\n", probeLine, beginLine);
  check(probes.front() < begins.front(),
        "the probe is EARLIER in the file — begin() clears VLF, so afterwards it always reads clean");

  // The counterpart ordering, and a real one rather than symmetry for its own sake: the calendar read
  // needs the I2C bus up, so it must come AFTER begin(). Above it, it would read an uninitialised bus
  // and hand `noteBootTrust` a value that means nothing.
  check(!epochReads.empty(), "readRtcEpoch() is called");
  if (!epochReads.empty()) {
    check(epochReads.front() > begins.front(),
          "and it is called AFTER begin(), because reading the calendar needs the bus up");
  }

  // The probe is inside setup(). Its whole value is being the first thing the device does; in a helper
  // called from the loop it would be measuring nothing.
  const auto setupAt = offsetsOf(source, "void setup()");
  check(!setupAt.empty(), "setup() is found");
  if (setupAt.empty()) return;
  check(probes.front() > setupAt.front(),
        "the probe sits inside setup(), not in a helper that runs later");

  /**
   * Nothing in SETUP may touch the library before the probe — `M5StamPLC.` at all, not just
   * `begin()`, because any call into it may bring the bus up as a side effect and the flag is gone the
   * moment that happens.
   *
   * SCOPED TO SETUP, and the first version of this check was not. It asserted that no `M5StamPLC.`
   * appeared anywhere earlier in the file, and failed on `M5StamPLC.readPlcInput` at line 817 — which
   * is inside the sampler's pin-read helper, DEFINED above `setup()` and CALLED from a task that
   * starts long after `begin()`. That is a definition-order artefact, not a violation: file position
   * and execution order are different things, and a gate that confuses them fails on correct code,
   * which is how gates get deleted. What the invariant is actually about is the sequence of statements
   * in `setup()`.
   */
  std::size_t firstLibraryUseInSetup = std::string::npos;
  for (const std::size_t at : offsetsOf(source, "M5StamPLC.")) {
    if (at > setupAt.front()) { firstLibraryUseInSetup = at; break; }
  }
  check(firstLibraryUseInSetup != std::string::npos && firstLibraryUseInSetup > probes.front(),
        "and no M5StamPLC call inside setup() precedes it, not merely no begin()");
}

}  // namespace

int main() {
  std::printf("boot_order — the one line in setup() whose POSITION is the requirement (N-d2)\n");
  theProbeComesFirst();
  if (failures > 0) {
    std::printf("\nFAILURES (%d of %d)\n", failures, checks);
    return 1;
  }
  std::printf("\nALL PASSED (%d checks)\n", checks);
  return 0;
}
