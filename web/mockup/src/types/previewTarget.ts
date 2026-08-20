/**
 * The screen a press just navigated to, held briefly so the Screens list can ring it.
 *
 * WAS `transitionPreview`, carrying an effect, an action label, a trigger label and a full computed
 * layout for an overlay that faded a miniature of the incoming screen over the emulated panel. That
 * overlay is OFF by decision — the firmware clears and repaints, so previewing a transition the device
 * cannot produce made the simulator less faithful — and `App.tsx` had been passing
 * `pendingTransition={undefined}` into the viewport ever since (J7).
 *
 * What survived the decision is the part outside the panel: `previewId` reaches `ScreenSelector`, which
 * puts `.preview-target` — a cyan ring — on the row for 1500 ms. That is a workspace affordance, not a
 * device simulation, and it is the only reader. So the state is narrowed to the two fields that reader
 * needs, and named for what it is.
 */
export interface PreviewTargetState {
  /** The screen to ring in the list. */
  screenId: string;
  /** `Date.now() + 1500`; the effect that clears it lives beside the state in `App.tsx`. */
  expiresAt: number;
}
