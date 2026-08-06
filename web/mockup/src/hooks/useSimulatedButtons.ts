import { useCallback, useRef, useState } from "react";
import {
  ArmedCombo,
  SimulatedButton,
  SimulatedButtonEvent,
  SimulatedButtonEventKind
} from "../types/buttonSimulation";
import {
  ComboButtons,
  ComboEvent,
  ComboState,
  ConsumedButtons,
  applyButtons,
  comboConsumed,
  initialComboState,
  kSelectorHoldMs,
  tickCombos
} from "../utils/comboGestures";

/** button_input.h:41-42 — the firmware's own thresholds, for single buttons only. */
const LONG_PRESS_MS = 1500;
const REPEAT_INTERVAL_MS = 250;

type TimerHandle = ReturnType<typeof setTimeout>;
type IntervalHandle = ReturnType<typeof setInterval>;

type SingleButton = "up" | "down" | "enter";

interface ButtonRuntimeState {
  longPressTriggered: boolean;
  longPressTimer?: TimerHandle;
  repeatTimer?: IntervalHandle;
}

export interface UseSimulatedButtonsResult {
  pressed: Record<SingleButton, boolean>;
  /** Which multi-button gesture is armed right now, for the panel's status line. */
  armedCombo: ArmedCombo;
  press: (button: SingleButton) => void;
  release: (button: SingleButton) => void;
  cancelAll: () => void;
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
  onEvent: (event: SimulatedButtonEvent) => void
): UseSimulatedButtonsResult {
  const [pressed, setPressed] = useState<Record<SingleButton, boolean>>({
    up: false,
    down: false,
    enter: false
  });
  const [armedCombo, setArmedCombo] = useState<ArmedCombo>(null);

  const runtimeRef = useRef<Record<SingleButton, ButtonRuntimeState>>({
    up: { longPressTriggered: false },
    down: { longPressTriggered: false },
    enter: { longPressTriggered: false }
  });

  /** The button LEVELS, which is what the machine consumes. */
  const levelsRef = useRef<ComboButtons>({ up: false, down: false, enter: false });
  const comboRef = useRef<ComboState>(initialComboState());
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
  }, []);

  const clearSelectorTimer = useCallback(() => {
    if (selectorTimerRef.current) {
      clearTimeout(selectorTimerRef.current);
      selectorTimerRef.current = undefined;
    }
  }, []);

  const publishArmed = useCallback((state: ComboState) => {
    setArmedCombo(state.selector.active ? "selector" : state.displayOff.active ? "display-off" : null);
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

  const press = useCallback(
    (button: SingleButton) => {
      if (levelsRef.current[button]) {
        return;
      }
      const nowMs = Date.now();
      levelsRef.current = { ...levelsRef.current, [button]: true };
      const runtime = runtimeRef.current[button];
      runtime.longPressTriggered = false;
      setPressed((current) => ({ ...current, [button]: true }));

      applyLevels(nowMs);

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
          // Display_UI_Requirements §3.1: UP/DOWN held repeat every 250 ms when navigating. Flat, not
          // accelerating — the accelerating tiers belong to a numeric editor (§5.4), which the mockup
          // does not preview. ENTER is excluded because held ENTER is a countdown, never a repeat.
          runtime.repeatTimer = setInterval(() => {
            if (comboConsumed(comboRef.current)[button]) {
              return;
            }
            emit(button, "repeat", Date.now());
          }, REPEAT_INTERVAL_MS);
        }
      }, LONG_PRESS_MS);
    },
    [applyLevels, emit]
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
      if (!runtime.longPressTriggered && !consumed[button]) {
        emit(button, "short", nowMs);
      }
      runtime.longPressTriggered = false;
    },
    [applyLevels, clearTimers, emit]
  );

  const cancelAll = useCallback(() => {
    (Object.keys(runtimeRef.current) as SingleButton[]).forEach((button) => {
      clearTimers(button);
      runtimeRef.current[button].longPressTriggered = false;
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

  return { pressed, armedCombo, press, release, cancelAll };
}
