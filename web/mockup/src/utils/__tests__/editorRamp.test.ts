import fs from "node:fs";
import path from "node:path";
import { describe, expect, it } from "vitest";
import { accelerationTier, kAccelTier2Ms, kAccelTier3Ms } from "../accel";
import {
  EditorRampInput,
  EditorRampState,
  initialEditorRampState,
  tickEditorRamp
} from "../editorRamp";

const firmwareSource = (...parts: string[]) =>
  fs.readFileSync(
    path.join(__dirname, "..", "..", "..", "..", "..", "Water-Flow-Meter-PlatformIO", "src", ...parts),
    "utf-8"
  );

/** A pass on an open editor with UP held since `pressStartMs`. */
function holdingUp(pressStartMs = 0, editorOwns = true): EditorRampInput {
  return { up: true, down: false, editorOwns, pressStartMs: { up: pressStartMs, down: 0 } };
}

/**
 * Drive the machine the way the hook does — arm, then pass at whatever it asks for — and collect the
 * steps. Returns the multipliers in order, which is the thing an operator experiences.
 */
function rampFor(totalMs: number): { atMs: number; multiplier: number }[] {
  let state: EditorRampState = initialEditorRampState();
  const steps: { atMs: number; multiplier: number }[] = [];
  let nowMs = 0;
  let next: number | null = 0;
  while (next !== null && nowMs <= totalMs) {
    const result = tickEditorRamp(state, holdingUp(0), nowMs);
    state = result.state;
    if (result.step) {
      steps.push({ atMs: nowMs, multiplier: result.step.multiplier });
    }
    next = result.nextPassMs;
    if (next === null) break;
    nowMs += next;
  }
  return steps;
}

describe("the editor ramp mirrors handleEditorRepeat", () => {
  it("arms without stepping, so a tap is not counted twice", () => {
    // The release short-press is the tap's +/-1. An arming pass that also stepped would make every tap
    // move the value by two, which is the bug that the firmware's own comment warns about at :148.
    const result = tickEditorRamp(initialEditorRampState(), holdingUp(0), 0);
    expect(result.step).toBeNull();
    expect(result.discard).toEqual([]);
    expect(result.state.active).toBe(true);
    expect(result.state.stepped).toBe(false);
  });

  it("takes 250 ms to the first step, not the queue's 1500 ms", () => {
    // This is the reported bug: the simulator had only the event queue, whose repeats begin at 1500 ms,
    // so a held UP looked dead for a second and a half and then paged the ring instead of stepping.
    const armed = tickEditorRamp(initialEditorRampState(), holdingUp(0), 0);
    expect(armed.nextPassMs).toBe(250);
    const stepped = tickEditorRamp(armed.state, holdingUp(0), 250);
    expect(stepped.step).toEqual({ button: "up", multiplier: 1, heldMs: 250 });
  });

  it("does not step early when a pass arrives before the interval", () => {
    const armed = tickEditorRamp(initialEditorRampState(), holdingUp(0), 0);
    const early = tickEditorRamp(armed.state, holdingUp(0), 100);
    expect(early.step).toBeNull();
    expect(early.nextPassMs).toBe(150);
    expect(early.state).toBe(armed.state);
  });

  it("accelerates 1 -> 5 -> 25 at the declared thresholds", () => {
    const multipliers = rampFor(4000).map((step) => step.multiplier);
    expect(multipliers[0]).toBe(1);
    expect(new Set(multipliers)).toEqual(new Set([1, 5, 25]));
    // Monotonic: a tier may never be revisited once passed, or a long hold would slow down.
    const firstFive = multipliers.indexOf(5);
    const firstTwentyFive = multipliers.indexOf(25);
    expect(firstFive).toBeGreaterThan(0);
    expect(firstTwentyFive).toBeGreaterThan(firstFive);
    expect(multipliers.slice(firstTwentyFive).every((m) => m === 25)).toBe(true);
  });

  it("crosses into each tier at the moment the threshold says", () => {
    const steps = rampFor(4000);
    const fiveAt = steps.find((step) => step.multiplier === 5)!.atMs;
    const twentyFiveAt = steps.find((step) => step.multiplier === 25)!.atMs;
    expect(fiveAt).toBeGreaterThanOrEqual(kAccelTier2Ms);
    expect(twentyFiveAt).toBeGreaterThanOrEqual(kAccelTier3Ms);
    // And not late: the step following the threshold, not several intervals after it.
    expect(fiveAt).toBeLessThan(kAccelTier2Ms + accelerationTier(0).intervalMs);
    expect(twentyFiveAt).toBeLessThan(kAccelTier3Ms + accelerationTier(kAccelTier2Ms).intervalMs);
  });

  it("reports total travel far beyond what tapping could reach", () => {
    // The point of the ramp: 19200 baud out of 1200 is unreachable one tap at a time. Three seconds of
    // holding must move the value by hundreds of steps, not tens.
    const total = rampFor(3000).reduce((sum, step) => sum + step.multiplier, 0);
    expect(total).toBeGreaterThan(200);
  });

  it("discards the button's queued events on every step", () => {
    // `buttonInput.discardEvents(button)` — otherwise the ring's repeats page the menu underneath the
    // editor, which is the other half of the reported bug.
    const armed = tickEditorRamp(initialEditorRampState(), holdingUp(0), 0);
    const stepped = tickEditorRamp(armed.state, holdingUp(0), 250);
    expect(stepped.discard).toEqual(["up"]);
  });

  it("swallows the release only when it actually stepped", () => {
    const armed = tickEditorRamp(initialEditorRampState(), holdingUp(0), 0);
    const stepped = tickEditorRamp(armed.state, holdingUp(0), 250);

    // Released after stepping: the queue must lose its short press, or a +25 hold also lands a +1.
    const releasedAfterStep = tickEditorRamp(
      stepped.state,
      { up: false, down: false, editorOwns: true, pressStartMs: { up: 0, down: 0 } },
      260
    );
    expect(releasedAfterStep.discard).toEqual(["up"]);

    // Released before stepping: the short press MUST survive — "a short press is always exactly +/-1".
    const releasedBeforeStep = tickEditorRamp(
      armed.state,
      { up: false, down: false, editorOwns: true, pressStartMs: { up: 0, down: 0 } },
      100
    );
    expect(releasedBeforeStep.discard).toEqual([]);
    expect(releasedBeforeStep.state).toEqual(initialEditorRampState());
  });

  it("stands down when no editor is open, leaving the ring its repeats", () => {
    // A setting screen is NOT an editor: the device pages the ring on a held UP there, and the simulator
    // must keep doing so. Silently stepping a value here would be a different lie than the one being fixed.
    const result = tickEditorRamp(initialEditorRampState(), holdingUp(0, false), 0);
    expect(result.step).toBeNull();
    expect(result.nextPassMs).toBeNull();
    expect(result.state.active).toBe(false);
  });

  it("treats both-held and neither-held as no adjustment", () => {
    const armed = tickEditorRamp(initialEditorRampState(), holdingUp(0), 0);
    const stepped = tickEditorRamp(armed.state, holdingUp(0), 250);
    for (const levels of [
      { up: true, down: true },
      { up: false, down: false }
    ]) {
      const result = tickEditorRamp(
        stepped.state,
        { ...levels, editorOwns: true, pressStartMs: { up: 0, down: 0 } },
        300
      );
      expect(result.step).toBeNull();
      expect(result.state.active).toBe(false);
    }
  });

  it("restarts the clock when the operator switches direction", () => {
    // Swapping UP for DOWN must not inherit UP's tier, or letting go of UP and pressing DOWN would fly
    // 25 steps at a time in the other direction.
    const armed = tickEditorRamp(initialEditorRampState(), holdingUp(0), 0);
    let state = armed.state;
    for (let nowMs = 250; nowMs <= 2000; nowMs += 150) {
      state = tickEditorRamp(state, holdingUp(0), nowMs).state;
    }
    const switched = tickEditorRamp(
      state,
      { up: false, down: true, editorOwns: true, pressStartMs: { up: 0, down: 2000 } },
      2000
    );
    expect(switched.step).toBeNull();
    expect(switched.state.button).toBe("down");
    expect(switched.state.stepped).toBe(false);

    const firstDownStep = tickEditorRamp(
      switched.state,
      { up: false, down: true, editorOwns: true, pressStartMs: { up: 0, down: 2000 } },
      2250
    );
    expect(firstDownStep.step?.multiplier).toBe(1);
  });
});

