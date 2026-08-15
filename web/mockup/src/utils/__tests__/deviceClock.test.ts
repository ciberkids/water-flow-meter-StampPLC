import { describe, expect, it } from "vitest";

import {
  SimulatedClockState,
  clockAfterSessionReset,
  kSimulatedClockChoices,
  sessionStartText,
  simulatedClock
} from "../deviceClock";

/**
 * 2026-08-12T14:32:07Z. `date -u -d @1786545127` -> Wed Aug 12 14:32:07 UTC 2026.
 *
 * The same anchor the firmware tests use, so a divergence between the two implementations shows up as
 * one of them disagreeing with a number both were checked against rather than with each other.
 */
const kEpoch = 1786545127;

describe("sessionStartText — the four renderings, which must match ui_bindings.cpp exactly", () => {
  it("renders the timestamp when the clock is trusted and the start is known", () => {
    expect(sessionStartText({ set: true, sessionStartEpoch: kEpoch, awaitingClock: false })).toBe(
      "2026-08-12 14:32 UTC"
    );
  });

  it("says CLOCK UNSET when there is no clock and no reset waiting", () => {
    expect(sessionStartText({ set: false, sessionStartEpoch: 0, awaitingClock: false })).toBe(
      "CLOCK UNSET"
    );
  });

  it("says AWAITING CLOCK when a reset happened before there was a clock", () => {
    expect(sessionStartText({ set: false, sessionStartEpoch: 0, awaitingClock: true })).toBe(
      "AWAITING CLOCK"
    );
  });

  it("says UNKNOWN when the clock is trusted but nothing recorded a start", () => {
    expect(sessionStartText({ set: true, sessionStartEpoch: 0, awaitingClock: false })).toBe("UNKNOWN");
  });

  /**
   * The three negatives are three DIFFERENT strings, not one repeated.
   *
   * Asserted as a set rather than only case by case, because the failure that matters is two of them
   * collapsing into the same text — which every individual assertion above would still pass if the
   * expected strings were edited to match.
   */
  it("keeps the three negatives distinguishable from each other", () => {
    const rendered = [
      sessionStartText({ set: false, sessionStartEpoch: 0, awaitingClock: false }),
      sessionStartText({ set: false, sessionStartEpoch: 0, awaitingClock: true }),
      sessionStartText({ set: true, sessionStartEpoch: 0, awaitingClock: false })
    ];
    expect(new Set(rendered).size).toBe(3);
  });

  it("never renders 1970 or a bare zero, whatever the flags say", () => {
    // Including the state DeviceClock cannot reach — an epoch with no clock behind it. Both
    // implementations refuse it rather than relying on it staying unreachable.
    for (const clock of [
      { set: false, sessionStartEpoch: 0, awaitingClock: false },
      { set: false, sessionStartEpoch: 0, awaitingClock: true },
      { set: true, sessionStartEpoch: 0, awaitingClock: false },
      { set: false, sessionStartEpoch: kEpoch, awaitingClock: false }
    ]) {
      const text = sessionStartText(clock);
      expect(text).not.toContain("1970");
      expect(text).not.toMatch(/^0/);
    }
  });

  it("stays inside the 20-character worst case P3's spec declares", () => {
    for (const clock of [
      { set: true, sessionStartEpoch: kEpoch, awaitingClock: false },
      // The widest year the device will ever be asked for: kLatestPlausibleEpoch, 2100-01-01.
      { set: true, sessionStartEpoch: 4102444800, awaitingClock: false },
      { set: false, sessionStartEpoch: 0, awaitingClock: false },
      { set: false, sessionStartEpoch: 0, awaitingClock: true },
      { set: true, sessionStartEpoch: 0, awaitingClock: false }
    ]) {
      expect(sessionStartText(clock).length).toBeLessThanOrEqual(20);
    }
  });

  /** The device is zone-free, so the same epoch must render identically wherever the browser is. */
  it("renders in UTC and labels it, so a browser in another zone shows the panel's characters", () => {
    // `date -u -d @1786579200` -> Thu Aug 13 00:00:00 UTC 2026. In any zone west of UTC a local-time
    // renderer would print the 12th here, which is the whole reason getUTC* is used.
    expect(sessionStartText({ set: true, sessionStartEpoch: 1786579200, awaitingClock: false })).toBe(
      "2026-08-13 00:00 UTC"
    );
  });

  it("truncates to the minute rather than rounding, matching the firmware's snprintf", () => {
    // 14:32:59 must still read 14:32 — a rounded minute would disagree with the panel by one.
    expect(sessionStartText({ set: true, sessionStartEpoch: kEpoch + 52, awaitingClock: false })).toBe(
      "2026-08-12 14:32 UTC"
    );
  });
});

