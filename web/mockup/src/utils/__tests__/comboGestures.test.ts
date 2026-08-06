import { describe, expect, it } from "vitest";
import {
  applyButtons,
  comboConsumed,
  initialComboState,
  kDisplayOffMaxMs,
  kSelectorHoldMs,
  tickCombos,
  type ComboButtons,
  type ComboEvent,
  type ComboState
} from "../comboGestures";

/**
 * The oracle for every case below is the firmware: src/input/interaction_handler.cpp and the
 * host suite's recoveryGestureTests() at test/host/interaction_test.cpp:1465-1517. Where a case
 * is a port, the comment cites the assertion it came from; where it is new, it pins a boundary
 * or a suppression the firmware suite never exercises.
 */

function held(...names: Array<keyof ComboButtons>): ComboButtons {
  return {
    up: names.includes("up"),
    down: names.includes("down"),
    enter: names.includes("enter")
  };
}

const noButtons = held();
const upDown = held("up", "down");
const allThree = held("up", "down", "enter");

function kinds(result: { events: readonly ComboEvent[] }): string[] {
  return result.events.map((event) => event.kind);
}

/**
 * A driver in the shape of the firmware's logic loop: button levels change, then time advances
 * pass by pass with the levels unchanged.
 */
function rig(): {
  apply: (buttons: ComboButtons, atMs: number) => ReturnType<typeof applyButtons>;
  tick: (atMs: number) => ReturnType<typeof tickCombos>;
  /** Polls every 50 ms up to and including `untilMs`, as holdAllThree() does (interaction_test.cpp:297-302). */
  poll: (untilMs: number) => ComboEvent[];
  state: () => ComboState;
} {
  let state = initialComboState();
  let levels = noButtons;
  let clock = 0;
  return {
    apply(buttons, atMs) {
      const result = applyButtons(state, buttons, atMs);
      state = result.state;
      levels = buttons;
      clock = atMs;
      return result;
    },
    tick(atMs) {
      const result = tickCombos(state, levels, atMs);
      state = result.state;
      clock = atMs;
      return result;
    },
    poll(untilMs) {
      const collected: ComboEvent[] = [];
      for (let at = clock + 50; at <= untilMs; at += 50) {
        const result = tickCombos(state, levels, at);
        state = result.state;
        clock = at;
        collected.push(...result.events);
      }
      return collected;
    },
    state() {
      return state;
    }
  };
}

describe("the constants come from the firmware header", () => {
  it("carries kDisplayOffComboMaxMs and kSelectorComboHoldMs verbatim", () => {
    // interaction_handler.h:91 and :93. A drifted copy here would make every boundary test
    // below agree with itself and with nothing on the device.
    expect(kDisplayOffMaxMs).toBe(1000);
    expect(kSelectorHoldMs).toBe(3000);
  });
});

