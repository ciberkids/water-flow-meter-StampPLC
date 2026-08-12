/**
 * The device's two multi-button gestures, as a pure state machine.
 *
 * Mirrors InteractionHandler::handleDisplayOffCombo (interaction_handler.cpp:194-218) and
 * InteractionHandler::handleSelectorCombo (interaction_handler.cpp:304-334). Everything here
 * is a LEVEL test over the three button states plus an explicit `nowMs`, exactly as on the
 * device: the firmware re-reads isPressed() on every pass of the logic loop and owns no
 * timers, so the mockup does not need any either. Time enters as a parameter, which is also
 * what makes the boundaries testable to the millisecond.
 *
 * The names below keep the firmware's `k` prefix rather than the mockup's SCREAMING_CASE
 * (useSimulatedButtons.ts's LONG_PRESS_MS) so that a reader can grep the constant across both
 * trees and land on the header that defines it.
 */

/**
 * UP+DOWN released strictly inside this window is a display-off request, not a hold.
 *
 * interaction_handler.h:91 — `static constexpr uint32_t kDisplayOffComboMaxMs = 1000;`
 */
export const kDisplayOffMaxMs = 1000;

/**
 * UP+DOWN+ENTER held this long opens the firmware-drawn Select Menu page (§3.4.1).
 *
 * interaction_handler.h:93 — `static constexpr uint32_t kSelectorComboHoldMs = 3000;`
 */
export const kSelectorHoldMs = 3000;

/** The three physical buttons, as levels. `true` means down. */
export interface ComboButtons {
  readonly up: boolean;
  readonly down: boolean;
  readonly enter: boolean;
}

/**
 * The buttons whose ordinary single-button events the adapter must DROP.
 *
 * Same shape as ComboButtons, deliberately a different type: these are not levels, and a
 * consumed button is very often one that has just gone up.
 */
export interface ConsumedButtons {
  readonly up: boolean;
  readonly down: boolean;
  readonly enter: boolean;
}

/**
 * A gesture that completed on this pass.
 *
 * A union rather than `{ button, kind }` because the two gestures do not share a button set —
 * display-off is UP+DOWN with ENTER up, the selector is all three — so any product type would
 * make illegal pairs representable.
 *
 * `display-off` means BOTH halves of interaction_handler.cpp:213-214: the caller resets
 * navigation to the P0 global-status page (`setPage(UiPage::GlobalStatus)`) and then blanks the
 * display (`enterIdle`). Blanking alone is not the gesture — §3.1 wants the display to wake on
 * P0 rather than wherever the operator happened to be.
 *
 * `open-pack-selector` is InteractionResult::openPackSelector (interaction_handler.h:45): the
 * caller opens the Select Menu page.
 */
export type ComboEvent =
  | { readonly kind: "display-off"; readonly heldMs: number; readonly atMs: number }
  | { readonly kind: "open-pack-selector"; readonly heldMs: number; readonly atMs: number };

/**
 * The whole machine's memory. One field per firmware struct, plus the consumed latch.
 *
 * `displayOff` is ComboState (interaction_handler.h:153-156) and `selector` is
 * SelectorComboState (interaction_handler.h:147-151) — including its `fired` flag, which is
 * what stops the selector re-opening on every pass for as long as the buttons stay down.
 */
export interface ComboState {
  readonly displayOff: { readonly active: boolean; readonly startMs: number };
  readonly selector: { readonly active: boolean; readonly startMs: number; readonly fired: boolean };
  /**
   * Buttons still held that have taken part in a combo. Sticky until the button goes up,
   * because the event that has to be swallowed is usually the release itself.
   */
  readonly consumed: ConsumedButtons;
}

/**
 * Which gesture is armed right now, for a UI that wants to say so.
 *
 * A projection of ComboState, not extra state: `state.selector.active ? "selector" :
 * state.displayOff.active ? "display-off" : null`. The selector wins the tie for the same reason
 * interaction_handler.cpp:80-84 runs it first — though the two are in fact mutually exclusive,
 * because display-off requires ENTER to be up.
 */
