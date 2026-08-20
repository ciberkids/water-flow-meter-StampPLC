import { describe, expect, it } from "vitest";
import {
  clampDatasetToDisplay,
  clampElementGeometry,
  clampCoordinate,
  coordinateLimit
} from "../datasetClamp";
import { DISPLAY_HEIGHT, DISPLAY_WIDTH } from "../layout";
import type { ScreenDataset, ScreenElement } from "../../types";

describe("dataset clamp helpers", () => {
  /**
   * J8: one clamp rule, shared by the Design panel's inputs and every import path.
   *
   * The panel used to clamp a coordinate to the panel DIMENSION, so x = 240 was accepted on a 240-wide
   * display and the element rendered outside the visible area with zero width, while the import path
   * corrected the very same geometry to 200. These assertions pin the rule that settled it: the limit is
   * `bound - size`, so the element's far edge lands ON the boundary and stays visible.
   */
  describe("the coordinate limit accounts for the element's own size (J8)", () => {
    it("puts a 40-wide element's far edge exactly on the right boundary", () => {
      expect(coordinateLimit("x", 40)).toBe(200);
      expect(clampCoordinate(9999, "x", undefined, 40)).toBe(200);
      expect(200 + 40).toBe(240);
    });

    it("does the same on the y axis, against the landscape height", () => {
      expect(coordinateLimit("y", 20)).toBe(115);
      expect(clampCoordinate(9999, "y", undefined, 20)).toBe(115);
      expect(115 + 20).toBe(135);
    });

    it("pins an element larger than the display to 0 rather than a negative coordinate", () => {
      expect(coordinateLimit("x", 400)).toBe(0);
      expect(clampCoordinate(50, "x", undefined, 400)).toBe(0);
    });

    it("still accepts a coordinate the element fits at", () => {
      expect(clampCoordinate(100, "x", undefined, 40)).toBe(100);
      expect(clampCoordinate(0, "x", undefined, 240)).toBe(0);
    });

    it("agrees with the import path for the same geometry — the disagreement J8 recorded", () => {
      const element: ScreenElement = {
        id: "pushed-right",
        kind: "box",
        x: 220,
        y: 260,
        width: 80,
        height: 20,
        content: ""
      };

      const imported = clampElementGeometry(element);
      expect(imported.element.x).toBe(160);
      expect(imported.element.y).toBe(115);

      // What the panel's own path now produces for the same numbers.
      expect(clampCoordinate(220, "x", undefined, 80)).toBe(160);
      expect(clampCoordinate(260, "y", undefined, 20)).toBe(115);
    });

    it("keeps the size-blind behaviour when no extent is given, so unrelated callers are unchanged", () => {
      expect(clampCoordinate(240, "x")).toBe(240);
      expect(clampCoordinate(135, "y")).toBe(135);
    });
  });

  // The device is landscape-only (decision D3): 240 wide by 135 tall. These assertions
  // previously read 135 x 240 — the panel's native portrait dimensions — which made the
  // suite green while the clamp silently squashed 49 of the 375 real elements.
  it("clamps individual geometry fields to the landscape display and reports adjustments", () => {
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

    expect(result.element.width).toBe(240);
    expect(result.element.height).toBe(135);
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

    // Landscape: DISPLAY_HEIGHT is the long edge and so is the canvas WIDTH. Reading these
    // the other way round is what let the portrait default pass unnoticed.
    const offendingElement = clamped.screens[0].elements.find((el) => el.id === "x-bad");
    expect(offendingElement?.x).toBeLessThanOrEqual(DISPLAY_HEIGHT - (offendingElement?.width ?? 0));
    expect(offendingElement?.y).toBeLessThanOrEqual(DISPLAY_WIDTH - (offendingElement?.height ?? 0));
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

describe("the shipped dataset survives ingest untouched", () => {
  // The bug this guards against did not look like a bug: the two synthetic tests above were
  // green while the clamp rewrote 49 of the 375 real elements — every scrollbar from x=232
  // to 130 and every full-width divider from 240 to 135 — because DEFAULT_BOUNDS used the
  // panel's native portrait dimensions.
  //
  // Synthetic fixtures cannot catch that: they were written against the same wrong constants.
  // Only the real dataset can, so it is the fixture.
  it("clamps nothing in src/data/screens.json", async () => {
    const dataset = (await import("../../data/screens.json")) as unknown as {
      default: ScreenDataset;
    };
    const { corrections } = clampDatasetToDisplay(dataset.default ?? (dataset as unknown as ScreenDataset));
    expect(
      corrections.map((c) => `${c.screenId}/${c.elementId}`),
      "the shipped dataset must fit the display exactly; a correction here means either the " +
        "clamp bounds are wrong or a screen was authored off-canvas"
    ).toEqual([]);
  });
});