describe("the UP+DOWN+ENTER recovery gesture — §3.4.1", () => {
  it("does not open the selector just under the hold, and opens it past it", () => {
    // Ported from interaction_test.cpp:1471-1474 (holdAllThree(2900), then tick(200)).
    const dev = rig();
    dev.apply(allThree, 0);
    expect(dev.poll(2900)).toEqual([]);
    expect(kinds(dev.tick(3100))).toEqual(["open-pack-selector"]);
  });

  it("opens the selector once and never again while the buttons stay held", () => {
    // Ported from interaction_test.cpp:1476-1479: "otherwise the selector would be re-entered
    // continuously for as long as the operator kept holding". Expressed as the contract rather
    // than as a count of opens: every later pass must report no event at all.
    const dev = rig();
    dev.apply(allThree, 0);
    dev.poll(3100);
    expect(dev.poll(4100)).toEqual([]);
    expect(kinds(dev.tick(9000))).toEqual([]);
  });

  it("fires at the threshold exactly and not one millisecond before", () => {
    // interaction_handler.cpp:328 compares with >=, so kSelectorHoldMs itself is inside.
    const early = rig();
    early.apply(allThree, 0);
    expect(kinds(early.tick(kSelectorHoldMs - 1))).toEqual([]);

    const onTime = rig();
    onTime.apply(allThree, 0);
    expect(kinds(onTime.tick(kSelectorHoldMs))).toEqual(["open-pack-selector"]);
  });

  it("stamps the hold from the third button down, not the first", () => {
    // interaction_handler.cpp:317-321 arms on the pass the set completes. UP+DOWN spent a second
    // waiting for ENTER here, and that second must not count towards the three.
    const dev = rig();
    dev.apply(held("up"), 0);
    dev.apply(upDown, 400);
    dev.apply(allThree, 1000);
    expect(dev.poll(3900)).toEqual([]);
    expect(kinds(dev.tick(4000))).toEqual(["open-pack-selector"]);
  });

  it("produces nothing at all when a button is released before the threshold", () => {
    // interaction_handler.cpp:312-315 discards the whole attempt.
    const dev = rig();
    dev.apply(allThree, 0);
    dev.poll(2900);
    expect(kinds(dev.apply(upDown, 2950))).toEqual([]);
    expect(kinds(dev.apply(noButtons, 5000))).toEqual([]);
  });

  it("opens the selector without also tripping display-off", () => {
    // Ported from interaction_test.cpp:1490-1492 — "THE COLLISION THAT MATTERS": the recovery
    // gesture is the display-off pair plus ENTER, held three times as long.
    const dev = rig();
    dev.apply(allThree, 0);
    expect(dev.poll(3100)).toEqual([{ kind: "open-pack-selector", heldMs: 3000, atMs: 3000 }]);
    expect(dev.poll(6000)).toEqual([]);
  });

  it("emits nothing and consumes all three buttons on the way out", () => {
    // Ported from interaction_test.cpp:1493-1495: "releasing the three does not also page or
    // descend on the way out". In the firmware that assertion passes only because a 3100 ms hold
    // has already sent each button's long press, which suppresses its release event
    // (button_input.cpp:33-40). Here it is the combo's own doing, so it holds for short holds too.
    const dev = rig();
    dev.apply(allThree, 0);
    dev.poll(3100);
    const release = dev.apply(noButtons, 3150);
    expect(kinds(release)).toEqual([]);
    expect(release.consumed).toEqual({ up: true, down: true, enter: true });
  });

  it("can be performed again after a full release", () => {
    // interaction_handler.cpp:312-315 clears `fired` along with the rest, so the gesture is not
    // one-shot for the lifetime of the device.
    const dev = rig();
    dev.apply(allThree, 0);
    dev.poll(3100);
    dev.apply(noButtons, 3200);
    dev.apply(allThree, 4000);
    expect(dev.poll(6900)).toEqual([]);
    expect(kinds(dev.tick(7000))).toEqual(["open-pack-selector"]);
  });
});

