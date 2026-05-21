import { ScreenDefinition, ScreenFlow } from "../types";
import { SimulatedButtonEvent } from "../types/buttonSimulation";

const DEFAULT_GESTURE = "short" as const;

function normaliseGesture(value: string | undefined): string {
  return value ?? DEFAULT_GESTURE;
}

export function findMatchingButtonFlows(
  screen: ScreenDefinition | undefined,
  event: SimulatedButtonEvent
): ScreenFlow[] {
  if (!screen?.flows || screen.flows.length === 0) {
    return [];
  }
  return screen.flows.filter((flow) => {
    if (!flow.trigger || flow.trigger.type !== "button") {
      return false;
    }
    if (flow.trigger.button !== event.button) {
      return false;
    }
    const expectedGesture = normaliseGesture(flow.trigger.gesture ?? DEFAULT_GESTURE);
    if (event.kind === "repeat") {
      return expectedGesture === DEFAULT_GESTURE;
    }
    return event.kind === expectedGesture;
  });
}
