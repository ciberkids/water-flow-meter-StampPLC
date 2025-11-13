import { ScreenDataset, ScreenElement } from "../types";
import { DISPLAY_HEIGHT, DISPLAY_WIDTH } from "./layout";

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

const MAX_COORD_X = DISPLAY_WIDTH;
const MAX_COORD_Y = DISPLAY_HEIGHT;
const MAX_DIMENSION = DISPLAY_HEIGHT;

export const clampCoordinate = (value: number, axis: "x" | "y"): number => {
  const max = axis === "x" ? MAX_COORD_X : MAX_COORD_Y;
  if (Number.isNaN(value)) {
    return 0;
  }
  return Math.max(0, Math.min(max, value));
};

export const clampSize = (value: number): number => {
  if (Number.isNaN(value)) {
    return 0;
  }
  return Math.max(0, Math.min(MAX_DIMENSION, value));
};

export const clampElementGeometry = (
  element: ScreenElement
): { element: ScreenElement; adjustments: ClampAdjustment[] } => {
  const adjustments: ClampAdjustment[] = [];
  const next: Partial<ScreenElement> = {};

  const handleChange = (
    field: ClampAdjustmentField,
    clampedValue: number,
    currentValue: number | undefined
  ) => {
    if (currentValue === undefined) {
      return;
    }
    if (currentValue !== clampedValue) {
      adjustments.push({ field, from: currentValue, to: clampedValue });
      next[field] = clampedValue;
    }
  };

  handleChange("x", clampCoordinate(element.x ?? 0, "x"), element.x);
  handleChange("y", clampCoordinate(element.y ?? 0, "y"), element.y);

  if (element.width !== undefined) {
    handleChange("width", clampSize(element.width), element.width);
  }
  if (element.height !== undefined) {
    handleChange("height", clampSize(element.height), element.height);
  }

  if (adjustments.length === 0) {
    return { element, adjustments };
  }
  return { element: { ...element, ...next }, adjustments };
};

export const clampDatasetToDisplay = (
  dataset: ScreenDataset
): { dataset: ScreenDataset; corrections: ClampCorrection[] } => {
  const corrections: ClampCorrection[] = [];
  let changed = false;

  const screens = dataset.screens.map((screen) => {
    let screenChanged = false;
    const elements = screen.elements.map((element) => {
      const { element: nextElement, adjustments } = clampElementGeometry(element);
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