describe("the UP+DOWN display-off gesture — §3.1", () => {
  it("turns the display off and does not open the selector", () => {
    // Ported from interaction_test.cpp:1502-1504 (tapUpDown), which presses both, polls twice at
    // 30 ms, and releases both.
    const dev = rig();
    dev.apply(upDown, 0);
    dev.poll(60);
    expect(kinds(dev.apply(noButtons, 90))).toEqual(["display-off"]);
  });

  it("arms on the level test whatever order the buttons arrive in", () => {
    // interaction_handler.cpp:201 is `up && down && !enter` — a level test. Press order is not
    // part of it, and neither is a settling delay.
    const orders: Array<[ComboButtons, ComboButtons]> = [
      [held("up"), upDown],
      [held("down"), upDown],
      [noButtons, upDown]
    ];
    for (const [first, second] of orders) {
      const dev = rig();
      dev.apply(first, 0);
      dev.apply(second, 10);
      expect(kinds(dev.apply(noButtons, 200))).toEqual(["display-off"]);
    }
  });

  it("fires for a tap pressed and released at the same instant", () => {
    // The 50 ms defect: useSimulatedButtons.ts:178 defers the arming check with setTimeout, so a
    // pointerdown+pointerup in one frame — or a click on a combo button — releases before the
    // gesture ever arms and display-off is unreachable. Arming is synchronous here, and a 0 ms
    // hold is strictly under the window.
    const dev = rig();
    dev.apply(upDown, 1234);
    const release = dev.apply(noButtons, 1234);
    expect(kinds(release)).toEqual(["display-off"]);
    expect(release.events[0]).toMatchObject({ heldMs: 0, atMs: 1234 });
  });

  it("fires on release, never while the buttons are still held", () => {
    // interaction_handler.cpp:201-207 returns early for the whole hold; only the else branch at
    // :209-217 can fire.
    const dev = rig();
    dev.apply(upDown, 0);
    expect(dev.poll(900)).toEqual([]);
    expect(kinds(dev.apply(noButtons, 950))).toEqual(["display-off"]);
  });

  it("fires just under the window and not at it", () => {
    // interaction_handler.cpp:210 is a strict <, so kDisplayOffMaxMs itself is a hold, not a tap.
    const justUnder = rig();
    justUnder.apply(upDown, 0);
    expect(kinds(justUnder.apply(noButtons, kDisplayOffMaxMs - 1))).toEqual(["display-off"]);

    const atTheLimit = rig();
    atTheLimit.apply(upDown, 0);
    expect(kinds(atTheLimit.apply(noButtons, kDisplayOffMaxMs))).toEqual([]);
  });

  it("reports the hold measured from the pair, not from either button alone", () => {
    // The window is `nowMs - comboState_.startMs` (interaction_handler.cpp:210), and startMs is
    // when the PAIR completed — UP had already been down for 5 s here, which is irrelevant.
    const dev = rig();
    dev.apply(held("up"), 0);
    dev.apply(upDown, 5000);
    const release = dev.apply(noButtons, 5300);
    expect(release.events[0]).toEqual({ kind: "display-off", heldMs: 300, atMs: 5300 });
  });

  it("does not fire while ENTER is down", () => {
    // interaction_handler.cpp:212 guards the fire with `!enter` as well as the window.
    const dev = rig();
    dev.apply(upDown, 0);
    dev.apply(allThree, 100);
    expect(kinds(dev.apply(held("enter"), 200))).toEqual([]);
  });
});

describe("ENTER's arrival and departure during an armed display-off", () => {
  it("is cancelled by ENTER joining mid-hold", () => {
    // interaction_handler.cpp:201 stops matching the moment ENTER goes down, and :209-217 clears
    // the state without firing. Releasing everything afterwards must therefore be silent, even
    // though the release is well inside the 1000 ms window measured from the original press.
    const dev = rig();
    dev.apply(upDown, 0);
    expect(kinds(dev.apply(allThree, 200))).toEqual([]);
    expect(kinds(dev.apply(noButtons, 400))).toEqual([]);
  });

  it("is re-armed with a fresh start time when ENTER releases and UP+DOWN stay held", () => {
    // Not pinned by the firmware suite, but it is what the code does: with ENTER up the level
    // test at interaction_handler.cpp:201 matches again, and :202-205 re-arms with nowMs because
    // the previous state was cleared on the pass ENTER joined.
    //
    // 1900 ms after the original press, so a stale start time would be outside the window and
    // this would emit nothing at all.
    const dev = rig();
    dev.apply(upDown, 0);
    dev.apply(allThree, 500);
    dev.apply(upDown, 1200);
    const release = dev.apply(noButtons, 1900);
    expect(release.events).toEqual([{ kind: "display-off", heldMs: 700, atMs: 1900 }]);
  });

  it("does not fire when the re-armed hold outlives the window", () => {
    // The mirror of the case above: the fresh start time is a real start time, not a licence.
    const dev = rig();
    dev.apply(upDown, 0);
    dev.apply(allThree, 500);
    dev.apply(upDown, 1200);
    expect(kinds(dev.apply(noButtons, 2200))).toEqual([]);
  });
});

