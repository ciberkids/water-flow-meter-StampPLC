import fs from "node:fs";
import path from "node:path";
import { describe, expect, it } from "vitest";
import {
  holdCountdownArms,
  holdCountdownIsVisible,
  holdCountdownText,
  kGestureLongPressMs
} from "../holdCountdown";

const firmwareSource = (...parts: string[]) =>
  fs.readFileSync(
    path.join(__dirname, "..", "..", "..", "..", "..", "Water-Flow-Meter-PlatformIO", "src", ...parts),
    "utf-8"
  );

const datasetScreens = () =>
  JSON.parse(
    fs.readFileSync(path.join(__dirname, "..", "..", "data", "screens.json"), "utf-8")
  ) as {
    screens: {
      id: string;
      elements: { binding?: string }[];
      flows?: { trigger: { type: string; holdButton?: string; durationMs?: number } }[];
    }[];
  };

describe("arming, which is where the destructive case was", () => {
  it("arms on ENTER alone", () => {
    expect(holdCountdownArms({ up: false, down: false, enter: true })).toBe(true);
  });

  it("does NOT arm while UP or DOWN is already held", () => {
    /**
     * The reported shape: UP+DOWN grabbed first, then ENTER, on `RESET TOTALS?`. The §3.4.1 recovery
     * gesture completes at 3000 ms and so does that screen's guard, so a simulator that armed anyway
     * performed the reset the operator was trying to escape a broken menu with.
     */
    expect(holdCountdownArms({ up: true, down: false, enter: true })).toBe(false);
    expect(holdCountdownArms({ up: false, down: true, enter: true })).toBe(false);
    expect(holdCountdownArms({ up: true, down: true, enter: true })).toBe(false);
  });

  it("does not arm without ENTER at all", () => {
    expect(holdCountdownArms({ up: true, down: true, enter: false })).toBe(false);
    expect(holdCountdownArms({ up: false, down: false, enter: false })).toBe(false);
  });

  it("is asked with the LEVELS, not with React state — pinned at the call site", () => {
    /**
     * The predicate above is worthless if its caller hands it a stale answer, and the first version of
     * this fix did exactly that.
     *
     * `handleButtonPressStart` originally tested the `pressed` state. The button panel's own
     * "BtnA + BtnB + BtnC — hold 3 s" control calls `onPressStart` for all three buttons synchronously
     * inside one click handler, so React has not re-rendered when ENTER's turn comes and `pressed` still
     * reads all-false — including `enter`.
     *
     * WHICH WAY THAT FAILS MATTERS, and an earlier version of this comment had it backwards. The
     * predicate requires `enter`, so an all-false argument returns false and arms NOTHING: a
     * `pressed`-based guard fails CLOSED. The three-button path would have been accidentally safe, and
     * hold-to-confirm would have died on every confirm screen instead — which is why `levelsNow()` is
     * right for a plain reason rather than a dramatic one. It is the hook's ref, and a ref read after
     * `press()` is what `isPressed()` answers on the device. The two cases below assert both halves.
     *
     * A source regex because the fact is invisible to every other check and fails SILENTLY: the app
     * still compiles, the panel still looks right, and the only symptom is either a destructive action
     * firing during a recovery gesture (no guard) or no confirm screen working at all (a stale guard).
     * Same shape and same justification as the firmware pins above, and as
     * `httpd_task_policy_test.cpp` reading `firmware.cpp` for the task priorities.
     */
    // Half one: what a stale all-false argument does. This is the fail-CLOSED half, stated as a check so
    // the reasoning in the comment above cannot drift from the predicate again.
    expect(holdCountdownArms({ up: false, down: false, enter: false })).toBe(false);
    // Half two: the level-derived argument, with ENTER's own level already set by press(), arms.
    expect(holdCountdownArms({ up: false, down: false, enter: true })).toBe(true);

    const app = fs.readFileSync(path.join(__dirname, "..", "..", "App.tsx"), "utf-8");
    expect(
      /holdCountdownArms\(levelsNow\(\)\)/.test(app),
      "App.tsx must ask holdCountdownArms with levelsNow(), never with the `pressed` state"
    ).toBe(true);
    expect(
      /holdCountdownArms\(\s*\{[^}]*pressed\./.test(app),
      "and must not reintroduce a `pressed`-derived argument"
    ).toBe(false);
  });

  it("is the firmware's own condition, quoted from interaction_handler.cpp", () => {
    const source = firmwareSource("input", "interaction_handler.cpp");
    const body = source.slice(source.indexOf("void InteractionHandler::handleHoldCountdown"));
    // `if (enterHeld && !otherHeld) { ... armHoldCountdown(...) }`, with otherHeld built from UP|DOWN.
    expect(
      /const bool otherHeld =[\s\S]{0,160}Button::Up\)\s*\|\|[\s\S]{0,80}Button::Down\)/.test(body),
      "otherHeld must still mean UP or DOWN"
    ).toBe(true);
    expect(
      /if\s*\(enterHeld\s*&&\s*!otherHeld\)/.test(body),
      "arming must still require ENTER alone"
    ).toBe(true);
  });
});

