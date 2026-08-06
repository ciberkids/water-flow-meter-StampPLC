/**
 * What the simulated button hardware can emit.
 *
 * The two combo members are VIRTUAL — the device has three buttons, and the firmware recognises the
 * combinations outside its per-button event queue (interaction_handler.cpp handles them before the queue
 * drains, and the dataset's flow schema cannot express them at all: `button` is a single-value enum).
 *
 * `"up+down+enter"` exists because without it the two gestures were indistinguishable: the hook tested
 * only `up && down`, so holding ENTER as well produced byte-identical events and both gestures ran the
 * same handler.
 */
export type SimulatedButton = "up" | "down" | "enter" | "up+down" | "up+down+enter";

/**
 * `"combo-warning"` is gone. It fired at 3000 ms and drove a factory-reset warning overlay for a gesture
 * Display_UI_Requirements §3.3 retired and interaction_handler.cpp:226 confirms is deleted. The 3000 ms
 * value was right and the gesture was wrong: 3 s is `kSelectorComboHoldMs`, which belongs to
 * UP+DOWN+ENTER.
 */
export type SimulatedButtonEventKind = "short" | "long" | "repeat";

/**
 * A button event.
 *
 * Only these (button, kind) pairs are legal, and they are exactly the device's vocabulary:
 *   - a single button with `short`, `long` or `repeat` (ENTER never repeats — held ENTER is a countdown)
 *   - `up+down` with `short`      — display off, on RELEASE inside kDisplayOffComboMaxMs (1000 ms)
 *   - `up+down+enter` with `long` — Select Menu, once at kSelectorComboHoldMs (3000 ms), while still held
 *
 * `up+down` + `long` is deliberately not producible: on hardware, holding UP+DOWN past 1 s and releasing
 * does NOT blank the display. It used to be the mockup's only combo event.
 */
export interface SimulatedButtonEvent {
  button: SimulatedButton;
  kind: SimulatedButtonEventKind;
  timestamp: number;
}
