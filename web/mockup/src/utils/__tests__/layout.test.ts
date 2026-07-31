import { describe, expect, it } from "vitest";
import { computeLayout, DISPLAY_HEIGHT, DISPLAY_WIDTH } from "../layout";
import type { ScreenDefinition } from "../../types";

/**
 * These tests pin the mockup to the DEVICE's coordinate convention.
 *
 * The previous version asserted a bottom-left origin ("treats (0,0) as the
 * lower-left corner", "renders higher Y values above lower Y values"). The firmware
 * draws with `display.setCursor(x, element.y)` against LovyanGFX's **top-left**
 * origin, so that convention rendered every screen vertically mirrored relative to
 * the hardware — and the tests locked the mirroring in place.
 *
 * The dataset is unambiguously authored top-down: `hdr-title` sits at y=2 and
 * `footer-hint` at y=124 on a 135-tall panel. Top-left is correct.
 */
const baseScreen: ScreenDefinition = {
  id: "test-screen",
  name: "Test",
  elements: [
    { id: "origin", kind: "text", x: 0, y: 0, content: "Origin" },
    { id: "header", kind: "text", x: 2, y: 2, content: "Header" },
    { id: "footer", kind: "text", x: 2, y: 124, content: "Footer" },
    { id: "mid-landscape", kind: "text", x: 200, y: 50, content: "Mid" }
  ],
  flows: []
};

const byId = (report: ReturnType<typeof computeLayout>, id: string) =>
  report.elements.find((item) => item.element.id === id)!;

describe("computeLayout", () => {
  it("maps X directly to pixels and allows the full landscape width", () => {
    const report = computeLayout(baseScreen, "landscape");
    expect(byId(report, "mid-landscape").left).toBe(200);
    expect(report.bounds.width).toBe(240);
    expect(report.bounds.height).toBe(135);
  });

  it("treats (0,0) as the TOP-left corner, as the device does", () => {
    const report = computeLayout(baseScreen, "landscape");
    const origin = byId(report, "origin");
    expect(origin.left).toBe(0);
    expect(origin.top).toBe(0);
  });

  it("places a low Y near the top and a high Y near the bottom", () => {
    const report = computeLayout(baseScreen, "landscape");
    const header = byId(report, "header");
    const footer = byId(report, "footer");

    // A header authored at y=2 must render at the top of the preview, and a footer
    // at y=124 near the bottom. The inverted implementation produced exactly the
    // reverse: header top=125, footer top=3.
    expect(header.top).toBe(2);
    expect(footer.top).toBe(124);
    expect(header.top).toBeLessThan(footer.top);
  });

  it("swaps the bounds for portrait", () => {
    const report = computeLayout(baseScreen, "portrait");
    expect(report.bounds.width).toBe(DISPLAY_WIDTH);
    expect(report.bounds.height).toBe(DISPLAY_HEIGHT);
  });

  it("flags horizontal overflow caused by position, not just by size", () => {
    // The regression this guards: outOfBounds used to be computed from the
    // already-clamped left, so `left + width > bounds.width` could only ever be
    // true for an element wider than the entire display. An element pushed off the
    // right edge by its x was silently shifted back instead of being reported.
    const screen: ScreenDefinition = {
      ...baseScreen,
      elements: [{ id: "pushed-off", kind: "box", x: 230, y: 10, width: 40, height: 10 }]
    };
    const report = computeLayout(screen, "landscape");
    const pushed = byId(report, "pushed-off");
    expect(pushed.outOfBounds).toBe(true);
    // It is still clamped for display, so the preview stays inside the panel.
    expect(pushed.left + pushed.width).toBeLessThanOrEqual(240);
  });

  it("flags vertical overflow caused by position", () => {
    const screen: ScreenDefinition = {
      ...baseScreen,
      elements: [{ id: "below-fold", kind: "box", x: 10, y: 130, width: 10, height: 20 }]
    };
    const report = computeLayout(screen, "landscape");
    expect(byId(report, "below-fold").outOfBounds).toBe(true);
  });

  it("does not flag an element that fits exactly at the edge", () => {
    const screen: ScreenDefinition = {
      ...baseScreen,
      elements: [{ id: "exact", kind: "box", x: 200, y: 125, width: 40, height: 10 }]
    };
    const report = computeLayout(screen, "landscape");
    expect(byId(report, "exact").outOfBounds).toBe(false);
  });
});
