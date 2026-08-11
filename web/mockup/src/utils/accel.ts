/**
 * §5.4's acceleration ramp, mirroring `ui/core/ui_accel.h`.
 *
 * The firmware implements this and the simulator did not, which is the whole bug: holding UP or DOWN
 * on an editor stepped nothing, so the operator held longer and the repeat fell through to the ring's
 * paging flow — leaving them on a neighbouring setting instead of a changed value. On the device the
 * editor OWNS those two buttons while it is open (`handleEditorRepeat`, which also discards the queued
 * release so it cannot add an extra step).
 *
 * The numbers are the requirement's, and they are duplicated across a language boundary rather than
 * derived — so a unit test reads the C++ header and compares, the same way the range-hint threshold is
 * kept honest.
 */
export interface AccelTier {
  /** Multiplier applied to the setting's own `step`. */
  multiplier: number;
  /** How long to wait before the next step, in ms. */
  intervalMs: number;
}

export const kAccelTier2Ms = 700;
export const kAccelTier3Ms = 1500;

export function accelerationTier(heldMs: number): AccelTier {
  if (heldMs < kAccelTier2Ms) {
    return { multiplier: 1, intervalMs: 250 };
  }
  if (heldMs < kAccelTier3Ms) {
    return { multiplier: 5, intervalMs: 150 };
  }
  return { multiplier: 25, intervalMs: 150 };
}
