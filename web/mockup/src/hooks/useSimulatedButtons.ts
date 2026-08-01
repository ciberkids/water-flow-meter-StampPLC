import { useCallback, useRef, useState } from "react";
import { SimulatedButton, SimulatedButtonEvent, SimulatedButtonEventKind } from "../types/buttonSimulation";

const LONG_PRESS_MS = 1500;
const COMBO_WARNING_MS = 3000;
const REPEAT_INTERVAL_MS = 250;
type TimerHandle = ReturnType<typeof setTimeout>;
type IntervalHandle = ReturnType<typeof setInterval>;

type SingleButton = "up" | "down" | "enter";

interface ButtonRuntimeState {
  pressed: boolean;
  longPressTriggered: boolean;
  longPressTimer?: TimerHandle;
  repeatTimer?: IntervalHandle;
}

interface ComboRuntimeState {
  active: boolean;
  longTriggered: boolean;
  warningTriggered: boolean;
  longTimer?: TimerHandle;
  warningTimer?: TimerHandle;
}

export interface UseSimulatedButtonsResult {
  pressed: Record<SingleButton, boolean>;
  comboActive: boolean;
  press: (button: SingleButton) => void;
  release: (button: SingleButton) => void;
  cancelAll: () => void;
}

export function useSimulatedButtons(onEvent: (event: SimulatedButtonEvent) => void): UseSimulatedButtonsResult {
  const [pressed, setPressed] = useState<Record<SingleButton, boolean>>({
    up: false,
    down: false,
    enter: false
  });
  const [comboActive, setComboActive] = useState(false);

  const runtimeRef = useRef<Record<SingleButton, ButtonRuntimeState>>({
    up: { pressed: false, longPressTriggered: false },
    down: { pressed: false, longPressTriggered: false },
    enter: { pressed: false, longPressTriggered: false }
  });

  const comboRef = useRef<ComboRuntimeState>({
    active: false,
    longTriggered: false,
    warningTriggered: false
  });

  const emitEvent = useCallback(
    (button: SimulatedButton, kind: SimulatedButtonEventKind) => {
      onEvent({ button, kind, timestamp: Date.now() });
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

  const clearComboTimers = useCallback(() => {
    const combo = comboRef.current;
    if (combo.longTimer) {
      clearTimeout(combo.longTimer);
      combo.longTimer = undefined;
    }
    if (combo.warningTimer) {
      clearTimeout(combo.warningTimer);
      combo.warningTimer = undefined;
    }
  }, []);

  const cancelCombo = useCallback(() => {
    clearComboTimers();
    comboRef.current.active = false;
    comboRef.current.longTriggered = false;
    comboRef.current.warningTriggered = false;
    setComboActive(false);
  }, [clearComboTimers]);

  const startCombo = useCallback(() => {
    const combo = comboRef.current;
    if (combo.active) return;

    // Cancel individual button long-press timers since we're entering combo mode
    clearTimers("up");
    clearTimers("down");

    combo.active = true;
    combo.longTriggered = false;
    combo.warningTriggered = false;
    setComboActive(true);

    // After LONG_PRESS_MS → fire combo long event
    combo.longTimer = setTimeout(() => {
      combo.longTriggered = true;
      emitEvent("up+down", "long");
    }, LONG_PRESS_MS);

    // After COMBO_WARNING_MS → fire warning overlay event
    combo.warningTimer = setTimeout(() => {
      combo.warningTriggered = true;
      emitEvent("up+down", "combo-warning");
    }, COMBO_WARNING_MS);
  }, [clearTimers, emitEvent]);

  const checkCombo = useCallback(() => {
    const rt = runtimeRef.current;
    if (rt.up.pressed && rt.down.pressed && !comboRef.current.active) {
      startCombo();
    }
  }, [startCombo]);

  const release = useCallback(
    (button: SingleButton) => {
      const runtime = runtimeRef.current[button];
      if (!runtime.pressed) {
        return;
      }

      // If combo was active and this is UP or DOWN, cancel the combo
      if (comboRef.current.active && (button === "up" || button === "down")) {
        const wasLongTriggered = comboRef.current.longTriggered;
        cancelCombo();

        runtime.pressed = false;
        runtime.longPressTriggered = false;
        setPressed((current) => ({ ...current, [button]: false }));

        // If combo long was triggered, don't fire individual short events
        if (wasLongTriggered) {
          return;
        }
        // Cancelled before long threshold — no individual event either (it was a combo attempt)
        return;
      }

      clearTimers(button);

      if (!runtime.longPressTriggered) {
        emitEvent(button, "short");
      }

      runtime.pressed = false;
      runtime.longPressTriggered = false;
      setPressed((current) => ({ ...current, [button]: false }));
    },
    [cancelCombo, clearTimers, emitEvent]
  );

  const press = useCallback(
    (button: SingleButton) => {
      const runtime = runtimeRef.current[button];
      if (runtime.pressed) {
        return;
      }

      runtime.pressed = true;
      runtime.longPressTriggered = false;
      setPressed((current) => ({ ...current, [button]: true }));

      // Check if this completes a combo
      if (button === "up" || button === "down") {
        // Small delay to allow the other button to be pressed "simultaneously"
        setTimeout(() => checkCombo(), 50);
      }

      runtime.longPressTimer = setTimeout(() => {
        // Don't fire individual long press if combo is active
        if (comboRef.current.active) return;

        runtime.longPressTriggered = true;
        emitEvent(button, "long");

        if (button !== "enter") {
          // Display_UI_Requirements §3.1: "UP / DOWN held — Repeat every 250 ms when
          // navigating; accelerating adjust in a numeric editor (§5.4)."
          //
          // Flat, not accelerating. This hook used to ramp the interval 250 -> 150 -> 150 ms
          // and tag each event with a §5.4 tier multiplier, which is the editor's contract,
          // not navigation's — and the mockup previews navigation only, so every repeat it
          // emits is a navigation repeat. The multiplier was never read by any consumer, and
          // the flat REPEAT_INTERVAL_MS above had been left declared and unused, which is the
          // fingerprint of the tier code displacing it.
          //
          // ENTER is excluded because ENTER-held is a countdown, never a repeat — the same
          // rule ButtonInputManager applies on the device.
          runtime.repeatTimer = setInterval(() => {
            if (comboRef.current.active) return;
            onEvent({ button, kind: "repeat", timestamp: Date.now() });
          }, REPEAT_INTERVAL_MS);
        }
      }, LONG_PRESS_MS);
    },
    [checkCombo, emitEvent, onEvent]
  );


  const cancelAll = useCallback(() => {
    cancelCombo();
    (Object.keys(runtimeRef.current) as SingleButton[]).forEach((button) => {
      if (runtimeRef.current[button].pressed) {
        release(button);
      } else {
        clearTimers(button);
      }
    });
  }, [cancelCombo, clearTimers, release]);

  return { pressed, comboActive, press, release, cancelAll };
}