describe("the mockup and the firmware agree on the acceleration tiers", () => {
  it("declares the same thresholds as ui_accel.h", () => {
    const source = firmwareSource("ui", "core", "ui_accel.h");
    const tier2 = /kAccelTier2Ms\s*=\s*(\d+)/.exec(source);
    const tier3 = /kAccelTier3Ms\s*=\s*(\d+)/.exec(source);
    expect(tier2, "firmware must declare kAccelTier2Ms").not.toBeNull();
    expect(tier3, "firmware must declare kAccelTier3Ms").not.toBeNull();
    expect(Number(tier2![1])).toBe(kAccelTier2Ms);
    expect(Number(tier3![1])).toBe(kAccelTier3Ms);
  });

  it("returns the same {multiplier, intervalMs} pairs as accelerationTier does in C++", () => {
    // The tiers are duplicated across a language boundary, so read them rather than trust them. The
    // firmware writes them as brace-initialised AccelTier literals, in tier order.
    const source = firmwareSource("ui", "core", "ui_accel.h");
    const body = source.slice(source.indexOf("accelerationTier"));
    const pairs = [...body.matchAll(/return\s*\{\s*(\d+)\s*,\s*(\d+)\s*\}/g)].map((match) => ({
      multiplier: Number(match[1]),
      intervalMs: Number(match[2])
    }));
    expect(pairs, "ui_accel.h must return three tiers").toHaveLength(3);
    expect(accelerationTier(0)).toEqual(pairs[0]);
    expect(accelerationTier(kAccelTier2Ms)).toEqual(pairs[1]);
    expect(accelerationTier(kAccelTier3Ms)).toEqual(pairs[2]);
    // And the boundaries belong to the LOWER tier on the way in, matching `<` in both languages.
    expect(accelerationTier(kAccelTier2Ms - 1)).toEqual(pairs[0]);
    expect(accelerationTier(kAccelTier3Ms - 1)).toEqual(pairs[1]);
  });
});
