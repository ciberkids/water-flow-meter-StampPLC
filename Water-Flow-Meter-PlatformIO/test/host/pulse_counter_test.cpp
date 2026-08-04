// Rising-edge detection — the measurement core, which had no host test until now.
//
// firmware.cpp is in no link set in test/host/run.sh, so the loop that produces every litre this
// product reports was the only untested subsystem in a project carrying 1072 checks elsewhere. These
// tests exist to close that, and the extraction in src/sensors/pulse_counter.h exists to make them
// possible.
//
// What is deliberately covered: the phantom-edge-on-enable trap, which is the one way this logic can
// silently invent volume that never flowed.
#include "sensors/pulse_counter.h"

#include <cstdio>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const char* what) {
  ++checks;
  std::printf("  %-72s %s\n", what, condition ? "ok" : "FAIL");
  if (!condition) ++failures;
}

/** Channels visited, in the order forEachRisingChannel reported them. */
std::vector<std::size_t> visited(uint8_t edges) {
  std::vector<std::size_t> seen;
  plc::forEachRisingChannel(edges, [&](std::size_t channel) { seen.push_back(channel); });
  return seen;
}

void risingEdgeTests() {
  std::printf("\n[rising edges — current & ~previous & enabled]\n");
  constexpr uint8_t kAll = 0xFF;

  check(plc::risingEdges(0x00, 0x01, kAll) == 0x01, "low -> high on channel 0 is an edge");
  check(plc::risingEdges(0x01, 0x01, kAll) == 0x00, "high -> high is NOT an edge");
  check(plc::risingEdges(0x01, 0x00, kAll) == 0x00, "high -> low is not a rising edge");
  check(plc::risingEdges(0x00, 0x00, kAll) == 0x00, "low -> low is nothing");
  check(plc::risingEdges(0x00, 0xFF, kAll) == 0xFF, "all eight can rise in one sample");

  // A real sample: some rise, some fall, some hold. Only the risers count.
  check(plc::risingEdges(0b0011'0011, 0b0101'0101, kAll) == 0b0100'0100,
        "mixed transitions yield only the channels that went low->high");

  // The enabled mask gates COUNTING.
  check(plc::risingEdges(0x00, 0xFF, 0b0000'0101) == 0b0000'0101,
        "a rising edge on a disabled channel is not counted");
  check(plc::risingEdges(0x00, 0xFF, 0x00) == 0x00, "with nothing enabled, nothing counts");
}

void phantomEdgeTests() {
  std::printf("\n[the phantom-edge trap — enabling a channel must not invent a pulse]\n");

  // The scenario: channel 3's input sits HIGH the whole time (a stuck-high line, or simply a sensor
  // whose pulse is high when the operator enables it). The channel starts disabled and is enabled
  // between samples.
  //
  // The CORRECT behaviour depends on the caller tracking `previous` from the FULL bitmap, including
  // disabled channels. If it only tracked enabled ones, `previous` for channel 3 would be 0 at the
  // moment of enabling, and 0 -> 1 would look like a real pulse: a litre that never flowed, on the
  // first sample after a configuration change.
  constexpr uint8_t kChannel3 = 0b0000'1000;

  // Tracked correctly: previous carries channel 3's real level even while it was disabled.
  const uint8_t trackedPrevious = kChannel3;
  check(plc::risingEdges(trackedPrevious, kChannel3, kChannel3) == 0x00,
        "a channel enabled while its input is already high yields NO edge");

  // The bug this guards against, stated explicitly so the test documents the failure mode: if the
  // caller had masked `previous` too, it would read 0 and manufacture an edge.
  const uint8_t wronglyMaskedPrevious = static_cast<uint8_t>(trackedPrevious & 0x00);
  check(plc::risingEdges(wronglyMaskedPrevious, kChannel3, kChannel3) == kChannel3,
        "whereas masking `previous` as well WOULD invent one — which is why the caller must not");

  // And a genuine pulse after enabling is still seen.
  check(plc::risingEdges(0x00, kChannel3, kChannel3) == kChannel3,
        "a real low->high after enabling is counted normally");
}

void iterationTests() {
  std::printf("\n[forEachRisingChannel — lowest channel first, one visit per edge]\n");

  check(visited(0x00).empty(), "no edges visits nothing (and must not read ctz(0))");

  const auto one = visited(0b0000'0001);
  check(one.size() == 1 && one[0] == 0, "a single edge on channel 0 visits it once");

  const auto top = visited(0b1000'0000);
  check(top.size() == 1 && top[0] == 7, "a single edge on channel 7 visits channel 7");

  const auto all = visited(0xFF);
  bool ascending = all.size() == 8;
  for (std::size_t i = 0; i < all.size(); ++i) {
    if (all[i] != i) ascending = false;
  }
  check(ascending, "eight edges visit channels 0..7 in ascending order");

  const auto sparse = visited(0b1010'0100);
  check(sparse.size() == 3, "three edges visit exactly three channels");
  check(sparse.size() == 3 && sparse[0] == 2 && sparse[1] == 5 && sparse[2] == 7,
        "and they are the right three, lowest first");

  // The iteration count is the population count, not the word width. That is the property that makes
  // the common case (no flow, no edges) cost nothing.
  std::size_t visits = 0;
  plc::forEachRisingChannel(0b0000'0000, [&](std::size_t) { ++visits; });
  check(visits == 0, "zero edges costs zero iterations, which is the common case at rest");
}

void enabledMaskTests() {
  std::printf("\n[enabledMaskFromBitmap — computed on configuration change, not per sample]\n");

  check(plc::enabledMaskFromBitmap(0x0000, 8) == 0x00, "no sensors connected -> empty mask");
  check(plc::enabledMaskFromBitmap(0x00FF, 8) == 0xFF, "all eight connected -> full mask");
  check(plc::enabledMaskFromBitmap(0b0000'0001, 8) == 0b0000'0001, "sensor 1 only");
  check(plc::enabledMaskFromBitmap(0b1000'0001, 8) == 0b1000'0001, "first and last");

  // The bitmap is 16-bit and the mask is 8-bit: bits above the channel count must not leak in.
  check(plc::enabledMaskFromBitmap(0xFF00, 8) == 0x00,
        "bits above the channel count are ignored rather than truncated into the mask");
  check(plc::enabledMaskFromBitmap(0xFFFF, 8) == 0xFF, "and the low eight still come through");
  check(plc::enabledMaskFromBitmap(0xFFFF, 4) == 0x0F, "a smaller channel count narrows the mask");
}

void integrationTests() {
  std::printf("\n[a run of samples, counted the way the polling loop does it]\n");

  // Two sensors enabled (channels 0 and 2), one disabled but physically pulsing (channel 1).
  constexpr uint8_t kEnabled = 0b0000'0101;
  uint32_t counts[8] = {};

  // A square wave on channels 0, 1 and 2, sampled eight times. Channel 1 is wired and pulsing but
  // not enabled, so it must contribute nothing.
  //
  // The expected count is DERIVED below rather than asserted from a guess — my first version of this
  // test claimed four and the real answer is three, because the sequence opens LOW so the first
  // sample produces no transition. Counting it by hand and hard-coding the answer is how a test comes
  // to disagree with correct code, so the expectation is computed from the same sequence.
  const uint8_t levels[] = {0b000, 0b111, 0b111, 0b000, 0b111, 0b000, 0b111, 0b000};
  std::size_t expectedEdges = 0;
  {
    uint8_t walk = 0;
    for (const uint8_t current : levels) {
      if ((current & 0b001) && !(walk & 0b001)) ++expectedEdges;
      walk = current;
    }
  }
  uint8_t previous = 0;
  for (const uint8_t current : levels) {
    const uint8_t edges = plc::risingEdges(previous, current, kEnabled);
    plc::forEachRisingChannel(edges, [&](std::size_t channel) { ++counts[channel]; });
    // The FULL bitmap, not the masked one — see the phantom-edge tests.
    previous = current;
  }

  std::printf("      counts: ch0=%u ch1=%u ch2=%u (expected %zu edges)\n", counts[0], counts[1],
              counts[2], expectedEdges);
  check(expectedEdges == 3, "the sequence contains three rising edges, not four — it opens low");
  check(counts[0] == expectedEdges, "channel 0 counted exactly the edges in the sequence");
  check(counts[2] == expectedEdges, "channel 2, on the same waveform, counted the same");
  check(counts[1] == 0, "channel 1 pulsed physically but is disabled, so counted zero");

  // A held-high input must not accumulate. This is the difference between counting edges and
  // sampling levels, and getting it wrong would make a stuck sensor look like enormous flow.
  uint32_t stuck = 0;
  previous = 0;
  for (int i = 0; i < 100; ++i) {
    const uint8_t edges = plc::risingEdges(previous, 0b0000'0001, 0b0000'0001);
    plc::forEachRisingChannel(edges, [&](std::size_t) { ++stuck; });
    previous = 0b0000'0001;
  }
  check(stuck == 1, "an input held high for 100 samples counts ONE edge, not 100");
}

}  // namespace

int main() {
  std::printf("pulse counter — the measurement core\n");
  risingEdgeTests();
  phantomEdgeTests();
  iterationTests();
  enabledMaskTests();
  integrationTests();
  std::printf("\n%s (%d checks, %d failures)\n", failures == 0 ? "ALL PASSED" : "FAILURES", checks,
              failures);
  return failures == 0 ? 0 : 1;
}
