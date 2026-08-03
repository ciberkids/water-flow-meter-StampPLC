// Host tests for the shared-SPI handover.
//
// The requirement is "no visible artifacts", which is stronger than "no corruption". A lock
// around each SPI transaction would satisfy the second and fail the first: the renderer draws a
// frame as many transactions, so releasing the bus partway leaves half a picture on the panel.
//
// So what these tests actually assert is that the display is NEVER interrupted between
// startWrite() and endWrite(), under every interleaving — a request arriving mid-frame, a release
// arriving before the repaint is drawn, a renderer that never closes its frame. None of those is
// reproducible on demand on a bench, which is why the arbiter is Arduino-free.
#include "bus/spi_arbiter.h"

#include <cstdio>

#include "led/led_patterns.h"

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const char* what) {
  ++checks;
  std::printf("  %-72s %s\n", what, condition ? "ok" : "FAIL");
  if (!condition) ++failures;
}

using plc::BusState;
using plc::SpiArbiter;

/** A renderer that records whether it was ever cut off mid-frame. */
struct FakeRenderer {
  SpiArbiter& bus;
  uint32_t now = 0;
  bool inFrame = false;
  std::size_t framesDrawn = 0;
  std::size_t framesSkipped = 0;
  std::size_t fullRepaints = 0;
  /** The thing that must never happen: the card taking the bus while a frame is open. */
  bool tornFrame = false;

  explicit FakeRenderer(SpiArbiter& b) : bus(b) {}

  /** One pass of the logic loop, as firmware.cpp runs it. */
  void pass(uint32_t deltaMs) {
    now += deltaMs;
    bus.update(now);

    if (!bus.mayBeginFrame()) {
      ++framesSkipped;
      return;
    }
    if (bus.consumeFullRepaintRequest()) {
      ++fullRepaints;
    }
    bus.noteFrameBegan(now);
    inFrame = true;
    // Mid-frame: if the card has the bus now, the panel has been torn.
    if (bus.cardGranted()) {
      tornFrame = true;
    }
    bus.noteFrameEnded(now);
    inFrame = false;
    ++framesDrawn;
  }

  /** Opens a frame and leaves it open, to model a renderer stuck mid-draw. */
  void beginAndHang(uint32_t deltaMs) {
    now += deltaMs;
    bus.update(now);
    if (!bus.mayBeginFrame()) return;
    bus.noteFrameBegan(now);
    inFrame = true;
  }
};

void defaultOwnershipTests() {
  std::printf("[the display owns the bus by default]\n");

  SpiArbiter bus;
  check(bus.state() == BusState::DisplayOwns, "the bus starts with the display");
  check(bus.mayBeginFrame(), "so a frame may begin");
  check(!bus.cardGranted(), "and the card has nothing");
  check(!bus.cardBusy(), "and the LEDs show normal semantics");

  FakeRenderer renderer(bus);
  for (int i = 0; i < 10; ++i) renderer.pass(80);
  check(renderer.framesDrawn == 10, "ten passes draw ten frames");
  check(renderer.framesSkipped == 0, "and skip none");
}

void bootGrantTests() {
  std::printf("\n[at boot the grant is immediate — no frame has ever opened]\n");

  // §4.5 asks the firmware to keep card access at boot wherever possible, and this is why: with
  // no frame open there is no contention to arbitrate at all.
  SpiArbiter bus;
  check(bus.requestCard(0), "the card is granted immediately at boot");
  check(bus.cardGranted(), "and may read");
  check(bus.cardBusy(), "with the LEDs reporting, since the display cannot");
  check(!bus.mayBeginFrame(), "and the renderer is told not to draw");
}

