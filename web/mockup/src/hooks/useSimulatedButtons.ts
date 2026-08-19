import { useCallback, useRef, useState } from "react";
import {
  EditorRampState,
  RampButton,
  initialEditorRampState,
  tickEditorRamp
} from "../utils/editorRamp";
import {
  SimulatedButton,
  SimulatedButtonEvent,
  SimulatedButtonEventKind
} from "../types/buttonSimulation";
import {
  ArmedCombo,
  ComboButtons,
  ComboEvent,
  ComboState,
  ConsumedButtons,
  applyButtons,
  armedComboOf,
  comboConsumed,
  initialComboState,
  kSelectorHoldMs,
  tickCombos
} from "../utils/comboGestures";

/** button_input.h:41-42 — the firmware's own thresholds, for single buttons only. */
const LONG_PRESS_MS = 1500;
const REPEAT_INTERVAL_MS = 250;

/**
 * How often to re-run the editor ramp while it has nothing to do.
 *
 * The device's answer is "every loop pass", which is far faster than this; a quarter second is the coarsest
 * period at which the ramp cannot miss a state change the operator would notice, since the fastest thing it
 * can do is step once per 150 ms.
 */
const RAMP_IDLE_PASS_MS = 250;

/**
 * How an open editor takes UP/DOWN away from the navigation ring (§5.4).
 *
 * The firmware runs `handleEditorRepeat` on EVERY loop pass, before it drains the event queue, and it
 * reads the button LEVELS rather than queued events — so an editor starts stepping about 250 ms after
 * the press, accelerates, and swallows everything that button had queued including the release
 * short-press. The queue's own repeats do not begin until 1500 ms and belong to the ring.
 *
 * That distinction is the whole reason a held UP did nothing in an editor here and paged the ring on a
 * setting screen: the simulator only ever had the queue.
 */
export interface EditorRepeatBridge {
  /** `editor().active && editor().setting` — is an editor open and owning UP/DOWN? */
  owns: () => boolean;
  /** One accelerated step: the setting's own `step`, times `multiplier`, signed by the button. */
  step: (button: "up" | "down", multiplier: number, heldMs: number) => void;
}

type TimerHandle = ReturnType<typeof setTimeout>;
type IntervalHandle = ReturnType<typeof setInterval>;

type SingleButton = "up" | "down" | "enter";

interface ButtonRuntimeState {
  longPressTriggered: boolean;
  longPressTimer?: TimerHandle;
  repeatTimer?: IntervalHandle;
  /** When the level went high, for the acceleration tier — `state.pressStartMs`. */
  pressStartMs: number;
  /** The editor's ramp, which reschedules itself at the current tier's interval. */
  editorTimer?: TimerHandle;
  /** Whether the ramp has stepped, which is what makes the release swallow-worthy. */
  editorStepped: boolean;
}

export interface UseSimulatedButtonsResult {
  pressed: Record<SingleButton, boolean>;
  /** Which multi-button gesture is armed right now, for the panel's status line. */
  armedCombo: ArmedCombo;
  press: (button: SingleButton) => void;
  release: (button: SingleButton) => void;
  cancelAll: () => void;
  /**
   * The button LEVELS as of right now — `buttonInput.isPressed()`, not `pressed`.
   *
   * `pressed` is React state, so it is one render behind inside the handler that just pressed a button.
   * That is fatal for anything deciding a gesture at press time: the panel's three-button control calls
   * `onPressStart` for UP, DOWN and ENTER synchronously in one click handler, so ENTER's handler still
   * sees `pressed` as all-false and any level test against it answers "ENTER alone". The firmware has no
   * such gap — `isPressed()` reads the pin state — and this getter is that read.
   */
  levelsNow: () => ComboButtons;
}

