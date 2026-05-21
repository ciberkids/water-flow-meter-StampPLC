import { DisplayOrientation, ScreenDefinition, ScreenElement } from "../types";

export const DISPLAY_WIDTH: number = 135;
export const DISPLAY_HEIGHT: number = 240;

const DEFAULT_METRICS: Record<ScreenElement["kind"], { charWidth: number; height: number; padding?: number }> = {
  text: { charWidth: 6, height: 8 },
  value: { charWidth: 7, height: 10 },
  badge: { charWidth: 6, height: 12, padding: 6 },
  box: { charWidth: 0, height: 0 },
  icon: { charWidth: 0, height: 10 },
  animation: { charWidth: 6, height: 18, padding: 6 },
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

    // In LTR/Cartesian: Left is just X - Alignment
    const unclampedLeft = normalizedX - alignOffset;
    const maxLeft = Math.max(bounds.width - boxWidth, 0);
    const left = clampValue(unclampedLeft, 0, maxLeft);

    // Invert Y axis: 0 is bottom, bounds.height is top.
    // CSS Top = Bounds Height - Y - Element Height
    const unclampedTop = bounds.height - normalizedY - boxHeight;
    const maxTop = Math.max(bounds.height - boxHeight, 0);
    const top = clampValue(unclampedTop, 0, maxTop);

    const roundedLeft = Math.round(left);
    const roundedTop = Math.round(top);
    const roundedWidth = Math.max(0, Math.round(boxWidth));
    const roundedHeight = Math.max(0, Math.round(boxHeight));

    const originalBox = {
      left: roundedLeft,
      top: roundedTop,
      width: roundedWidth,
      height: roundedHeight
    };

    const outOfBounds =
      originalBox.left < 0 ||
      originalBox.top < 0 ||
      originalBox.left + originalBox.width > bounds.width ||
      originalBox.top + originalBox.height > bounds.height;

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
