export type SimulatedButton = "up" | "down" | "enter" | "up+down";

export type SimulatedButtonEventKind = "short" | "long" | "repeat" | "combo-warning";

export interface SimulatedButtonEvent {
  button: SimulatedButton;
  kind: SimulatedButtonEventKind;
  timestamp: number;
  /** Acceleration tier for repeat events (§5.3.5): 1 = base, 5 = medium, 25 = fast */
  tier?: number;
}