export type ArmedCombo = "display-off" | "selector" | null;

/** That projection, as the one implementation of it — the adapter must not re-derive it. */
export function armedComboOf(state: ComboState): ArmedCombo {
  /**
   * ARMED means "something is still pending", so a fired selector is not armed any more.
   *
   * Without the `!fired` test the panel went on announcing "Select Menu opens at 3 s" for as long as
   * the three buttons stayed down — after the menu had already opened. The gesture worked perfectly and
   * its only feedback said it had not happened yet, which was reported as the gesture being broken:
   * "it behaves like all the buttons are pressed and nothing happens". Holding longer, which is what
   * that message invites, does nothing at all — the machine fires once per grab.
   */
  if (state.selector.active && !state.selector.fired) {
    return "selector";
  }
  return state.displayOff.active ? "display-off" : null;
}

export interface ComboUpdate {
  readonly state: ComboState;
  readonly events: readonly ComboEvent[];
  readonly consumed: ConsumedButtons;
}

export interface ComboTickResult {
  readonly state: ComboState;
  readonly events: readonly ComboEvent[];
}

const noneConsumed: ConsumedButtons = { up: false, down: false, enter: false };

export function initialComboState(): ComboState {
  return {
    displayOff: { active: false, startMs: 0 },
    selector: { active: false, startMs: 0, fired: false },
    consumed: noneConsumed
  };
}

/**
 * The buttons currently combo-consumed, for events the adapter raises on its own schedule.
 *
 * A hook's long-press and repeat timers fire between button changes, so they cannot read the
 * `consumed` of the last applyButtons() call — that answer is already stale by the time the
 * 1500 ms timer runs. This is the query for those moments.
 */
export function comboConsumed(state: ComboState): ConsumedButtons {
  return state.consumed;
}

/**
 * One pass of the machine. Both public entry points delegate here.
 *
 * The firmware has a single path too: handleSelectorCombo and handleDisplayOffCombo run on
 * every InteractionHandler::update, whether or not a button moved. Splitting them here into
 * "buttons changed" and "time advanced" variants that behaved differently would be a divergence
 * with no firmware counterpart.
 */
