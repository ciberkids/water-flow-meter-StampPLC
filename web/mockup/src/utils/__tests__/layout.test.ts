
import { describe, expect, it } from "vitest";
import { computeLayout, DISPLAY_HEIGHT, DISPLAY_WIDTH } from "../layout";
import type { ScreenDefinition } from "../../types";

const baseScreen: ScreenDefinition = {
  id: "test-screen",
  name: "Test",
  elements: [
    { id: "origin", kind: "text", x: 0, y: 0, content: "Origin" },
    { id: "max-portrait", kind: "box", x: 135, y: 240, width: 10, height: 10, content: "" },
    { id: "max-landscape", kind: "box", x: 240, y: 135, width: 10, height: 10, content: "" },
    { id: "mid-landscape", kind: "text", x: 200, y: 50, content: "Mid" }
  ],
  flows: []
};

describe("computeLayout", () => {
  it("Landscape: maps X directly to pixels (1:1) and allows > 135", () => {
    const report = computeLayout(baseScreen, "landscape");
    const mid = report.elements.find((e) => e.element.id === "mid-landscape")!;

    // X=200 should result in Left=200 (minus alignment, text alignment default left=0 offset)
    // Actually default align is left.
    expect(mid.left).toBe(200);
    expect(report.bounds.width).toBe(240);
  });

  it("Landscape: clamps Y to 135 (height)", () => {
    const report = computeLayout(baseScreen, "landscape");
    const max = report.elements.find((e) => e.element.id === "max-portrait")!;
    // Y=240 should be clamped to 135
    // Top = Height - Y - ElementHeight
    // If Y is clamped to 135 (Height), Top = 135 - 135 - 10 = -10?
    // Wait, clampValue(y, 0, bounds.height). So Y=135.
    // unclampedTop = 135 - 135 - 10 = -10.
    // maxTop = 135 - 10 = 125.
    // top = clampValue(-10, 0, 125) = 0.
    // So element is at top edge.
    expect(max.top).toBe(0);
  });

  it("Portrait: clamps X to 135 (width)", () => {
    const report = computeLayout(baseScreen, "portrait");
    const mid = report.elements.find((e) => e.element.id === "mid-landscape")!;
    // X=200 should be clamped to 135.
    // Left = 135 - Align.
    // Element Width (Text "Mid") approx 18px.
    // maxLeft = 135 - 18 = 117.
    // Left = 117.
    // Actually normalizedX is clamped to 135.
    // unclampedLeft = 135.
    // Left = min(135, 117) = 117.
    expect(mid.left).toBeLessThanOrEqual(135);
  });

  it("treats (0,0) as the lower-left corner", () => {
    const report = computeLayout(baseScreen, "landscape");
    const origin = report.elements.find((item) => item.element.id === "origin");

    // Left = 0
    expect(origin?.left).toBe(0);

    // Top = Height - Y - H = 135 - 0 - 8 = 127
    expect(origin?.top).toBe(135 - 8);
  });

  it("renders higher Y values above lower Y values (visually)", () => {
    const screenWithTwoElements: ScreenDefinition = {
      ...baseScreen,
      elements: [
        { id: "low", kind: "text", x: 10, y: 10, content: "Low" },
        { id: "high", kind: "text", x: 10, y: 50, content: "High" }
      ]
    };
    const report = computeLayout(screenWithTwoElements, "portrait");
    const low = report.elements.find((e) => e.element.id === "low")!;
    const high = report.elements.find((e) => e.element.id === "high")!;

    // In DOM/CSS, lower 'top' value means higher on screen
    expect(high.top).toBeLessThan(low.top);
  });
});
