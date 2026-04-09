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
    // Clean up tiered acceleration timers
    const rt = runtime as any;
    if (rt._tier2Timer) { clearTimeout(rt._tier2Timer); rt._tier2Timer = undefined; }
    if (rt._tier3Timer) { clearTimeout(rt._tier3Timer); rt._tier3Timer = undefined; }
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
          // §5.3.5 Tiered acceleration:
          // Tier 1: 0–700ms after long press: ±1 every 250ms (base cadence)
          // Tier 2: 700ms–1.5s: ±5 every 150ms
          // Tier 3: >1.5s: ±25 every 150ms
          const TIER1_DURATION = 700;
          const TIER2_DURATION = 800; // 700 + 800 = 1500ms total
          const TIER1_INTERVAL = 250;
          const TIER2_INTERVAL = 150;
          const TIER3_INTERVAL = 150;

          // Start tier 1 repeats
          runtime.repeatTimer = setInterval(() => {
            if (comboRef.current.active) return;
            onEvent({ button, kind: "repeat", timestamp: Date.now(), tier: 1 });
          }, TIER1_INTERVAL);

          // After 700ms, switch to tier 2
          const tier2Timer = setTimeout(() => {
            if (!runtime.pressed || comboRef.current.active) return;
            if (runtime.repeatTimer) clearInterval(runtime.repeatTimer);
            runtime.repeatTimer = setInterval(() => {
              if (comboRef.current.active) return;
              onEvent({ button, kind: "repeat", timestamp: Date.now(), tier: 5 });
            }, TIER2_INTERVAL);

            // After another 800ms, switch to tier 3
            const tier3Timer = setTimeout(() => {
              if (!runtime.pressed || comboRef.current.active) return;
              if (runtime.repeatTimer) clearInterval(runtime.repeatTimer);
              runtime.repeatTimer = setInterval(() => {
                if (comboRef.current.active) return;
                onEvent({ button, kind: "repeat", timestamp: Date.now(), tier: 25 });
              }, TIER3_INTERVAL);
            }, TIER2_DURATION);

            // Store tier3 timer for cleanup
            (runtime as any)._tier3Timer = tier3Timer;
          }, TIER1_DURATION);

          // Store tier2 timer for cleanup
          (runtime as any)._tier2Timer = tier2Timer;
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