function step(state: ComboState, buttons: ComboButtons, nowMs: number): ComboUpdate {
  const events: ComboEvent[] = [];

  // Selector first. interaction_handler.cpp:80-84 orders it ahead of display-off to make the
  // precedence explicit rather than incidental; the two cannot both fire anyway, since
  // display-off requires ENTER to be up.
  let selector = state.selector;
  const allThree = buttons.up && buttons.down && buttons.enter;
  if (!allThree) {
    // interaction_handler.cpp:312-315: losing any of the three discards the whole attempt,
    // `fired` included — so a fresh grab of all three can open the selector again.
    selector = { active: false, startMs: 0, fired: false };
  } else {
    if (!selector.active) {
      // interaction_handler.cpp:317-321: stamped on the pass the THIRD button completed the
      // set, not on the first one down.
      selector = { active: true, startMs: nowMs, fired: false };
    }
    const heldMs = nowMs - selector.startMs;
    if (!selector.fired && heldMs >= kSelectorHoldMs) {
      // interaction_handler.cpp:328-333: at or past the threshold, WHILE HELD — not on release —
      // and latched so it cannot re-fire on the following passes.
      selector = { active: true, startMs: selector.startMs, fired: true };
      events.push({ kind: "open-pack-selector", heldMs, atMs: nowMs });
    }
  }

  let displayOff = state.displayOff;
  if (buttons.up && buttons.down && !buttons.enter) {
    // interaction_handler.cpp:201-207. A level test with no order and no deferral: the arm
    // happens on the very pass the condition holds, and the start time is kept across
    // subsequent passes rather than restamped.
    if (!displayOff.active) {
      displayOff = { active: true, startMs: nowMs };
    }
  } else if (displayOff.active) {
    // interaction_handler.cpp:209-217. ENTER joining makes the predicate false too, which is why
    // the fire is guarded by `!enter` as well as by the window: a hold that grew into the
    // three-button gesture must not blank the display on its way there.
    const heldMs = nowMs - displayOff.startMs;
    displayOff = { active: false, startMs: 0 };
    // interaction_handler.cpp:210 — strictly less than. Held for the full window or longer, the
    // release produces nothing at all.
    if (heldMs < kDisplayOffMaxMs && !buttons.enter) {
      events.push({ kind: "display-off", heldMs, atMs: nowMs });
    }
  }

  // ── Event swallowing ────────────────────────────────────────────────────────────────────────
  //
  // The firmware calls buttonInput.clearEvents() while the selector combo is armed
  // (interaction_handler.cpp:326) and at the instant display-off fires
  // (interaction_handler.cpp:215), for the reason its own comment gives at
  // interaction_handler.cpp:323-325: otherwise the release of the three buttons "would also
  // dispatch UP-short, DOWN-short and ENTER-short against whatever screen is showing, so
  // recovering from an unusable pack would page and descend on the way out".
  //
  // A per-button latch that survives until the button goes up implements that intent on the
  // passes where the firmware's two clearEvents() calls do not reach:
  //   * UP+DOWN held past 1500 ms — the armed branch returns early without clearing
  //     (interaction_handler.cpp:201-207), so ButtonInputManager's long press and its 250 ms
  //     repeats (button_input.cpp:48-60) reach the flow dispatcher and page the screen;
  //   * a staggered UP/DOWN release — the fire pass clears the FIRST release's event, and the
  //     second button's release, one pass later, finds the combo already cleared;
  //   * ENTER released under 1500 ms during a three-button hold — `all` goes false, so
  //     interaction_handler.cpp:312-315 returns without clearing and ENTER-short descends.
  // None of the three is pinned by test/host/interaction_test.cpp: recoveryGestureTests holds
  // for 3100 ms, so every release is silent for an unrelated reason (button_input.cpp:33-40
  // suppresses the release event once the long press has been sent), and tapUpDown()
  // (interaction_test.cpp:312-320) releases both buttons on the same pass.
  const participating: ConsumedButtons = {
    up: displayOff.active || selector.active,
    down: displayOff.active || selector.active,
    enter: selector.active
  };
  const consumed: ConsumedButtons = {
    up: state.consumed.up || participating.up,
    down: state.consumed.down || participating.down,
    enter: state.consumed.enter || participating.enter
  };

  return {
    state: {
      displayOff,
      selector,
      // Reported for this pass, then dropped: the latch exists to swallow the release, so it has
      // to outlive the press by exactly one call and no more.
      consumed: {
        up: consumed.up && buttons.up,
        down: consumed.down && buttons.down,
        enter: consumed.enter && buttons.enter
      }
    },
    events,
    consumed
  };
}

/**
 * Feed a button-level change. Call this on every press and release, synchronously.
 *
 * Synchronously is the contract, not an implementation note. useSimulatedButtons.ts:178 defers
 * the arming check by 50 ms so a second button can "arrive simultaneously"; a fast tap — a
 * pointerdown and pointerup in the same frame, or a click — releases before that timer runs, so
 * the gesture can never fire. Here, pressing both buttons and releasing them at the same
 * `nowMs` fires display-off, because 0 < kDisplayOffMaxMs.
 */
export function applyButtons(state: ComboState, buttons: ComboButtons, nowMs: number): ComboUpdate {
  return step(state, buttons, nowMs);
}

/**
 * Advance time with the button levels unchanged.
 *
 * This is the only way the selector can fire: it completes WHILE HELD, so no button change marks
 * the moment (interaction_handler.cpp:328-333). Pass the same levels that were last applied —
 * a tick is the firmware's "another pass of the loop with nothing new on the pins", and it does
 * not report `consumed` because no single-button event can be pending when nothing moved. Use
 * comboConsumed() for timer-driven events.
 */
export function tickCombos(state: ComboState, buttons: ComboButtons, nowMs: number): ComboTickResult {
  const { state: next, events } = step(state, buttons, nowMs);
  return { state: next, events };
}
