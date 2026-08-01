export type SimulatedButton = "up" | "down" | "enter" | "up+down";

export type SimulatedButtonEventKind = "short" | "long" | "repeat" | "combo-warning";

export interface SimulatedButtonEvent {
  button: SimulatedButton;
  kind: SimulatedButtonEventKind;
  timestamp: number;
}