describe("simulatedClock — how the Clock control reaches all four states", () => {
  it("maps each of the four states to the three facts the device would publish", () => {
    expect(simulatedClock("dated", kEpoch)).toEqual({
      set: true,
      sessionStartEpoch: kEpoch,
      awaitingClock: false
    });
    expect(simulatedClock("undated", kEpoch)).toEqual({
      set: true,
      sessionStartEpoch: 0,
      awaitingClock: false
    });
    expect(simulatedClock("unset", kEpoch)).toEqual({
      set: false,
      sessionStartEpoch: 0,
      awaitingClock: false
    });
    expect(simulatedClock("awaiting", kEpoch)).toEqual({
      set: false,
      sessionStartEpoch: 0,
      awaitingClock: true
    });
  });

  /**
   * THE requirement from item 4: all four renderings reachable from the panel, and all four DIFFERENT.
   *
   * Driven through `kSimulatedClockChoices` rather than a hand-written list of states, so a choice added
   * to or removed from the control is covered by this test automatically — the failure it guards against
   * is a state existing in the type and being unofferable in the UI, which a hardcoded list would hide.
   */
  it("offers exactly the four states, and every one renders differently", () => {
    const rendered = kSimulatedClockChoices.map((choice) =>
      sessionStartText(simulatedClock(choice.state, kEpoch))
    );
    expect(new Set(rendered)).toEqual(
      new Set(["2026-08-12 14:32 UTC", "UNKNOWN", "CLOCK UNSET", "AWAITING CLOCK"])
    );
    expect(rendered).toHaveLength(4);
  });

  it("gives every choice a label and a hint naming what P3 will show", () => {
    for (const choice of kSimulatedClockChoices) {
      expect(choice.label.length).toBeGreaterThan(0);
      expect(choice.hint).toContain("P3");
    }
  });
});

describe("clockAfterSessionReset — a mirror of DeviceClock::noteSessionStart", () => {
  it("dates the reset when there is a clock", () => {
    expect(clockAfterSessionReset("dated")).toBe("dated");
    expect(clockAfterSessionReset("undated")).toBe("dated");
  });

  /**
   * And CANNOT date it when there is none.
   *
   * This is the arm that matters: the firmware sets `sessionStartUnknown_` here rather than inventing a
   * timestamp, so a reset performed on a clockless simulated device must land on AWAITING CLOCK. Jumping
   * to "dated" would show a time the device could not have produced — the one thing the mirror prevents.
   */
  it("leaves the reset awaiting a clock when there is none", () => {
    expect(clockAfterSessionReset("unset")).toBe("awaiting");
    expect(clockAfterSessionReset("awaiting")).toBe("awaiting");
    expect(sessionStartText(simulatedClock(clockAfterSessionReset("unset"), kEpoch))).toBe(
      "AWAITING CLOCK"
    );
  });

  it("is total over every state the control can produce", () => {
    for (const choice of kSimulatedClockChoices) {
      const next: SimulatedClockState = clockAfterSessionReset(choice.state);
      expect(["dated", "awaiting"]).toContain(next);
    }
  });
});
