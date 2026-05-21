import { describe, expect, it } from "vitest";
import {
  clampDatasetToDisplay,
  clampElementGeometry,
  clampCoordinate
} from "../datasetClamp";
import { DISPLAY_HEIGHT, DISPLAY_WIDTH } from "../layout";
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

    expect(result.element.width).toBe(135);
    expect(result.element.height).toBe(240);
    expect(result.element.x).toBe(0);
    expect(result.element.y).toBe(0);

    const adjustmentFields = result.adjustments.map((adj) => adj.field).sort();
    expect(adjustmentFields).toEqual(["height", "width", "x", "y"]);
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
            { id: "x-bad", kind: "box", x: 400, y: 20, width: 80, height: 10 }
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
    expect(offendingElement?.x).toBeLessThanOrEqual(DISPLAY_WIDTH - (offendingElement?.width ?? 0));
    expect(offendingElement?.y).toBeLessThanOrEqual(DISPLAY_HEIGHT - (offendingElement?.height ?? 0));
  });

  it("accepts alternate bounds to match landscape editing", () => {
    const landscapeBounds = { width: DISPLAY_HEIGHT, height: DISPLAY_WIDTH };
    const element: ScreenElement = {
      id: "landscape-placement",
      kind: "text",
      x: 240,
      y: 135,
      content: "ok in landscape"
    };

    const result = clampElementGeometry(element, landscapeBounds);

    expect(result.adjustments).toHaveLength(0);
    expect(result.element.x).toBe(240);
    expect(result.element.y).toBe(135);

    const { dataset: clampedDataset, corrections } = clampDatasetToDisplay(
      {
        screens: [
          {
            id: "screen-landscape",
            name: "Landscape",
            description: "",
            elements: [element],
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
      },
      landscapeBounds
    );

    expect(corrections).toHaveLength(0);
    expect(clampedDataset.screens[0].elements[0].x).toBe(240);
    expect(clampedDataset.screens[0].elements[0].y).toBe(135);
  });
});