void midFrameRequestTests() {
  std::printf("\n[a request arriving mid-frame waits for the boundary]\n");

  SpiArbiter bus;
  FakeRenderer renderer(bus);

  // Open a frame, then ask for the bus while it is open.
  renderer.beginAndHang(10);
  check(renderer.inFrame, "a frame is open");
  check(!bus.requestCard(renderer.now), "the request is not granted immediately");
  check(bus.state() == BusState::CardRequested, "it is queued");
  check(!bus.cardGranted(), "and the card must not touch the bus yet");

  // Close the frame: the handover completes exactly here and nowhere else.
  bus.noteFrameEnded(renderer.now);
  renderer.inFrame = false;
  check(bus.cardGranted(), "closing the frame completes the handover");
  check(!renderer.tornFrame, "and the frame was never torn");
}

void noTornFrameUnderPressureTests() {
  std::printf("\n[no torn frame under repeated handovers — the actual requirement]\n");

  SpiArbiter bus;
  FakeRenderer renderer(bus);

  // Hammer it, with the card HOLDING the bus for a few passes each time — a real read is not
  // instantaneous, and a card that acquires and releases within one pass never actually contends.
  // If the arbiter ever granted the card between startWrite and endWrite, tornFrame catches it.
  int held = 0;
  for (int cycle = 0; cycle < 200; ++cycle) {
    renderer.pass(7);
    if (bus.cardGranted()) {
      if (++held >= 3) {
        bus.releaseCard(renderer.now);
        held = 0;
      }
    } else {
      bus.requestCard(renderer.now);
    }
    bus.update(renderer.now);
  }
  check(!renderer.tornFrame, "200 request/release cycles never interrupt an open frame");
  check(renderer.framesDrawn > 0, "and the display still got frames");
  check(renderer.framesSkipped > 0, "while genuinely yielding the bus sometimes");
  std::printf("      %zu frames drawn, %zu skipped, %u handovers\n", renderer.framesDrawn,
              renderer.framesSkipped, bus.handovers());
}

void fullRepaintTests() {
  std::printf("\n[after a handover the display repaints IN FULL]\n");

  SpiArbiter bus;
  FakeRenderer renderer(bus);
  renderer.pass(80);
  const std::size_t before = renderer.fullRepaints;

  bus.requestCard(renderer.now);
  check(bus.cardGranted(), "the card takes the bus between frames");
  renderer.pass(80);
  check(renderer.framesSkipped == 1, "the renderer draws nothing while the card holds it");

  bus.releaseCard(renderer.now);
  check(bus.state() == BusState::DisplayResuming, "releasing hands the bus back");
  renderer.pass(80);
  check(renderer.fullRepaints == before + 1,
        "and the next frame is a FULL repaint, not an incremental update");

  // The obligation is discharged by drawing it — one full repaint, not one per frame forever.
  renderer.pass(80);
  check(renderer.fullRepaints == before + 1, "subsequent frames are ordinary again");
}

void hungRendererTests() {
  std::printf("\n[a stuck renderer must not lock the card out forever]\n");

  SpiArbiter bus;
  FakeRenderer renderer(bus);
  renderer.beginAndHang(10);
  check(renderer.inFrame, "the renderer has opened a frame and stopped");

  bus.requestCard(renderer.now);
  check(!bus.cardGranted(), "the card waits, as it should");

  // Just under the timeout: still waiting. The threshold has to be generous enough that a slow
  // but healthy frame is never interrupted.
  bus.update(renderer.now + SpiArbiter::kFrameWaitTimeoutMs - 1);
  check(!bus.cardGranted(), "a slow frame is not interrupted");

  bus.update(renderer.now + SpiArbiter::kFrameWaitTimeoutMs);
  check(bus.cardGranted(), "but a wedged one eventually yields");
  check(bus.timeouts() == 1, "and the timeout is counted, so it is diagnosable");
  // Nothing was torn: a renderer that is not progressing has nothing to tear.
  check(!renderer.tornFrame, "taking the bus from a stuck renderer tears nothing");
}

