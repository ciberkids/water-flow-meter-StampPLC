import { expect, test, type Page } from "@playwright/test";
import fs from "node:fs";
import path from "node:path";
import { Buffer } from "node:buffer";
import { fileURLToPath } from "node:url";
import { warningBannerLayout } from "../../src/utils/warningBanner";

/**
 * The warning banner, rendered — the coverage DF17's repair did NOT provide.
 *
 * Every snapshot in `mockup.spec.ts` captures a workspace with no warning live, so `warningBanner` is empty
 * in all of them and the band at y=116 is painted in none. §2c's relocation of that band from y=34 was
 * therefore asserted by host tests and by `tools/audit/screen-geometry.ts`, and seen by nothing (DF20).
 *
 * This file exists rather than more cases in `mockup.spec.ts` for two reasons. The band is not a screen, so
 * it needs its own setup — a sensor driven into a fault — and none of that belongs in a `beforeEach` shared
 * with 44 other tests. And its snapshot clips to `.display-surface` instead of `fullPage`, which is what the
 * band actually is: 240 x 135 of emulated panel, ~10 KB, indifferent to every workspace control around it.
 * The full-page baselines next door are 200-800 KB each and move whenever any panel changes.
 *
 * GEOMETRY IS ASSERTED IN LAYOUT PIXELS, not from `boundingBox()`. The panel scales by a CSS transform, so
 * `offsetTop` and `clientWidth` read the unscaled numbers the renderer was given.
 *
 * THE NUMBERS ARE LITERALS — 116, 18, 4, 16 — read off `UiRenderer::drawWarningBanner` and spelled out here.
 * They were written as `warningBannerLayout.y` and so on at first, which is a test that cannot fail: the
 * negative check moved the band back to y=34, the module and the assertion moved together, and only the
 * SNAPSHOT noticed. A test deriving its expectation from the same constant the code uses tracks the code
 * instead of holding it. `warningBannerLayout` is still imported, for one assertion whose whole job is to
 * catch the module drifting away from these literals.
 */

const testDir = fileURLToPath(new URL(".", import.meta.url));
const mockupRoot = path.resolve(testDir, "..", "..");
const fixture = JSON.parse(
  fs.readFileSync(path.resolve(mockupRoot, "tests", "fixtures", "legacy-screens.json"), "utf-8")
) as { screens: Array<{ id: string; name: string }> };

/** A clean workspace on a known dataset, on the Simulation tab. */
async function openSimulation(page: Page) {
  await page.setViewportSize({ width: 1440, height: 900 });
  await page.goto("/");
  await page.evaluate(() => {
    window.localStorage.clear();
    window.sessionStorage.clear();
  });
  await page.reload();
  await page.getByRole("button", { name: "Import & Export", exact: true }).click();
  await page.setInputFiles('input[data-testid="dataset-import"]', {
    name: "legacy-screens.json",
    mimeType: "application/json",
    buffer: Buffer.from(JSON.stringify(fixture))
  });
  await expect(page.getByTestId("dataset-feedback")).toContainText("Imported legacy-screens.json");
  await page.getByRole("button", { name: "Simulation", exact: true }).click();
}

/**
 * The row for one sensor, found by the number it PRINTS rather than by position.
 *
 * `FirmwareValuesPanel` carries a comment about exactly this: reading state from position while writing by
 * field is the split that let a checkbox toggle one sensor while displaying another.
 */
const sensorRow = (page: Page, sensorNumber: number) =>
  page.locator(".sensor-row").filter({ has: page.getByRole("button", { name: `S${sensorNumber}` }) });

/** Content-box geometry, in the unscaled layout pixels the renderer was handed. */
const layoutBox = (page: Page, testId: string) =>
  page.getByTestId(testId).evaluate((node) => {
    const el = node as HTMLElement;
    return { left: el.offsetLeft, top: el.offsetTop, width: el.clientWidth, height: el.clientHeight };
  });

