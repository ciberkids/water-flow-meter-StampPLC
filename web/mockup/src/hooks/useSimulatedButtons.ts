import { useCallback, useRef, useState } from "react";
import { SimulatedButton, SimulatedButtonEvent, SimulatedButtonEventKind } from "../types/buttonSimulation";

const LONG_PRESS_MS = 1500;
const REPEAT_INTERVAL_MS = 250;
type TimerHandle = ReturnType<typeof setTimeout>;
type IntervalHandle = ReturnType<typeof setInterval>;

interface ButtonRuntimeState {
  pressed: boolean;
  longPressTriggered: boolean;
  longPressTimer?: TimerHandle;
  repeatTimer?: IntervalHandle;
}

export interface UseSimulatedButtonsResult {
  pressed: Record<SimulatedButton, boolean>;
  press: (button: SimulatedButton) => void;
  release: (button: SimulatedButton) => void;
  cancelAll: () => void;
}

export function useSimulatedButtons(onEvent: (event: SimulatedButtonEvent) => void): UseSimulatedButtonsResult {
  const [pressed, setPressed] = useState<Record<SimulatedButton, boolean>>({
    up: false,
    down: false,
    enter: false
  });

  const runtimeRef = useRef<Record<SimulatedButton, ButtonRuntimeState>>({
    up: { pressed: false, longPressTriggered: false },
    down: { pressed: false, longPressTriggered: false },
    enter: { pressed: false, longPressTriggered: false }
  });

  const emitEvent = useCallback(
    (button: SimulatedButton, kind: SimulatedButtonEventKind) => {
      onEvent({ button, kind, timestamp: Date.now() });
    },
    [onEvent]
  );

  const clearTimers = useCallback((button: SimulatedButton) => {
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

  const release = useCallback(
    (button: SimulatedButton) => {
      const runtime = runtimeRef.current[button];
      if (!runtime.pressed) {
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
    [clearTimers, emitEvent]
  );

  const press = useCallback(
    (button: SimulatedButton) => {
      const runtime = runtimeRef.current[button];
      if (runtime.pressed) {
        return;
      }

      runtime.pressed = true;
      runtime.longPressTriggered = false;
      setPressed((current) => ({ ...current, [button]: true }));

      runtime.longPressTimer = setTimeout(() => {
        runtime.longPressTriggered = true;
        emitEvent(button, "long");
        if (button !== "enter") {
          runtime.repeatTimer = setInterval(() => {
            emitEvent(button, "repeat");
          }, REPEAT_INTERVAL_MS);
        }
      }, LONG_PRESS_MS);
    },
    [emitEvent]
  );

  const cancelAll = useCallback(() => {
    (Object.keys(runtimeRef.current) as SimulatedButton[]).forEach((button) => {
      if (runtimeRef.current[button].pressed) {
        release(button);
      } else {
        clearTimers(button);
      }
    });
  }, [clearTimers, release]);

  return { pressed, press, release, cancelAll };
}