describe("visibility: a guard at the gesture boundary draws no countdown", () => {
  it("pins the boundary against interaction_handler.h", () => {
    const header = firmwareSource("input", "interaction_handler.h");
    const match = /kGestureLongPressMs\s*=\s*(\d+)/.exec(header);
    expect(match, "interaction_handler.h must still declare kGestureLongPressMs").not.toBeNull();
    expect(kGestureLongPressMs).toBe(Number(match![1]));
  });

  it("is STRICTLY greater, matching the firmware's `>`", () => {
    const source = firmwareSource("input", "interaction_handler.cpp");
    expect(
      /holdCountdown_\.durationMs\s*>\s*kGestureLongPressMs/.test(source),
      "the countdown must still be gated on `> kGestureLongPressMs`"
    ).toBe(true);
    expect(holdCountdownIsVisible(kGestureLongPressMs)).toBe(false);
    expect(holdCountdownIsVisible(kGestureLongPressMs + 1)).toBe(true);
    expect(holdCountdownIsVisible(3000)).toBe(true);
    expect(holdCountdownIsVisible(30000)).toBe(true);
  });

  it("renders what ui_bindings.cpp renders, `0 s` included", () => {
    // At rest, on any confirm screen: `countdownSeconds` is zero and the binding prints it anyway.
    expect(holdCountdownText(null)).toBe("0 s");
    // A 1.5 s guard NEVER writes the number, so it reads `0 s` for the whole hold.
    expect(holdCountdownText({ remainingMs: 1400, totalMs: 1500 })).toBe("0 s");
    // A 3 s guard counts, rounded up so it opens at its full duration rather than one below it.
    expect(holdCountdownText({ remainingMs: 3000, totalMs: 3000 })).toBe("3 s");
    expect(holdCountdownText({ remainingMs: 2001, totalMs: 3000 })).toBe("3 s");
    expect(holdCountdownText({ remainingMs: 2000, totalMs: 3000 })).toBe("2 s");
    expect(holdCountdownText({ remainingMs: 1, totalMs: 30000 })).toBe("1 s");
    expect(holdCountdownText({ remainingMs: 0, totalMs: 30000 })).toBe("0 s");
  });
});

describe("the dataset this actually decides", () => {
  it("has confirm screens on BOTH sides of the boundary, all six binding countdown.value", () => {
    /**
     * The reason the visibility rule is not academic. Two of the shipped guards are 1500 ms and both
     * draw a `countdown.value` element, so on hardware those two screens show a static `0 s` while
     * their four neighbours count down. If that ever stops being true — the dataset drops the element,
     * or the durations move off the boundary — this test says so, because the divergence it describes
     * would have quietly become a different one.
     */
    const holds = datasetScreens()
      .screens.map((screen) => ({
        id: screen.id,
        durationMs: screen.flows?.find(
          (flow) => flow.trigger.type === "timeout" && flow.trigger.holdButton === "enter"
        )?.trigger.durationMs,
        bindsCountdown: screen.elements.some((element) => element.binding === "countdown.value")
      }))
      .filter((screen) => screen.durationMs !== undefined);

    expect(holds.length).toBeGreaterThan(0);
    const frozen = holds.filter((screen) => !holdCountdownIsVisible(screen.durationMs!));
    const counting = holds.filter((screen) => holdCountdownIsVisible(screen.durationMs!));
    expect(frozen.map((screen) => screen.id).sort()).toEqual([
      "confirm-reset-max-flow",
      "confirm-reset-session"
    ]);
    expect(counting.length).toBe(4);
    // Every one of them draws the element, which is why the frozen two are visible as a defect at all.
    expect(holds.every((screen) => screen.bindsCountdown || screen.id === "confirm-factory-reset")).toBe(
      true
    );
  });
});
