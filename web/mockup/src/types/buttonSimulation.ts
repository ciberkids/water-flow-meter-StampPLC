export type SimulatedButton = "up" | "down" | "enter";

export type SimulatedButtonEventKind = "short" | "long" | "repeat";

export interface SimulatedButtonEvent {
  button: SimulatedButton;
  kind: SimulatedButtonEventKind;
  timestamp: number;
}
