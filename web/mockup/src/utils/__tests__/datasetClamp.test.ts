import { describe, expect, it } from "vitest";
import {
  clampDatasetToDisplay,
  clampElementGeometry,
  clampCoordinate,
  clampSize
} from "../datasetClamp";
import type { ScreenDataset, ScreenElement } from "../../types";

describe("dataset clamp helpers", () => {
  it("clamps individual geometry fields and reports adjustments", () => {
    const element: ScreenElement = {
      id: "overflow-box",
      kind: "box",
      x: 200,
      y: -5,
      width: 280,
      height: 260,
      content: ""
    };

    const result = clampElementGeometry(element);

    expect(result.element.x).toBe(clampCoordinate(200, "x"));
    expect(result.element.y).toBe(clampCoordinate(-5, "y"));
    expect(result.element.width).toBe(clampSize(280));
    expect(result.element.height).toBe(clampSize(260));

    expect(result.adjustments).toEqual([
      { field: "x", from: 200, to: clampCoordinate(200, "x") },
      { field: "y", from: -5, to: clampCoordinate(-5, "y") },
      { field: "width", from: 280, to: clampSize(280) },
      { field: "height", from: 260, to: clampSize(260) }
    ]);
  });

  it("clamps datasets and returns correction summaries", () => {
    const dataset: ScreenDataset = {
      screens: [
        {
          id: "screen-a",
          name: "Screen A",
          description: "",
          elements: [
            { id: "x-ok", kind: "text", x: 10, y: 10, content: "OK" },
            { id: "x-bad", kind: "box", x: 180, y: 20, width: 80, height: 10 }
          ],
          flows: []
        }
      ],
      theme: {
        name: "test",
        colors: {
          displayBackground: "#000",
          textPrimary: "#fff",
          textMuted: "#ccc",
          textStrong: "#fff",
          value: "#fff",
          badgeBackground: "#000",
          badgeBorder: "#fff",
          icon: "#fff",
          legend: "#fff",
          gridMinor: "#111",
          gridMajor: "#222"
        },
        typography: { base: 8, value: 10, badge: 8 },
        animation: { easing: "linear" }
      }
    };

    const { dataset: clamped, corrections } = clampDatasetToDisplay(dataset);

    expect(corrections).toHaveLength(1);
    expect(corrections[0]).toMatchObject({
      screenId: "screen-a",
      elementId: "x-bad"
    });

    const offendingElement = clamped.screens[0].elements.find((el) => el.id === "x-bad");
    expect(offendingElement?.x).toBeLessThanOrEqual(135);
    expect(offendingElement?.width).toBeLessThanOrEqual(240);
  });
});
