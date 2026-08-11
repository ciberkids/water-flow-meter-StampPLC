import { AccelTier, accelerationTier } from "./accel";

/**
 * `InteractionHandler::handleEditorRepeat` as a pure machine (Display_UI_Requirements §5.4).
 *
 * Why a machine and not a timer with logic inside it: the combo gestures went the same way for the same
 * reason. The firmware's version is a function of (levels, held duration, editor state) evaluated on every
 * loop pass, so a pure per-pass step can be tested to the millisecond, while the browser's contribution —
 * "something must call this while the button is merely held" — stays a single self-rescheduling timeout in
 * useSimulatedButtons.
 *
 * What it exists to fix: the simulator had only the button EVENT QUEUE, whose repeats start at 1500 ms and
 * belong to the navigation ring. So holding UP in an editor did nothing for a second and a half, and then
 * the ring's repeat arrived — dropped on an editor screen (`f-inc` names no target) and followed on a
 * setting screen (`f-prev`/`f-next` do), which is why a held UP froze the value in one place and paged the
 * menu in another. The device never had that gap: `handleEditorRepeat` runs BEFORE the queue drains, reads
 * the button levels directly, and takes UP/DOWN away from the queue entirely while an editor is open.
 */
export type RampButton = "up" | "down";

export interface EditorRampState {
  active: boolean;
  button: RampButton | null;
  /** When the last step was applied — or when the ramp armed, which starts the same clock. */
  lastStepMs: number;
  /** Whether a step has actually been applied, which is what makes the release swallow-worthy. */
  stepped: boolean;
}

export function initialEditorRampState(): EditorRampState {
  return { active: false, button: null, lastStepMs: 0, stepped: false };
}

export interface EditorRampInput {
  up: boolean;
  down: boolean;
  /** `editor.active && editor.setting` — whether an editor is open to receive the steps. */
  editorOwns: boolean;
  /** When each button's level went high, for the acceleration tier. */
  pressStartMs: Record<RampButton, number>;
}

export interface EditorRampResult {
  state: EditorRampState;
  /** The step this pass produced, if any. `multiplier` scales the setting's own `step`. */
  step: { button: RampButton; multiplier: number; heldMs: number } | null;
  /**
   * Buttons whose queued events must be discarded — `buttonInput.discardEvents(button)`.
   *
   * Emitted on every pass that steps, and once more when the ramp ends after having stepped: that last
   * one is what swallows the RELEASE short press, so a hold that stepped +25 does not also land a +1 on
   * the way up.
   */
  discard: RampButton[];
  /** How long until the next pass should run; null when the ramp is idle and needs no pass. */
  nextPassMs: number | null;
}

function endRamp(state: EditorRampState): EditorRampResult {
  return {
    state: initialEditorRampState(),
    step: null,
    // The firmware discards only when it actually stepped. Discarding unconditionally would eat the
    // short press of every ordinary tap, which is the one thing a tap must always deliver.
    discard: state.active && state.stepped && state.button ? [state.button] : [],
    nextPassMs: null
  };
}

/**
 * One loop pass.
 *
 * Returns the next state plus what the caller must do: apply a step, discard a button's queued events,
 * and when to come back. The three reasons to stand down are the firmware's, in its order — no editor,
 * neither-or-both buttons, and a change of button — and all three are re-tested every pass because all
 * three can change while the button stays down.
 */
export function tickEditorRamp(
  state: EditorRampState,
  input: EditorRampInput,
  nowMs: number
): EditorRampResult {
  if (!input.editorOwns) {
    return endRamp(state);
  }

  if (input.up === input.down) {
    // Neither, or both: neither is an adjustment. Both-held is also the display-off gesture, which the
    // combo machine owns, so stepping here would fight it.
    return endRamp(state);
  }

  const button: RampButton = input.up ? "up" : "down";
  if (!state.active || state.button !== button) {
    /**
     * ARMING is a pass of its own, and deliberately does not step.
     *
     * The short press already delivers the first ±1 on release; if arming also stepped, a tap would move
     * the value twice. Arming starts the clock instead, so the first ramped step lands one tier interval
     * (250 ms) after the button went down — which is also why a tap released before then behaves exactly
     * as it always did.
     */
    return {
      state: { active: true, button, lastStepMs: nowMs, stepped: false },
      step: null,
      discard: [],
      nextPassMs: accelerationTier(0).intervalMs
    };
  }

  const heldMs = Math.max(0, nowMs - input.pressStartMs[button]);
  const tier: AccelTier = accelerationTier(heldMs);
  const sinceLast = nowMs - state.lastStepMs;
  if (sinceLast < tier.intervalMs) {
    // Not yet. Come back for the remainder rather than polling: a tier promotion can only shorten the
    // wait, and it is re-read on the pass that follows.
    return { state, step: null, discard: [], nextPassMs: tier.intervalMs - sinceLast };
  }

  return {
    state: { active: true, button, lastStepMs: nowMs, stepped: true },
    step: { button, multiplier: tier.multiplier, heldMs },
    discard: [button],
    nextPassMs: tier.intervalMs
  };
}