describe("the participating buttons' single-button events are swallowed", () => {
  it("consumes the second of UP/DOWN to be released, so no stray short press escapes", () => {
    // The firmware clears the queue at the instant display-off fires
    // (interaction_handler.cpp:215), which covers the first release only; the second button's
    // release lands a pass later, when the combo has already been cleared. tapUpDown()
    // (interaction_test.cpp:312-320) releases both on one pass, so the suite never sees it. In
    // the mockup it showed up as a spurious short press that paged the woken display.
    const dev = rig();
    const armed = dev.apply(upDown, 0);
    expect(armed.consumed).toMatchObject({ up: true, down: true });

    const firstRelease = dev.apply(held("down"), 200);
    expect(kinds(firstRelease)).toEqual(["display-off"]);
    expect(firstRelease.consumed.up).toBe(true);

    const secondRelease = dev.apply(noButtons, 300);
    expect(kinds(secondRelease)).toEqual([]);
    expect(secondRelease.consumed.down).toBe(true);
  });

  it("consumes ENTER's release during a three-button hold", () => {
    // ENTER released under the 1500 ms long-press threshold still has a short press to deliver
    // (button_input.cpp:33-40), and on that pass `all` is already false, so
    // interaction_handler.cpp:312-315 returns without clearing it. On the device that press
    // descends a level; in the mockup it did the same.
    const dev = rig();
    dev.apply(allThree, 0);
    dev.poll(800);
    const enterUp = dev.apply(upDown, 800);
    expect(kinds(enterUp)).toEqual([]);
    expect(enterUp.consumed.enter).toBe(true);
  });

  it("consumes UP and DOWN for the whole armed hold, so a long press cannot page", () => {
    // The armed branch (interaction_handler.cpp:201-207) returns early and clears nothing, so on
    // the device a UP+DOWN hold past 1500 ms delivers both long presses and then a 250 ms repeat
    // stream (button_input.cpp:48-60) to whatever screen is showing. comboConsumed() is what lets
    // the adapter's own long-press timer know to stay quiet.
    const dev = rig();
    dev.apply(upDown, 0);
    dev.poll(2000);
    expect(comboConsumed(dev.state())).toMatchObject({ up: true, down: true });

    // Past the window, so the release is not a display-off either — it must be silent, not a pair
    // of short presses.
    const release = dev.apply(noButtons, 2100);
    expect(kinds(release)).toEqual([]);
    expect(release.consumed).toEqual({ up: true, down: true, enter: false });
  });

  it("leaves a button that never joined a combo alone", () => {
    // The suppression must be narrow: ENTER on its own descends, and UP on its own pages.
    const enterOnly = rig();
    expect(enterOnly.apply(held("enter"), 0).consumed).toEqual({ up: false, down: false, enter: false });
    expect(enterOnly.apply(noButtons, 100).consumed).toEqual({ up: false, down: false, enter: false });

    const upOnly = rig();
    expect(upOnly.apply(held("up"), 0).consumed.up).toBe(false);
    expect(upOnly.apply(noButtons, 100).consumed.up).toBe(false);
  });

  it("stops consuming a button once it is up again", () => {
    // The latch is scoped to the press it belongs to. If it outlived the release, the operator's
    // next ordinary press would be swallowed and the panel would look dead.
    const dev = rig();
    dev.apply(upDown, 0);
    dev.apply(noButtons, 200);
    expect(comboConsumed(dev.state())).toEqual({ up: false, down: false, enter: false });
    expect(dev.apply(held("up"), 400).consumed.up).toBe(false);
    expect(dev.apply(noButtons, 500).consumed.up).toBe(false);
  });
});

describe("the machine is a value, not a mutable object", () => {
  it("never writes to the state it is handed", () => {
    // The adapter will hold this in a ref. If either entry point mutated in place, every other
    // test here would still pass — they all read the returned state — while React saw its ref
    // change under it and the firing pass and the arming pass shared one object.
    const armed = applyButtons(initialComboState(), upDown, 0).state;
    const snapshot = structuredClone(armed);

    const fired = applyButtons(armed, noButtons, 300);
    expect(armed).toEqual(snapshot);
    expect(fired.state).not.toBe(armed);
    expect(kinds(fired)).toEqual(["display-off"]);

    const threeHeld = applyButtons(initialComboState(), allThree, 0).state;
    const threeSnapshot = structuredClone(threeHeld);
    const opened = tickCombos(threeHeld, allThree, kSelectorHoldMs);
    expect(threeHeld).toEqual(threeSnapshot);
    expect(opened.state).not.toBe(threeHeld);
    expect(kinds(opened)).toEqual(["open-pack-selector"]);

    // And replaying the same pass from the same state gives the same answer, which it could not
    // if the first call had consumed anything.
    expect(tickCombos(threeHeld, allThree, kSelectorHoldMs)).toEqual(opened);
  });
});
