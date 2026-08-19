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
 * This is `InteractionHandler::matchFlow` and must stay it, gesture for gesture. The firmware maps an
 * event to exactly one `FlowGesture` and then requires an exact match:
 *
 *   `mapGesture`:  !isLongPress -> Short  ·  isRepeat -> Hold  ·  otherwise -> Long
 *   `matchFlow`:   trigger == Button && flow.button == button && flow.gesture == gesture
 *
 * There is no fallback of any kind. A repeat asks for `hold` and gets `hold` or nothing.
 *
 * This file used to answer a repeat by taking the `short` flow instead whenever it named a target
 * screen, on the theory that "a repeat is the same transition again" and that §3.1's held UP/DOWN pages
 * the ring every 250 ms. §3.1 said that until it was amended on 2026-08-17, and the firmware does emit the
 * repeats — but NOTHING answers
 * them: the dataset declares 196 short flows, 13 long, and zero hold. So a held UP/DOWN navigates nowhere
 * on the device, and that invented fallback was the whole of the reported bug. Holding UP on a setting
 * page paged the operator several settings away from the one they were reading, which the device never
 * did, and it also fought the editor: on an editor screen it dropped `f-inc` for having no target, so the
 * value froze while the ramp was still missing.
 *
 * `Display_UI_Requirements` §3.1.1 has since withdrawn the repeating navigation step, so this is no longer
 * a gap of any kind — a held UP/DOWN moving nothing on a navigation screen is now the requirement. If a
 * `hold` flow is ever declared, it is declared in the pipeline, where the host tests can see it; the
 * preview may still never invent one on its own.
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
  return buttonFlowsFor(screen, event.button, "hold");
}
