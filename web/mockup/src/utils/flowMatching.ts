import { ScreenDefinition, ScreenFlow } from "../types";
import { SimulatedButtonEvent } from "../types/buttonSimulation";

const DEFAULT_GESTURE = "short" as const;

function normaliseGesture(value: string | undefined): string {
  return value ?? DEFAULT_GESTURE;
}

function buttonFlowsFor(
  screen: ScreenDefinition,
  button: SimulatedButtonEvent["button"],
  gesture: string
): ScreenFlow[] {
  return screen.flows!.filter((flow) => {
    if (!flow.trigger || flow.trigger.type !== "button") {
      return false;
    }
    if (flow.trigger.button !== button) {
      return false;
    }
    return normaliseGesture(flow.trigger.gesture ?? DEFAULT_GESTURE) === gesture;
  });
}

/**
 * Resolves a simulated button event to the flows it fires.
 *
 * The `repeat` case mirrors InteractionHandler::matchFlow in the firmware, and has to keep
 * mirroring it — a preview that pages when the device does not (or the reverse) is worse
 * than no preview at all.
 *
 * Display_UI_Requirements §3.1 says a held UP/DOWN repeats the navigation step every
 * 250 ms, and the dataset has no vocabulary for "a repeated step": a repeat is the same
 * transition again. So a repeat prefers an explicit `hold` flow and otherwise re-fires the
 * `short` one — but only when that flow names a target screen.
 *
 * The target-screen condition is the important half. A target-bearing UP/DOWN short flow is
 * a move to a sibling; a target-less one acts in place, and in-place actions must not
 * auto-fire. In the firmware that protects the §5.4 acceleration ramp, which owns a held
 * UP/DOWN inside a numeric editor, and the §5.5 Nyquist override's "Save anyway".
 */
export function findMatchingButtonFlows(
  screen: ScreenDefinition | undefined,
  event: SimulatedButtonEvent
): ScreenFlow[] {
  if (!screen?.flows || screen.flows.length === 0) {
    return [];
  }

  if (event.kind !== "repeat") {
    return buttonFlowsFor(screen, event.button, event.kind);
  }

  const holdFlows = buttonFlowsFor(screen, event.button, "hold");
  if (holdFlows.length > 0) {
    return holdFlows;
  }
  return buttonFlowsFor(screen, event.button, DEFAULT_GESTURE).filter(
    (flow) => Boolean(flow.targetScreenId)
  );
}
