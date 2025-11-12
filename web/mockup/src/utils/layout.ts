import { DisplayOrientation, ScreenDefinition, ScreenElement } from "../types";

export const DISPLAY_WIDTH = 135;
export const DISPLAY_HEIGHT = 240;

const DEFAULT_METRICS: Record<ScreenElement["kind"], { charWidth: number; height: number; padding?: number }> = {
  text: { charWidth: 6, height: 8 },
  value: { charWidth: 7, height: 10 },
  badge: { charWidth: 6, height: 12, padding: 6 },
  box: { charWidth: 0, height: 0 },
  icon: { charWidth: 0, height: 10 },
  animation: { charWidth: 6, height: 18, padding: 6 },
  scrollbar: { charWidth: 4, height: 60 }
};

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

export function computeLayout(screen: ScreenDefinition, orientation: DisplayOrientation): LayoutReport {
  const bounds = boundsForOrientation(orientation);
  const elements: LayoutElement[] = [];
  const overflow: LayoutElement[] = [];

  for (const element of screen.elements) {
    const portraitWidth = estimateWidth(element);
    const portraitHeight = estimateHeight(element);

    let left = element.x;
    let top = element.y;

    // Align before rotation to reflect intended anchor point.
    const align = element.align ?? "left";
    if (align === "center") {
      left = element.x - portraitWidth / 2;
    } else if (align === "right") {
      left = element.x - portraitWidth;
    }

    let boxWidth = portraitWidth;
    let boxHeight = portraitHeight;

    if (orientation === "landscape") {
      // Rotate 90° clockwise so the StampPLC portrait data maps to landscape mounts.
      const rotatedLeft = top;
      const rotatedTop = DISPLAY_WIDTH - (left + portraitWidth);
      left = rotatedLeft;
      top = rotatedTop;
      boxWidth = portraitHeight;
      boxHeight = portraitWidth;
    }

    const roundedLeft = Math.round(left);
    const roundedTop = Math.round(top);
    const roundedWidth = Math.max(0, Math.round(boxWidth));
    const roundedHeight = Math.max(0, Math.round(boxHeight));

    const outOfBounds =
      roundedLeft < 0 ||
      roundedTop < 0 ||
      roundedLeft + roundedWidth > bounds.width ||
      roundedTop + roundedHeight > bounds.height;

    const layoutElement: LayoutElement = {
      element,
      left: roundedLeft,
      top: roundedTop,
      width: roundedWidth,
      height: roundedHeight,
      outOfBounds
    };

    elements.push(layoutElement);
    if (outOfBounds) {
      overflow.push(layoutElement);
    }
  }

  return { bounds, elements, overflow };
}
