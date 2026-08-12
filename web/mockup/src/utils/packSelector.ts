/**
 * The firmware-drawn Select Menu, as data (Loadable_UI_Menu_Packs §3.4).
 *
 * This page is the one thing in the UI the dataset cannot describe: `UiRenderer` short-circuits to
 * `drawPackSelector` before every table-driven path, which is the requirement and not an optimisation —
 * §3.4.1 says the gesture must work "even if the active pack draws nothing at all", and that is only
 * true if the firmware owns this page. So the simulator has to own it too, and everything it needs to
 * draw and drive it lives here rather than being spread between the viewport and the app.
 *
 * Every number and string is the firmware's, and a unit test reads them back out of
 * `ui_renderer.cpp` and `ui_pack_selector.{h,cpp}` rather than trusting this copy. That is the same
 * treatment the acceleration tiers get, for the same reason: two languages, one contract, and nothing
 * but care holding them together.
 */

/** Geometry and copy, read off `UiRenderer::drawPackSelector`. */
export const packSelectorLayout = {
  title: { x: 4, y: 4 },
  /** Row `i` sits at `top + i * pitch`. */
  rows: { top: 20, pitch: 12 },
  /** The cursor glyph and the label are two separate draws at two x positions. */
  cursorX: 4,
  labelX: 16,
  /** The running menu's marker, hard against the right edge. */
  activeMarkerX: 228,
  footer: { x: 4, y: 116 },
  footerText: "UP/DN choose  ENTER select",
  truncatedText: "...more on card, not shown",
  unavailableText: "selector unavailable",
  /** `PackSelector::kMaxEntries` — chosen against the panel, not the filesystem. */
  maxEntries: 8
} as const;

/** Index 0 is always the built-in default, never supplied by the card. */
export const kBuiltInIndex = 0;

/**
 * `PackSelector::moveCursor`, which WRAPS.
 *
 * Worth stating plainly because the obvious guess is wrong and I made it: this looks like a short list
 * where clamping would be kinder, and the firmware instead does modular arithmetic —
 * `next = cursor + (delta % span)`, then folded into range. So DOWN from the last entry lands on the
 * built-in default. A simulator that clamped would quietly disagree with the device at exactly the
 * moment an operator is trying to get back to a working UI.
 */
export function movePackCursor(cursor: number, delta: number, entryCount: number): number {
  if (entryCount === 0) {
    return 0;
  }
  let next = cursor + (delta % entryCount);
  if (next < 0) next += entryCount;
  if (next >= entryCount) next -= entryCount;
  return next;
}

/** What committing the cursor does — `PackSelector::commitAction`. */
export type PackCommit = "nothing" | "write-pointer" | "delete-pointer";

/**
 * `PackSelector::commitAction`.
 *
 * Selecting the pack that is already running does NOTHING, deliberately: writing the same pointer and
 * rebooting into an identical UI "would look like the device ignoring the press". Selecting the
 * built-in default deletes the pointer rather than writing one.
 */
export function packCommitAction(cursor: number, activeIndex: number): PackCommit {
  if (cursor === activeIndex) {
    return "nothing";
  }
  return cursor === kBuiltInIndex ? "delete-pointer" : "write-pointer";
}
