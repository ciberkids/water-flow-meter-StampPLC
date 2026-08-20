import { ScreenDataset, ScreenElement } from "../types";
import { DISPLAY_HEIGHT, DISPLAY_WIDTH } from "./layout";

export type DisplayBounds = {
  width: number;
  height: number;
};

/**
 * Landscape, because landscape is the only orientation the device supports (decision D3).
 *
 * DISPLAY_WIDTH and DISPLAY_HEIGHT are the ST7789V2 panel's NATIVE portrait dimensions —
 * 135 x 240 — and `boundsForOrientation` in layout.ts swaps them for the landscape the
 * firmware actually drives. This module used the unswapped pair, so its default bounds were
 * a portrait 135 x 240 canvas.
 *
 * Measured against the real dataset, that clamped **49 of 375 elements**: every scrollbar
 * from x=232 to 130, and every full-width divider from 240 to 135. It was survivable only
 * because the exporter re-reads the on-disk file rather than the clamped in-memory one — so
 * fixing the stale-export trap (D4) without fixing this first would have turned it into a
 * corrupt-export trap that ships portrait-squashed geometry to the firmware.
 */
const DEFAULT_BOUNDS: DisplayBounds = {
  width: DISPLAY_HEIGHT,
  height: DISPLAY_WIDTH
};

export type ClampAdjustmentField = "x" | "y" | "width" | "height";

export type ClampAdjustment = {
  field: ClampAdjustmentField;
  from: number;
  to: number;
};

export type ClampCorrection = {
  screenId: string;
  elementId: string;
  adjustments: ClampAdjustment[];
};

const clampValue = (value: number, min: number, max: number) => Math.max(min, Math.min(max, value));

export const clampWidth = (value: number, bounds: DisplayBounds = DEFAULT_BOUNDS): number => {
  if (Number.isNaN(value)) {
    return 0;
  }
  return clampValue(value, 0, bounds.width);
};

export const clampHeight = (value: number, bounds: DisplayBounds = DEFAULT_BOUNDS): number => {
  if (Number.isNaN(value)) {
    return 0;
  }
  return clampValue(value, 0, bounds.height);
};

/**
 * The upper bound for a coordinate: the display, LESS the element's own size along that axis.
 *
 * `extent` is why this exists (J8). Without it the limit was the panel dimension itself, so x = 240 was
 * "legal" on a 240-wide display and the element sat entirely outside the visible area, rendering with
 * `clientWidth` 0. `clampElementGeometry` below — the path every IMPORT takes — has always used
 * `bound - size`, so the same geometry was corrected one way on import and accepted the other way when
 * typed into the Design panel. An element you cannot see is not a placement, so the size-aware rule wins
 * and both paths now share this helper.
 *
 * `Math.max(0, …)` keeps the bound sane for an element LARGER than the display: the coordinate pins to 0
 * and the overflow shows up as width/height, which is what the clamp buttons are for.
 */
export const coordinateLimit = (
  axis: "x" | "y",
  extent: number,
  bounds: DisplayBounds = DEFAULT_BOUNDS
): number => {
  const limit = axis === "x" ? bounds.width : bounds.height;
  return Math.max(0, limit - (Number.isFinite(extent) ? Math.max(0, extent) : 0));
};

export const clampCoordinate = (
  value: number,
  axis: "x" | "y",
  bounds: DisplayBounds = DEFAULT_BOUNDS,
  extent = 0
): number => {
  if (Number.isNaN(value)) {
    return 0;
  }
  return clampValue(value, 0, coordinateLimit(axis, extent, bounds));
};

export const clampElementGeometry = (
  element: ScreenElement,
  bounds: DisplayBounds = DEFAULT_BOUNDS
): { element: ScreenElement; adjustments: ClampAdjustment[] } => {
  const adjustments: ClampAdjustment[] = [];
  const next: Partial<ScreenElement> = {};

  const recordChange = (
    field: ClampAdjustmentField,
    previousValue: number | undefined,
    nextValue: number
  ) => {
    if (previousValue === undefined) {
      return;
    }
    if (previousValue !== nextValue) {
      adjustments.push({ field, from: previousValue, to: nextValue });
      next[field] = nextValue;
    }
  };

  const originalWidth = element.width;
  const originalHeight = element.height;

  const clampedWidth =
    originalWidth !== undefined ? clampWidth(originalWidth, bounds) : undefined;
  if (originalWidth !== undefined && clampedWidth !== originalWidth) {
    adjustments.push({ field: "width", from: originalWidth, to: clampedWidth! });
    next.width = clampedWidth;
  }

  const clampedHeight =
    originalHeight !== undefined ? clampHeight(originalHeight, bounds) : undefined;
  if (originalHeight !== undefined && clampedHeight !== originalHeight) {
    adjustments.push({ field: "height", from: originalHeight, to: clampedHeight! });
    next.height = clampedHeight;
  }

  const widthForRange = clampedWidth ?? originalWidth ?? 0;
  const heightForRange = clampedHeight ?? originalHeight ?? 0;

  // One rule, one home: the same helper the Design panel's inputs use (J8). These four lines used to
  // spell out `bounds.width - widthForRange` themselves, which is how the panel came to disagree with
  // the importer without anything failing.
  const rawX = element.x ?? 0;
  const clampedX = clampValue(rawX, 0, coordinateLimit("x", widthForRange, bounds));
  recordChange("x", element.x, clampedX);

  const rawY = element.y ?? 0;
  const clampedY = clampValue(rawY, 0, coordinateLimit("y", heightForRange, bounds));
  recordChange("y", element.y, clampedY);

  if (adjustments.length === 0) {
    return { element, adjustments };
  }
  return { element: { ...element, ...next }, adjustments };
};

export const clampDatasetToDisplay = (
  dataset: ScreenDataset,
  bounds: DisplayBounds = DEFAULT_BOUNDS
): { dataset: ScreenDataset; corrections: ClampCorrection[] } => {
  const corrections: ClampCorrection[] = [];
  let changed = false;

  const screens = dataset.screens.map((screen) => {
    let screenChanged = false;
    const elements = screen.elements.map((element) => {
      const { element: nextElement, adjustments } = clampElementGeometry(element, bounds);
      if (adjustments.length > 0) {
        screenChanged = true;
        corrections.push({
          screenId: screen.id,
          elementId: element.id,
          adjustments
        });
        return nextElement;
      }
      return element;
    });
    if (screenChanged) {
      changed = true;
      return { ...screen, elements };
    }
    return screen;
  });

  if (!changed) {
    return { dataset, corrections };
  }
  return { dataset: { ...dataset, screens }, corrections };
};