/**
 * Adapter between pointer/keyboard input and the device's event vocabulary.
 *
 * All combo logic lives in utils/comboGestures.ts — a pure level-test machine that mirrors
 * interaction_handler.cpp and is unit-tested to the millisecond. This file owns only what a browser
 * forces on us: React state, and the timers that stand in for the firmware's polling loop.
 *
 * What it no longer does, and why:
 *
 *   - It no longer defers combo arming by 50 ms. That deferral meant a fast tap released both buttons
 *     before the check ran, so the display-off gesture — which fires on RELEASE inside 1 s — could never
 *     be performed by clicking. A clickable combo control would have been a permanent no-op.
 *   - It no longer emits `up+down`/`long` at 1500 ms or `combo-warning` at 3000 ms. The device has no
 *     UP+DOWN-held event at any duration, and 3000 ms belongs to the three-button selector.
 *   - It no longer ignores ENTER. `checkCombo` tested `up.pressed && down.pressed` only, which is why
 *     UP+DOWN and UP+DOWN+ENTER produced identical events and ran the same handler.
 *
 * One deliberate divergence, recorded because it is a divergence: the firmware calls clearEvents() only
 * on the successful (<1 s) display-off path, so releasing UP+DOWN after a longer hold dispatches UP-short
 * then DOWN-short — two page moves whose net effect is to land back on the starting screen. The machine
 * here swallows those releases instead, which is what the firmware's own comment says the clearing is
 * for. The visible difference is two trace entries, not a different screen.
 */
