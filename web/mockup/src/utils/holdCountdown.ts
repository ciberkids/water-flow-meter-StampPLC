/**
 * `InteractionHandler::handleHoldCountdown`'s three decisions, as pure predicates.
 *
 * The countdown itself is a timer and lives in App.tsx, as it must — but every decision the firmware
 * makes about it is a function of the button LEVELS and the flow's declared duration, and those are
 * exactly the parts that were wrong here. So they move out, the way `comboGestures.ts` and
 * `editorRamp.ts` moved out, and for the same reason: nothing in App.tsx can be tested at all
 * (`docs/active_work/open_decisions.md` — "no `@testing-library`"), so a rule that stays inline is a
 * rule nobody can pin.
 *
 * What was wrong, in the firmware's own terms:
 *
 *   - ARMING. interaction_handler.cpp:493 arms only `if (enterHeld && !otherHeld)`. The simulator armed
 *     on the ENTER press with no level test, so grabbing UP+DOWN and then ENTER on `RESET TOTALS?` —
 *     the §3.4.1 recovery gesture, whose whole point is to be safe from anywhere — ALSO reset the
 *     totals, at the same 3000 ms the Select Menu opens.
 *   - SWALLOWING. update()'s `else { buttonInput.clearEvents(); }` (interaction_handler.cpp:110-114)
 *     drops every queued single-button event on every pass while a countdown owns ENTER, which is what
 *     makes §4.3 note 2's "UP/DOWN have no effect during a countdown" true. The simulator dispatched
 *     them, so UP on a confirm screen paged away mid-countdown while the reset still landed behind it.
 *   - VISIBILITY. The panel gets a moving number only when the guard is LONGER than the gesture
 *     boundary (interaction_handler.cpp:523, `holdCountdown_.durationMs > kGestureLongPressMs`).
 *     At or below it the guard is a plain long press, `countdownSeconds` is never written, and
 *     `countdown.value` renders `0 s` for the whole hold.
 */

/**
 * The gesture boundary: at or below this a duration is a long press, above it a countdown.
 *
 * interaction_handler.h:89 — `static constexpr uint32_t kGestureLongPressMs = 1500;`, whose comment
 * quotes the requirement: "durations longer than that are always countdowns shown on screen, never
 * gesture thresholds".
 */
export const kGestureLongPressMs = 1500;

/** The three physical buttons as levels, matching `comboGestures.ts`'s `ComboButtons`. */
export interface HoldButtons {
  readonly up: boolean;
  readonly down: boolean;
  readonly enter: boolean;
}

/**
 * Whether an ENTER press may arm the screen's hold countdown.
 *
 * `!otherHeld` is the load-bearing half. Note what it does NOT say: once armed, UP or DOWN joining
 * does not abort the countdown (interaction_handler.cpp:503-511 aborts on ENTER release and on
 * nothing else), so this is a test at the moment of arming only. That asymmetry is the firmware's,
 * and it means the device can genuinely run a countdown and the three-button recovery gesture at the
 * same time when ENTER goes down first — see the report accompanying this file.
 */
export function holdCountdownArms(buttons: HoldButtons): boolean {
  return buttons.enter && !buttons.up && !buttons.down;
}

/** Whether this duration is drawn as a countdown at all, or is merely a long press. */
export function holdCountdownIsVisible(durationMs: number): boolean {
  return durationMs > kGestureLongPressMs;
}

/**
 * What `countdown.value` renders — `ui_bindings.cpp:777-780`, `"%u s"` of `context.countdownSeconds`.
 *
 * Unconditional on the device: the binding prints the number whether or not a countdown is running, so
 * a confirm screen at rest shows `0 s` and a 1.5 s guard shows `0 s` for its whole hold. The mockup
 * used to fall back to the sample string `3` at rest and animate the 1.5 s guards, which made the two
 * screens that show a frozen zero on hardware look like the two that count down.
 *
 * Rounded UP while running, matching `secondsRemaining`'s ceiling division
 * (interaction_handler.cpp:61-64), so a 3 s hold opens at "3 s" rather than at "2 s".
 */
export function holdCountdownText(
  countdown: { readonly remainingMs: number; readonly totalMs: number } | null
): string {
  if (!countdown || !holdCountdownIsVisible(countdown.totalMs)) {
    return "0 s";
  }
  const remaining = Math.max(0, countdown.remainingMs);
  return `${Math.ceil(remaining / 1000)} s`;
}