test.describe("the warning banner is drawn, and drawn where §2c put it", () => {
  test("a sampling override raises the band at y=116, 18 px tall, edge to edge", async ({ page }) => {
    await openSimulation(page);

    // The band is absent until something is wrong: the gate is `hasWarnings || (uncalibrated && !editing)`.
    await expect(page.getByTestId("warning-banner-band")).toHaveCount(0);

    // `override` models the operator's half of §5.5 — the one per-sensor control that raises a SAMPLING
    // warning without touching the calibration, so this case exercises the gate's first clause alone.
    await sensorRow(page, 1).locator("label").filter({ hasText: "override" }).locator("input").check();

    const band = page.getByTestId("warning-banner-band");
    await expect(band).toHaveCount(1);

    const box = await layoutBox(page, "warning-banner-band");
    expect(box.top).toBe(116); // `bannerY`, moved from 34 by §2c
    expect(box.height).toBe(18); // `bannerH`
    expect(box.left).toBe(0);
    expect(box.width).toBe(240); // edge to edge
    // The firmware carries `static_assert(bannerY + bannerH <= kPanelHeight)`; this is its counterpart.
    expect(box.top + box.height).toBeLessThanOrEqual(135);

    // Marker and text sit at the two x positions the firmware draws them at, both `bannerY + 4`.
    const marker = await layoutBox(page, "warning-banner-marker");
    const text = await layoutBox(page, "warning-banner-text");
    expect(marker.left).toBe(4);
    expect(text.left).toBe(16);
    expect(marker.top).toBe(120);
    expect(text.top).toBe(120);

    // And the module the app renders from still agrees with those literals, so a change there is a named
    // failure here rather than a silent re-baseline.
    expect({
      y: warningBannerLayout.y,
      height: warningBannerLayout.height,
      markerX: warningBannerLayout.markerX,
      textX: warningBannerLayout.textX,
      textDy: warningBannerLayout.textDy,
      panelWidth: warningBannerLayout.panelWidth
    }).toEqual({ y: 116, height: 18, markerX: 4, textX: 16, textDy: 4, panelWidth: 240 });

    // The wording is a COUNT, not a channel list: the list reached 50 characters on eight flagged sensors
    // and `drawWarningBanner` has no truncation, so it overflowed the panel silently.
    await expect(page.getByTestId("warning-banner-text")).toHaveText(/sampling/i);

    // The panel only — 240 x 135 of it. Not the workspace.
    await expect(page.locator(".display-surface")).toHaveScreenshot("panel-sampling-warning.png", {
      animations: "disabled",
      caret: "hide"
    });
  });

  test("an uncalibrated channel raises it too — the half §2c widened the gate for", async ({ page }) => {
    await openSimulation(page);
    await expect(page.getByTestId("warning-banner-band")).toHaveCount(0);

    /**
     * Clearing `q_max` is how the device reaches "in service, no valid calibration": `configIsValid`
     * refuses a zero maximum, and since DF16 the mockup DERIVES `ready` from that predicate, so this one
     * edit is the whole commissioning gap. Before DF16 it left the row reading `OK` and raised nothing.
     *
     * The route is the panel's own: select the sensor, which is what fills the "Sensor settings" group —
     * the group heading names the selection, exactly as `UiNavigator` scopes the device's config screens to
     * one channel — then open the group and write the field.
     */
    await sensorRow(page, 1).getByRole("button", { name: "S1" }).click();
    await page.getByRole("button", { name: /Sensor settings \(S1\)/ }).click();
    const qMax = page.locator('[id="value-config.sensor.maxFlow"]');
    await expect(qMax).toBeVisible();
    await qMax.fill("0");
    await qMax.blur();

    await expect(page.getByTestId("warning-banner-band")).toHaveCount(1);
    const box = await layoutBox(page, "warning-banner-band");
    expect(box.top).toBe(116);
    expect(box.height).toBe(18);
    await expect(page.getByTestId("warning-banner-text")).toHaveText(/calibrat/i);
  });

  test("the band clears when the fault does", async ({ page }) => {
    await openSimulation(page);
    const override = sensorRow(page, 1)
      .locator("label")
      .filter({ hasText: "override" })
      .locator("input");

    await override.check();
    await expect(page.getByTestId("warning-banner-band")).toHaveCount(1);
    await override.uncheck();
    // Asserting the CHANGE, not just the state: a banner that never clears looks identical to one that
    // never lit while a fault happens to be live.
    await expect(page.getByTestId("warning-banner-band")).toHaveCount(0);
  });
});