export function useSimulatedButtons(
  onEvent: (event: SimulatedButtonEvent) => void,
  editorRepeat?: EditorRepeatBridge
): UseSimulatedButtonsResult {
  const [pressed, setPressed] = useState<Record<SingleButton, boolean>>({
    up: false,
    down: false,
    enter: false
  });
  const [armedCombo, setArmedCombo] = useState<ArmedCombo>(null);

  const runtimeRef = useRef<Record<SingleButton, ButtonRuntimeState>>({
    up: { longPressTriggered: false, pressStartMs: 0, editorStepped: false },
    down: { longPressTriggered: false, pressStartMs: 0, editorStepped: false },
    enter: { longPressTriggered: false, pressStartMs: 0, editorStepped: false }
  });

  /** Read through a ref so a re-rendered bridge does not rebuild `press`, which owns live timers. */
  const editorRepeatRef = useRef<EditorRepeatBridge | undefined>(editorRepeat);
  editorRepeatRef.current = editorRepeat;

  /** The button LEVELS, which is what the machine consumes. */
  const levelsRef = useRef<ComboButtons>({ up: false, down: false, enter: false });
  const comboRef = useRef<ComboState>(initialComboState());
  /** §5.4's ramp, the other pure machine this file drives. */
  const rampRef = useRef<EditorRampState>(initialEditorRampState());
  /** Fires the selector, which completes while held and so is marked by no button change. */
  const selectorTimerRef = useRef<TimerHandle | undefined>(undefined);

  const emit = useCallback(
    (button: SimulatedButton, kind: SimulatedButtonEventKind, timestamp: number) => {
      onEvent({ button, kind, timestamp });
    },
    [onEvent]
  );

  const clearTimers = useCallback((button: SingleButton) => {
    const runtime = runtimeRef.current[button];
    if (runtime.longPressTimer) {
      clearTimeout(runtime.longPressTimer);
      runtime.longPressTimer = undefined;
    }
    if (runtime.repeatTimer) {
      clearInterval(runtime.repeatTimer);
      runtime.repeatTimer = undefined;
    }
    if (runtime.editorTimer) {
      clearTimeout(runtime.editorTimer);
      runtime.editorTimer = undefined;
    }
  }, []);

  const clearSelectorTimer = useCallback(() => {
    if (selectorTimerRef.current) {
      clearTimeout(selectorTimerRef.current);
      selectorTimerRef.current = undefined;
    }
  }, []);

  const publishArmed = useCallback((state: ComboState) => {
    setArmedCombo(armedComboOf(state));
  }, []);

  const dispatchComboEvents = useCallback(
    (events: readonly ComboEvent[], nowMs: number) => {
      for (const event of events) {
        if (event.kind === "display-off") {
          emit("up+down", "short", nowMs);
        } else {
          emit("up+down+enter", "long", nowMs);
        }
      }
    },
    [emit]
  );

  /**
   * Re-arm the selector's deadline.
   *
   * The machine fires the selector from tickCombos, so something has to call it while the buttons are
   * simply held. One timeout, aimed at the exact remaining milliseconds, replaces a polling interval.
   */
  const scheduleSelector = useCallback(
    (state: ComboState, nowMs: number) => {
      clearSelectorTimer();
      if (!state.selector.active || state.selector.fired) {
        return;
      }
      const remaining = Math.max(0, state.selector.startMs + kSelectorHoldMs - nowMs);
      selectorTimerRef.current = setTimeout(() => {
        selectorTimerRef.current = undefined;
        const firedAt = Date.now();
        const result = tickCombos(comboRef.current, levelsRef.current, firedAt);
        comboRef.current = result.state;
        publishArmed(result.state);
        dispatchComboEvents(result.events, firedAt);
      }, remaining);
    },
    [clearSelectorTimer, dispatchComboEvents, publishArmed]
  );

  /** Feed the machine the current levels, synchronously, and act on whatever it returns. */
  const applyLevels = useCallback(
    (nowMs: number): ConsumedButtons => {
      const update = applyButtons(comboRef.current, levelsRef.current, nowMs);
      comboRef.current = update.state;
      publishArmed(update.state);
      dispatchComboEvents(update.events, nowMs);
      scheduleSelector(update.state, nowMs);
      return update.consumed;
    },
    [dispatchComboEvents, publishArmed, scheduleSelector]
  );

  /**
   * One loop pass of the ramp, rescheduling itself for as long as the machine asks.
   *
   * All the decisions belong to `tickEditorRamp`; this owns only what the firmware gets for free from
   * running in a loop — being called again — plus the browser-side halves of `discardEvents`: cancelling
   * the queue timers that would otherwise page the ring underneath the editor.
   */
  const runEditorPass = useCallback(
    (button: RampButton) => {
      const runtime = runtimeRef.current[button];
      runtime.editorTimer = undefined;

      const levels = levelsRef.current;
      const bridge = editorRepeatRef.current;
      const result = tickEditorRamp(
        rampRef.current,
        {
          up: levels.up,
          down: levels.down,
          // A button the combo machine has consumed is part of a gesture, not an adjustment.
          editorOwns: Boolean(bridge?.owns()) && !comboConsumed(comboRef.current)[button],
          pressStartMs: {
            up: runtimeRef.current.up.pressStartMs,
            down: runtimeRef.current.down.pressStartMs
          }
        },
        Date.now()
      );
      rampRef.current = result.state;

      for (const discarded of result.discard) {
        // `buttonInput.discardEvents(button)`: the editor owns this button, so the queue's long press and
        // its 250 ms ring repeats must not also fire. Without this the ramp would step the value AND page
        // the ring — the two-homes version of the very bug being fixed.
        clearTimers(discarded);
        runtimeRef.current[discarded].editorStepped = true;
      }

      if (result.step) {
        bridge?.step(result.step.button, result.step.multiplier, result.step.heldMs);
      }

      /**
       * Keep passing for as long as the level is high, even when the machine is idle.
       *
       * The firmware gets this free: `handleEditorRepeat` runs every loop pass whether or not it has
       * anything to do, so a ramp that stood down — both buttons briefly held, an editor not open yet —
       * re-arms on the next pass. Stopping at the first idle result instead would leave the ramp dead for
       * the rest of the hold, and the operator holding a button that had just worked.
       */
      if (levels[button]) {
        const nextMs = result.nextPassMs ?? RAMP_IDLE_PASS_MS;
        runtime.editorTimer = setTimeout(() => runEditorPass(button), nextMs);
      }
    },
    [clearTimers]
  );

  const press = useCallback(
    (button: SingleButton) => {
      if (levelsRef.current[button]) {
        return;
      }
      const nowMs = Date.now();
      levelsRef.current = { ...levelsRef.current, [button]: true };
      const runtime = runtimeRef.current[button];
      runtime.longPressTriggered = false;
      runtime.pressStartMs = nowMs;
      runtime.editorStepped = false;
      setPressed((current) => ({ ...current, [button]: true }));

      applyLevels(nowMs);

      /**
       * §5.4's ramp is armed for UP/DOWN unconditionally, and decides on its first pass.
       *
       * It cannot be decided here: the editor may be opened or closed while the button is already down,
       * and the firmware re-tests it every loop pass rather than at the press. The first pass lands at
       * the first tier's 250 ms, which is why a held UP starts moving the value almost at once on the
       * device while the ring's own repeats wait for 1500 ms.
       */
      if (button !== "enter") {
        runEditorPass(button);
      }

      // A combo that is armed swallows the participating buttons' long press and repeats too — the
      // firmware's clearEvents() drains the queue rather than filtering it by kind.
      runtime.longPressTimer = setTimeout(() => {
        runtime.longPressTimer = undefined;
        if (comboConsumed(comboRef.current)[button]) {
          return;
        }
        runtime.longPressTriggered = true;
        emit(button, "long", Date.now());

        if (button !== "enter") {
          // These repeats mirror `button_input.cpp`'s event stream: a long press at 1.5 s, then a repeat
          // every 250 ms, flat — the accelerating tiers belong to a numeric editor (§5.4) and are driven
          // by `runEditorPass`, which clears these timers the moment it takes the button. ENTER is
          // excluded because held ENTER is a countdown, never a repeat.
          //
          // Display_UI_Requirements §3.1.1 (2026-08-17) withdrew the repeating NAVIGATION step, so these
          // events exist for fidelity with the firmware queue, not to page a ring. They are answered only
          // where the firmware answers them: a `hold` flow if one is ever declared, and the Select Menu
          // cursor, which moves on any event kind. Do not delete this timer with the requirement.
          runtime.repeatTimer = setInterval(() => {
            if (comboConsumed(comboRef.current)[button]) {
              return;
            }
            emit(button, "repeat", Date.now());
          }, REPEAT_INTERVAL_MS);
        }
      }, LONG_PRESS_MS);
    },
    [applyLevels, emit, runEditorPass]
  );

  const release = useCallback(
    (button: SingleButton) => {
      if (!levelsRef.current[button]) {
        return;
      }
      const nowMs = Date.now();
      levelsRef.current = { ...levelsRef.current, [button]: false };
      const runtime = runtimeRef.current[button];
      clearTimers(button);
      setPressed((current) => ({ ...current, [button]: false }));

      const consumed = applyLevels(nowMs);

      // Order matters: the machine has already reported whether this release belongs to a combo, so the
      // single-button short press is suppressed on exactly the releases the device suppresses.
      //
      // `editorStepped` suppresses it too, for the reason interaction_handler.cpp:190 gives: the ramp
      // discards everything the button had queued INCLUDING the release, so a hold that stepped +25 does
      // not also land a +1 on the way up. A hold that never stepped — released under 250 ms, or held on a
      // screen with no editor — still emits its short press, because "a short press is always exactly ±1".
      if (!runtime.longPressTriggered && !consumed[button] && !runtime.editorStepped) {
        emit(button, "short", nowMs);
      }
      runtime.longPressTriggered = false;
      runtime.editorStepped = false;

      /**
       * Releasing one of UP/DOWN hands the ramp back to the other, if it is still down.
       *
       * Pressing both ends the ramp — `up == down` is not an adjustment — and ending it discards the
       * stepping button's queued events, which cancels its pass timer too. Without this kick the surviving
       * button would stay held with nothing scheduled to notice, so the operator would be leaning on a
       * button that had just been working. The firmware re-arms on its next loop pass and never notices.
       */
      const other: SingleButton | null = button === "up" ? "down" : button === "down" ? "up" : null;
      if (other && levelsRef.current[other] && !runtimeRef.current[other].editorTimer) {
        runEditorPass(other);
      }
    },
    [applyLevels, clearTimers, emit, runEditorPass]
  );

  const cancelAll = useCallback(() => {
    (Object.keys(runtimeRef.current) as SingleButton[]).forEach((button) => {
      clearTimers(button);
      runtimeRef.current[button].longPressTriggered = false;
      runtimeRef.current[button].editorStepped = false;
    });
    clearSelectorTimer();
    // A cancel is a lost pointer or a blurred window, not a gesture: the levels drop and the machine
    // resets without emitting anything. Feeding the release through applyLevels would fire display-off
    // for a window that merely lost focus.
    levelsRef.current = { up: false, down: false, enter: false };
    comboRef.current = initialComboState();
    setArmedCombo(null);
    setPressed({ up: false, down: false, enter: false });
  }, [clearSelectorTimer, clearTimers]);

  const levelsNow = useCallback(() => levelsRef.current, []);

  return { pressed, armedCombo, press, release, cancelAll, levelsNow };
}