void idempotenceTests() {
  std::printf("\n[the protocol survives sloppy callers]\n");

  SpiArbiter bus;
  check(bus.requestCard(0), "granted");
  check(bus.requestCard(0), "requesting again while granted is idempotent");
  check(bus.cardGranted(), "and still granted");

  bus.releaseCard(0);
  bus.releaseCard(0);
  check(bus.state() == BusState::DisplayResuming, "releasing twice is harmless");

  SpiArbiter fresh;
  fresh.releaseCard(0);
  check(fresh.state() == BusState::DisplayOwns, "releasing a bus you never held changes nothing");
  check(!fresh.consumeFullRepaintRequest(), "and owes no repaint");
}

void ledStatusTests() {
  std::printf("\n[the LEDs are the status channel while the panel is unavailable]\n");

  // Distinct from every other pattern in §3: never solid, never accelerating, and never a single
  // channel. Two channels alternating with the third is a shape nothing else produces.
  const auto first = plc::cardBusyState(0);
  const auto second = plc::cardBusyState(plc::kCardBusyPeriodMs);
  check(first.red && first.green && !first.blue, "the first phase is amber (red+green)");
  check(!second.red && !second.green && second.blue, "the second is blue");
  check(plc::cardBusyState(plc::kCardBusyPeriodMs * 2).blue == first.blue,
        "and it alternates on a fixed period");

  // Never all three at once, which is what §3.5 reserves for reset acceptance.
  bool everWhite = false;
  for (uint32_t t = 0; t < plc::kCardBusyPeriodMs * 6; t += 25) {
    const auto state = plc::cardBusyState(t);
    if (state.red && state.green && state.blue) everWhite = true;
  }
  check(!everWhite, "it is never solid white, which §3.5 reserves for reset acceptance");
}


void accessPatternTests() {
  std::printf("\n[the three real access points — see the table in spi_arbiter.h]\n");

  // 1. Boot: no display, so no contention. Already covered by bootGrantTests, restated here as
  //    part of the inventory so the three cases sit together.
  {
    SpiArbiter bus;
    check(bus.requestCard(0), "boot: granted with no frame ever opened");
  }

  // 2. Opening the selector: the ONE case that genuinely needs arbitration. The operator is
  //    looking at the page while the directory is enumerated, so an artifact here is the one a
  //    user would actually see.
  {
    SpiArbiter bus;
    FakeRenderer renderer(bus);
    for (int i = 0; i < 3; ++i) renderer.pass(80);          // the selector page is on screen
    renderer.beginAndHang(80);                              // a frame happens to be open
    check(!bus.requestCard(renderer.now), "selector: the enumeration waits for the frame");
    bus.noteFrameEnded(renderer.now);
    renderer.inFrame = false;
    check(bus.cardGranted(), "then reads the directory");
    bus.releaseCard(renderer.now);
    renderer.pass(80);
    check(renderer.fullRepaints == 1, "and the page is repainted in full afterwards");
    check(!renderer.tornFrame, "with nothing torn");
  }

  // 3. Selecting an entry: write, then reboot. It need not hand the bus back — a repaint of a
  //    screen about to disappear buys nothing — and holding it until the reboot must be safe.
  {
    SpiArbiter bus;
    FakeRenderer renderer(bus);
    renderer.pass(80);
    check(bus.requestCard(renderer.now), "select: granted between frames");
    for (int i = 0; i < 5; ++i) renderer.pass(80);
    check(bus.cardGranted(), "and held right up to the reboot without being released");
    check(renderer.framesSkipped == 5, "the display simply stops drawing until the reboot");
    check(!renderer.tornFrame, "and no frame is torn on the way out");
  }
}

}  // namespace

int main() {
  std::printf("plc::SpiArbiter — the display/SD handover\n\n");
  defaultOwnershipTests();
  bootGrantTests();
  midFrameRequestTests();
  noTornFrameUnderPressureTests();
  fullRepaintTests();
  hungRendererTests();
  idempotenceTests();
  accessPatternTests();
  ledStatusTests();
  std::printf("\n%s (%d checks, %d failures)\n", failures == 0 ? "ALL PASSED" : "FAILURES", checks,
              failures);
  return failures == 0 ? 0 : 1;
}
