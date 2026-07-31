import { DisplayOrientation, ScreenDefinition, ScreenElement } from "../types";

export const DISPLAY_WIDTH: number = 135;
export const DISPLAY_HEIGHT: number = 240;

const DEFAULT_METRICS: Record<ScreenElement["kind"], { charWidth: number; height: number; padding?: number }> = {
  text: { charWidth: 6, height: 8 },
  value: { charWidth: 7, height: 10 },
  badge: { charWidth: 6, height: 12, padding: 6 },
  box: { charWidth: 0, height: 0 },
  icon: { charWidth: 0, height: 10 },
  scrollbar: { charWidth: 4, height: 60 }
};

const clampValue = (value: number, min: number, max: number) => Math.max(min, Math.min(max, value));

export interface LayoutBounds {
  width: number;
  height: number;
  orientation: DisplayOrientation;
}

export interface LayoutElement {
  element: ScreenElement;
  left: number;
  top: number;
  width: number;
  height: number;
  outOfBounds: boolean;
  originalLeft: number;
  originalTop: number;
  originalWidth: number;
  originalHeight: number;
}

export interface LayoutReport {
  bounds: LayoutBounds;
  elements: LayoutElement[];
  overflow: LayoutElement[];
}

export function estimateWidth(element: ScreenElement): number {
  if (element.width !== undefined) {
    return element.width;
  }

  const metrics = DEFAULT_METRICS[element.kind];
  if (!metrics) {
    return 0;
  }

  if (element.kind === "box" || element.kind === "icon") {
    return element.width ?? (element.height ?? metrics.height ?? 0);
  }

  const contentLength = element.content?.length ?? 0;
  const padding = metrics.padding ?? 0;
  return contentLength * metrics.charWidth + padding;
}

export function estimateHeight(element: ScreenElement): number {
  if (element.height !== undefined) {
    return element.height;
  }

  const metrics = DEFAULT_METRICS[element.kind];
  if (!metrics) {
    return 0;
  }

  if (element.kind === "icon") {
    return element.height ?? metrics.height;
  }

  if (element.kind === "box") {
    return element.height ?? 0;
  }

  return metrics.height;
}

function boundsForOrientation(orientation: DisplayOrientation): LayoutBounds {
  if (orientation === "landscape") {
    return { width: DISPLAY_HEIGHT, height: DISPLAY_WIDTH, orientation };
  }
  return { width: DISPLAY_WIDTH, height: DISPLAY_HEIGHT, orientation };
}

function clampBoxToBounds(
  bounds: LayoutBounds,
  box: { left: number; top: number; width: number; height: number }
) {
  let { left, top, width, height } = box;

  if (left < 0) {
    const delta = -left;
    left = 0;
    width = Math.max(0, width - delta);
  }

  if (top < 0) {
    const delta = -top;
    top = 0;
    height = Math.max(0, height - delta);
  }

  const overflowRight = left + width - bounds.width;
  if (overflowRight > 0) {
    width = Math.max(0, width - overflowRight);
  }

  const overflowBottom = top + height - bounds.height;
  if (overflowBottom > 0) {
    height = Math.max(0, height - overflowBottom);
  }

  return { left, top, width, height };
}

export function computeLayout(screen: ScreenDefinition, orientation: DisplayOrientation): LayoutReport {
  const bounds = boundsForOrientation(orientation);
  const elements: LayoutElement[] = [];
  const overflow: LayoutElement[] = [];

  for (const element of screen.elements) {
    const portraitWidth = estimateWidth(element);
    const portraitHeight = estimateHeight(element);

    // No scaling - 1:1 mapping
    let boxWidth = portraitWidth;
    let boxHeight = portraitHeight;

    const normalizedX = clampValue(element.x ?? 0, 0, bounds.width);
    const normalizedY = clampValue(element.y ?? 0, 0, bounds.height);

    const alignOffset =
      element.align === "center" ? boxWidth / 2 : element.align === "right" ? boxWidth : 0;

    const unclampedLeft = normalizedX - alignOffset;

    // Top-left origin, matching the device. This previously inverted the Y axis
    // ("CSS Top = Bounds Height - Y - Element Height"), while the firmware draws
    // with display.setCursor(x, element.y) against LovyanGFX's top-left origin —
    // so the mockup rendered every screen vertically MIRRORED. An element authored
    // at y=2 (hdr-title) appeared at the bottom of the preview and at the top of
    // the panel. That made the tool unusable as a fidelity check for exactly the
    // vertical-stacking defects it is meant to catch.
    const unclampedTop = normalizedY;

    // outOfBounds must be judged on the AUTHORED box, before clamping. It used to
    // be computed from the already-clamped left/top, which made positional overflow
    // undetectable: with left clamped to [0, width - boxWidth], the sum
    // left + width could only exceed the display when a single element was wider
    // than the whole panel. Elements pushed off the right edge by their x were
    // silently shifted back instead of being flagged.
    const authoredBox = {
      left: Math.round(unclampedLeft),
      top: Math.round(unclampedTop),
      width: Math.max(0, Math.round(boxWidth)),
      height: Math.max(0, Math.round(boxHeight))
    };

    const outOfBounds =
      authoredBox.left < 0 ||
      authoredBox.top < 0 ||
      authoredBox.left + authoredBox.width > bounds.width ||
      authoredBox.top + authoredBox.height > bounds.height;

    const originalBox = authoredBox;
    const clampedBox = clampBoxToBounds(bounds, originalBox);

    const layoutElement: LayoutElement = {
      element,
      left: clampedBox.left,
      top: clampedBox.top,
      width: clampedBox.width,
      height: clampedBox.height,
      outOfBounds,
      originalLeft: originalBox.left,
      originalTop: originalBox.top,
      originalWidth: originalBox.width,
      originalHeight: originalBox.height
    };

    elements.push(layoutElement);
    if (outOfBounds) {
      overflow.push(layoutElement);
    }
  }

  return { bounds, elements, overflow };
}
