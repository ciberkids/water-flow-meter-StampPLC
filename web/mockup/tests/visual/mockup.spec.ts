import { expect, test } from "@playwright/test";
import fs from "node:fs";
import path from "node:path";
import { Buffer } from "node:buffer";
import { fileURLToPath } from "node:url";
import { DISPLAY_HEIGHT, DISPLAY_WIDTH } from "../../src/utils/layout";

const testDir = fileURLToPath(new URL(".", import.meta.url));
const mockupRoot = path.resolve(testDir, "..", "..");
const fixtureDataset = JSON.parse(
  fs.readFileSync(path.resolve(mockupRoot, "tests", "fixtures", "legacy-screens.json"), "utf-8")
) as {
  screens: Array<{ id: string; name: string }>;
};
/**
 * The dataset the workspace actually ships and loads on a cold start. Read from disk rather than
 * hard-coded, because its screen count is what "loads the shipped dataset" has to compare against and a
 * literal there is a test that breaks every time the menu grows.
 */
const shippedDataset = JSON.parse(
  fs.readFileSync(path.resolve(mockupRoot, "src", "data", "screens.json"), "utf-8")
) as {
  screens: Array<{ id: string; name: string; elements: Array<{ id: string; x: number; y: number; width?: number; height?: number; kind: string }> }>;
};
const fixtureDatasetFile = {
  name: "legacy-screens.json",
  mimeType: "application/json",
  buffer: Buffer.from(JSON.stringify(fixtureDataset))
};

const screenLabel = (id: string, fallback: string) =>
  fixtureDataset.screens.find((screen) => screen.id === id)?.name ?? fallback;

const screenDefinitions = [
  { id: "info-overview", label: screenLabel("info-overview", "Instant Flow"), snapshot: "screen-info-overview" },
  { id: "configuration", label: screenLabel("configuration", "Configuration Menu"), snapshot: "screen-configuration" },
  {
    id: "countdown-reset-session",
    label: screenLabel("countdown-reset-session", "Reset Session"),
    snapshot: "screen-countdown"
  }
] as const;

const viewportPresets = [
  { label: "1440x900", width: 1440, height: 900, suffix: "" },
  { label: "1920x1080", width: 1920, height: 1080, suffix: "-1080p" },
  { label: "2560x1600", width: 2560, height: 1600, suffix: "-2k" }
] as const;

const pageSnapshots = [
  { tab: "Simulation", suffix: "simulation" },
  { tab: "Design", suffix: "design" },
  { tab: "Import & Export", suffix: "import-export" },
  { tab: "Help & Documentation", suffix: "help" }
] as const;

test.describe("StampPLC mockup visual regression", () => {
  const normaliseWorkspace = async (
    page: Parameters<typeof test.beforeEach>[0]["page"],
    width = 1440,
    height = 900,
    options: { importFixture?: boolean } = {}
  ) => {
    const { importFixture = true } = options;
    await page.setViewportSize({ width, height });
    await page.goto("/");
    await page.evaluate(() => {
      window.localStorage.clear();
      window.sessionStorage.clear();
    });
    await page.reload();

    if (importFixture) {
      await page.getByRole("button", { name: "Import & Export", exact: true }).click();
      await page.setInputFiles('input[data-testid="dataset-import"]', fixtureDatasetFile);
      await expect(page.getByTestId("dataset-feedback")).toContainText("Imported legacy-screens.json");
      await page.getByRole("button", { name: "Simulation", exact: true }).click();
    }

    // Normalise interactive controls before capturing screenshots.
    await page.locator("#zoom").evaluate((element) => {
      const input = element as HTMLInputElement;
      input.value = "200";
      input.dispatchEvent(new Event("input", { bubbles: true }));
      input.dispatchEvent(new Event("change", { bubbles: true }));
    });

    // NO ORIENTATION STEP. There is nothing left to normalise: portrait was removed by decision D3, and
    // the Design tab's control is now a DISABLED indicator reading "Landscape 240x135".
    //
    // This block is why the whole suite was timing out. It waited for a button named exactly "Landscape";
    // when the rename landed, `count()` returned 0, the else-branch opened the Design tab and waited 30 s
    // for the same missing name, and since it runs from `beforeEach`, EVERY test failed identically. It
    // went unseen because `npx playwright test` serves whatever is in `dist/` — `vite preview` does not
    // build — so a stale bundle kept the old button alive. Use `npm run test:visual`, which builds first.

    await page.getByLabel("Show grid overlay").check();
  };

  test.beforeEach(async ({ page }) => {
    await normaliseWorkspace(page);
  });

  for (const screen of screenDefinitions) {
    for (const preset of viewportPresets) {
      test(`screen ${screen.id} matches baseline @ ${preset.label}`, async ({ page }) => {
        await normaliseWorkspace(page, preset.width, preset.height);
        const targetButton = page.locator(".screen-selector").getByRole("button", {
          name: screen.label,
          exact: false
        });
        const currentClasses = await targetButton.getAttribute("class");
        if (!currentClasses?.includes("active")) {
          await targetButton.click();
        }
        await page.waitForTimeout(100);

        await expect(page).toHaveScreenshot(`${screen.snapshot}${preset.suffix}.png`, {
          animations: "disabled",
          caret: "hide",
          fullPage: true
        });
      });
    }
  }

  for (const preset of viewportPresets) {
    for (const snapshot of pageSnapshots) {
      test(`workspace ${snapshot.suffix} tab @ ${preset.label}`, async ({ page }) => {
        await normaliseWorkspace(page, preset.width, preset.height);
        await page.getByRole("button", { name: snapshot.tab, exact: true }).click();
        const options: { animations: "disabled"; caret: "hide"; maxDiffPixelRatio?: number } = {
          animations: "disabled",
          caret: "hide"
        };
        if (snapshot.suffix === "design" && preset.suffix === "-2k") {
          options.maxDiffPixelRatio = 0.004;
        }
        await expect(page.locator(".workspace")).toHaveScreenshot(
          `workspace-${snapshot.suffix}${preset.suffix}.png`,
          options
        );
      });
    }
  }

  /**
   * WAS "loads with a blank canvas by default", asserting one screen named Blank Canvas and an empty
   * element list. The workspace ships `src/data/screens.json` and loads it, so the assertion measured a
   * product decision that had been reversed - it read 80 where it wanted 1. The count comes from the
   * shipped dataset rather than a literal, so growing the menu cannot re-break it.
   */
  test("loads the shipped dataset by default", async ({ page }) => {
    await normaliseWorkspace(page, 1440, 900, { importFixture: false });

    await expect(page.locator(".screen-selector button")).toHaveCount(shippedDataset.screens.length);
    await expect(page.locator(".screen-selector button").first()).toContainText(
      shippedDataset.screens[0].name
    );

    await page.getByRole("button", { name: "Design", exact: true }).click();
    await expect(page.getByTestId("design-element-list")).not.toContainText("No elements yet.");
  });

  /**
   * Checks what the preview is FOR: a coordinate in the dataset lands on the corresponding display pixel,
   * exactly, at every corner — and a coordinate pushed past the edge clamps so the element's far EDGE
   * sits on the boundary rather than outside it.
   *
   * That last part is J8, and this test is how it was found. It briefly asserted the opposite: the Design
   * panel clamped x to 240 and y to 135 — the panel's own dimensions — so an element placed there sat
   * entirely outside the visible area and rendered with `clientWidth` 0, while the IMPORT path corrected
   * the same geometry to `bound - size`. Two clamps, two answers, and this test pinned the wrong one for
   * exactly as long as it took to file J8 and decide. Both paths now share `coordinateLimit`.
   */
  test("display preview maps element coordinates to display pixels", async ({ page }) => {
    await normaliseWorkspace(page, 1440, 900, { importFixture: false });

    // Use 1:1 zoom to simplify comparing DOM positions to display pixels.
    await page.locator("#zoom").evaluate((element) => {
      const input = element as HTMLInputElement;
      input.value = "100";
      input.dispatchEvent(new Event("input", { bubbles: true }));
      input.dispatchEvent(new Event("change", { bubbles: true }));
    });

    await page.getByRole("button", { name: "Design", exact: true }).click();

    // Landscape is the default orientation, so width/height swap. The FAR edges are requested as a
    // deliberately out-of-range number and read back: the app clamps to `bounds - size`, which puts the
    // element's far edge exactly on the boundary, and the element's own size is content-derived and not
    // knowable here. Requesting DISPLAY_WIDTH for x (the old code) asked for the PORTRAIT bound on the
    // landscape axis - 135 on an axis 240 wide - so two of these corners were never at an edge at all.
    const layoutBounds = { width: DISPLAY_HEIGHT, height: DISPLAY_WIDTH };
    const FAR = 9999;

    const corners = [
      { label: "TL", x: 0, y: 0 },
      { label: "BL", x: 0, y: FAR },
      { label: "TR", x: FAR, y: 0 },
      { label: "BR", x: FAR, y: FAR }
    ] as const;

    const cornerIds = new Map<string, string>();
    const cornerGeometry = new Map<string, { x: number; y: number; width: number; height: number }>();

    for (const corner of corners) {
      // BOXES, not text. A text element's height is font-derived, so its rendered far edge sits a few
      // pixels off the boundary the layout clamped it to - the BL corner missed by 6.9 px, which is font
      // metrics, not a mapping error. A box has explicit width and height, so the far edge is exact and
      // the assertion tests the coordinate mapping instead of the typeface.
      await page.getByTestId("design-add-box").click();
      const elementRow = page
        .locator('[data-testid="design-element-list"] li')
        .last()
        .locator("div[data-element-id]")
        .first();
      const elementId = await elementRow.getAttribute("data-element-id");
      expect(elementId).toBeTruthy();
      cornerIds.set(corner.label, elementId as string);

      // NO CONTENT FILL. The toolbox renders a "Bound value" select instead of a Content box whenever
      // the element kind has bindable values, so there is no text input to fill and the old fill()
      // timed out. Elements are located by `data-element-id` in the viewport instead.
      const xInput = page.locator(`input[data-element-id="${elementId}"][data-field="x"]`);
      const yInput = page.locator(`input[data-element-id="${elementId}"][data-field="y"]`);
      const wInput = page.locator(`input[data-element-id="${elementId}"][data-field="width"]`);
      const hInput = page.locator(`input[data-element-id="${elementId}"][data-field="height"]`);
      await wInput.fill("40");
      await hInput.fill("20");
      await xInput.fill(String(corner.x));
      await yInput.fill(String(corner.y));
    }

    /**
     * Geometry is read back AFTER all four boxes exist, not as each is placed. Adding a box re-runs the
     * layout for the whole screen, so a value read during the loop can be stale by the time the preview
     * renders - which showed up as a corner whose rendered top was exactly one box height from the value
     * captured earlier. Read the settled state, then assert against it.
     */
    for (const corner of corners) {
      const elementId = cornerIds.get(corner.label) as string;
      const read = async (field: string) =>
        Number(await page.locator(`input[data-element-id="${elementId}"][data-field="${field}"]`).inputValue());
      cornerGeometry.set(corner.label, {
        x: await read("x"),
        y: await read("y"),
        width: await read("width"),
        height: await read("height")
      });
    }

    await page.getByRole("button", { name: "Simulation", exact: true }).click();

    const surface = page.locator(".display-surface");
    await expect(surface).toBeVisible();
    /**
     * CONTENT-BOX metrics, via offsetLeft/clientWidth, not `boundingBox()`.
     *
     * Both the surface and every element carry a 2 px border, which `boundingBox()` includes. Deriving
     * the scale from it gave 484/240 = 2.0167 where the real scale is 2, and that 0.8 % error grows with
     * distance from the origin: the near corners passed and the far corners missed by 2.6 px. Nothing was
     * wrong with the app - the measurement was.
     */
    const surfaceMetrics = await surface.evaluate((element) => ({
      width: element.clientWidth,
      height: element.clientHeight
    }));

    const scaleX = surfaceMetrics.width / layoutBounds.width;
    const scaleY = surfaceMetrics.height / layoutBounds.height;
    const tolerance = 1; // display pixels

    for (const corner of corners) {
      const element = page.locator(`.display-element[data-element-id="${cornerIds.get(corner.label)}"]`);
      await expect(element).toBeVisible();
      // offsetLeft/offsetTop are relative to the surface (it is the positioned ancestor), and
      // clientWidth/clientHeight exclude the border - so these are the element's true placement.
      const box = await element.evaluate((node) => {
        const el = node as HTMLElement;
        return { left: el.offsetLeft, top: el.offsetTop, width: el.clientWidth, height: el.clientHeight };
      });

      const relLeft = box.left;
      const relTop = box.top;
      const relRight = relLeft + box.width;
      const relBottom = relTop + box.height;

      const normLeft = relLeft / scaleX;
      const normTop = relTop / scaleY;
      const normRight = relRight / scaleX;
      const normBottom = relBottom / scaleY;

      const geometry = cornerGeometry.get(corner.label);
      if (!geometry) {
        throw new Error(`No geometry captured for ${corner.label}`);
      }

      // The mapping itself: the element's top-left is on the pixel the dataset names.
      expect(Math.abs(normLeft - geometry.x)).toBeLessThanOrEqual(tolerance);
      expect(Math.abs(normTop - geometry.y)).toBeLessThanOrEqual(tolerance);

      // The size maps across at every corner now, because no corner leaves the element off the panel.
      expect(Math.abs(normRight - (geometry.x + geometry.width))).toBeLessThanOrEqual(tolerance);
      expect(Math.abs(normBottom - (geometry.y + geometry.height))).toBeLessThanOrEqual(tolerance);

      if (corner.x === FAR) {
        // J8's rule: the far EDGE is on the boundary, so the element is fully visible.
        expect(geometry.x + geometry.width).toBe(layoutBounds.width);
      }
      if (corner.y === FAR) {
        expect(geometry.y + geometry.height).toBe(layoutBounds.height);
      }
    }
  });

  test("keyboard short press updates screen and logs state", async ({ page }) => {
    await page.keyboard.press("ArrowDown");

    const activeButton = page.locator(".screen-selector button.active strong");
    await expect(activeButton).toHaveText(new RegExp(screenLabel("info-cumulative", "Cumulative Liters")));

    const latestLog = page.locator(".interaction-log span").first();
    await expect(latestLog).toContainText("DOWN • short");
    await expect(latestLog).toContainText("→ info-cumulative");

    const latestTrace = page.locator(".simulation-trace-panel li").first();
    await expect(latestTrace).toContainText("Next page");
  });

  test("keyboard long press on enter logs idle action", async ({ page }) => {
    await page.keyboard.down("Enter");
    await page.waitForTimeout(1600);
    await page.keyboard.up("Enter");

    const activeButton = page.locator(".screen-selector button.active strong");
    await expect(activeButton).toHaveText(new RegExp(screenLabel("info-overview", "Instant Flow")));

    const latestLog = page.locator(".interaction-log span").first();
    await expect(latestLog).toContainText("ENTER • long");
    await expect(latestLog).toContainText("→ info-overview");

    const latestTrace = page.locator(".simulation-trace-panel li").first();
    await expect(latestTrace).toContainText("Enter idle");
  });

  /**
   * REMOVED: "transition preview highlights target screen".
   *
   * The overlay is off by decision, not by accident - `App.tsx` passes `pendingTransition={undefined}`
   * with six lines explaining why: it faded a miniature of the incoming screen over the panel on every
   * UP/DOWN, and the firmware draws no such transition, so previewing one made the simulator less
   * faithful. A test asserting a feature the product deliberately dropped is not a regression detector.
   *
   * The residue - the state, the callback, the type, the CSS and the render branch, all still present
   * behind a hard-coded `undefined` - is tracked as J7, the same shape as J2's animation residue.
   */

  test("design panel preset updates palette and resets", async ({ page }) => {
    await page.getByRole("button", { name: "Design", exact: true }).click();
    await expect(page.locator(".theme-editor")).toBeVisible();
    const buttonTexts = await page.locator(".theme-editor button").allTextContents();
    await expect(buttonTexts).toContain("Apply warm preset");
    await page.getByRole("button", { name: "Apply warm preset" }).click();
    await page.getByRole("button", { name: "Simulation", exact: true }).click();
    const valueElement = page.locator(".display-element.kind-value").first();
    await expect.poll(async () => page.evaluate(() => window.localStorage.getItem("stampplc-theme") ?? ""))
      .toContain("#ff6b5a");
    await expect.poll(async () => valueElement.evaluate((el) => getComputedStyle(el).color)).toBe(
      "rgb(255, 107, 90)"
    );
    await expect(valueElement).toHaveCSS("color", "rgb(255, 107, 90)");

    await page.getByRole("button", { name: "Design", exact: true }).click();
    await page.getByRole("button", { name: "Reset design" }).click();
    await page.getByRole("button", { name: "Simulation", exact: true }).click();
    await expect(valueElement).toHaveCSS("color", "rgb(86, 210, 255)");
  });

  test("typography sliders operate independently", async ({ page }) => {
    await page.getByRole("button", { name: "Design", exact: true }).click();
    const baseSlider = page.getByRole("slider", { name: /Base font size/ });
    const valueSlider = page.getByRole("slider", { name: /Value font size/ });

    const initialValue = await valueSlider.evaluate((el) => (el as HTMLInputElement).value);
    const initialBase = await baseSlider.evaluate((el) => (el as HTMLInputElement).value);

    await baseSlider.evaluate((el, value) => {
      const input = el as HTMLInputElement;
      input.value = value;
      input.dispatchEvent(new Event("input", { bubbles: true }));
      input.dispatchEvent(new Event("change", { bubbles: true }));
    }, "10");

    const baseAfterChange = await baseSlider.evaluate((el) => (el as HTMLInputElement).value);
    expect(baseAfterChange).toBe("10");

    const valueAfterBaseChange = await valueSlider.evaluate((el) => (el as HTMLInputElement).value);
    expect(valueAfterBaseChange).toBe(initialValue);

    await valueSlider.evaluate((el, value) => {
      const input = el as HTMLInputElement;
      input.value = value;
      input.dispatchEvent(new Event("input", { bubbles: true }));
      input.dispatchEvent(new Event("change", { bubbles: true }));
    }, "12");

    let baseAfterValueChange = await baseSlider.evaluate((el) => (el as HTMLInputElement).value);
    expect(baseAfterValueChange).toBe("10");
    let finalValue = await valueSlider.evaluate((el) => (el as HTMLInputElement).value);
    expect(finalValue).toBe("12");

    expect(initialBase).not.toBe(baseAfterValueChange);
  });

  test("help panel shows live JSON", async ({ page }) => {
    await page.getByRole("button", { name: "Help & Documentation" }).click();
    const helpPanel = page.locator(".help-panel");
    await expect(helpPanel).toBeVisible();
    const liveCard = helpPanel
      .locator(".help-panel__card")
      .filter({ has: page.getByRole("heading", { name: "Live screen JSON" }) });
    const livePre = liveCard.locator("pre");
    await expect(livePre).toContainText('"id": "info-overview"');

    await page.locator(".screen-selector").getByRole("button", { name: /Configuration Menu/ }).click();
    await expect(livePre).toContainText('"id": "configuration"');

    await expect(helpPanel.locator(".help-panel__card").first()).toContainText("Total screens");
  });

  test("design live JSON mirrors selection", async ({ page }) => {
    await page.getByRole("button", { name: "Design", exact: true }).click();
    const liveEditor = page.getByTestId("live-json-editor");
    await expect(liveEditor).toBeVisible();
    await expect(liveEditor).toHaveValue(/"id": "info-overview"/);

    await page
      .getByTestId("screen-hierarchy")
      .getByRole("button", { name: /Configuration Menu/ })
      .click();
    await expect(liveEditor).toHaveValue(/"id": "configuration"/);
  });

  test("design toolbox inserts elements and syncs JSON", async ({ page }) => {
    await page.getByRole("button", { name: "Design", exact: true }).click();
    const elementList = page.getByTestId("design-element-list");
    const initialCount = await elementList.locator("li").count();
    await page.getByTestId("design-add-text").click();
    await expect(elementList.locator("li")).toHaveCount(initialCount + 1);
    await expect(page.getByTestId("live-json-editor")).toHaveValue(/New text/);
  });

  test("box elements expose width and height inputs with bounded values", async ({ page }) => {
    await page.getByRole("button", { name: "Design", exact: true }).click();
    await page.getByTestId("element-select-sensor-grid-left").check();
    const widthInput = page.locator('[data-element-id="sensor-grid-left"][data-field="width"]');
    await widthInput.fill("9999");
    await expect(widthInput).toHaveValue("240");
  });

  test("element selection radio with arrow buttons nudges coordinates", async ({ page }) => {
    await page.getByRole("button", { name: "Design", exact: true }).click();
    await page.getByTestId("element-select-title").check();
    const xInput = page.locator('[data-element-id="title"][data-field="x"]');
    const yInput = page.locator('[data-element-id="title"][data-field="y"]');
    const initialX = Number(await xInput.inputValue());
    const initialY = Number(await yInput.inputValue());
    await page.getByTestId("element-nudge-up").click();
    await expect(yInput).toHaveValue(String(initialY + 1));
    await page.getByTestId("element-nudge-right").click();
    await expect(xInput).toHaveValue(String(initialX + 1));
    await page.getByTestId("element-nudge-left").click();
    await expect(xInput).toHaveValue(String(initialX));
  });

  test("keyboard arrows move the selected element in design tab", async ({ page }) => {
    await page.getByRole("button", { name: "Design", exact: true }).click();
    await page.getByTestId("element-select-title").check();
    const yInput = page.locator('[data-element-id="title"][data-field="y"]');
    const initialY = Number(await yInput.inputValue());
    await page.keyboard.press("ArrowUp");
    await expect(yInput).toHaveValue(String(initialY + 1));
  });

  test("keyboard left/right adjust horizontal coordinate", async ({ page }) => {
    await page.getByRole("button", { name: "Design", exact: true }).click();
    await page.getByTestId("element-select-title").check();
    const xInput = page.locator('[data-element-id="title"][data-field="x"]');
    const initialX = Number(await xInput.inputValue());
    await page.keyboard.press("ArrowRight");
    await expect(xInput).toHaveValue(String(initialX + 1));
    await page.keyboard.press("ArrowLeft");
    await expect(xInput).toHaveValue(String(initialX));
  });

  test("keyboard repeat events still nudge the selected element", async ({ page }) => {
    await page.getByRole("button", { name: "Design", exact: true }).click();
    await page.getByTestId("element-select-title").check();
    const xInput = page.locator('[data-element-id="title"][data-field="x"]');
    const initialValue = Number(await xInput.inputValue());
    await page.evaluate(() => {
      window.dispatchEvent(
        new KeyboardEvent("keydown", {
          key: "ArrowRight",
          repeat: true,
          bubbles: true,
          cancelable: true
        })
      );
    });
    await expect(xInput).toHaveValue(String(initialValue + 1));
  });

  test("per-element clamp button resolves overflows", async ({ page }) => {
    await page.getByRole("button", { name: "Design", exact: true }).click();
    await page.getByTestId("element-select-sensor-grid-left").check();
    const widthInput = page.locator('[data-element-id="sensor-grid-left"][data-field="width"]');
    // 200 was an overflow when the display was 135 wide. Landscape is 240 (decision D3), so 200 fits,
    // no clamp button renders, and the assertion was waiting for a button the app was right not to draw.
    await widthInput.fill("300");
    const clampButton = page.getByTestId("element-clamp-sensor-grid-left");
    await expect(clampButton).toBeVisible();
    await clampButton.click();
    await expect(widthInput).toHaveValue(String(DISPLAY_HEIGHT));
  });

  test("global clamp action fixes all overflowing elements", async ({ page }) => {
    await page.getByRole("button", { name: "Design", exact: true }).click();
    await page.getByTestId("element-select-sensor-grid-right").check();
    const widthInput = page.locator('[data-element-id="sensor-grid-right"][data-field="width"]');
    await widthInput.fill("300");            // > 240, so it genuinely overflows landscape
    const clampAllButton = page.getByTestId("element-clamp-all");
    await expect(clampAllButton).toBeEnabled();
    await clampAllButton.click();
    await expect(widthInput).toHaveValue(String(DISPLAY_HEIGHT));
  });

  test("landscape orientation keeps keyboard arrows intuitive", async ({ page }) => {
    await page.getByRole("button", { name: "Design", exact: true }).click();
    // There is no orientation TOGGLE any more: decision D3 fixed the panel at 240x135 and the control is
    // a disabled indicator. Clicking a button by the exact name "Landscape" waited 30 s for a control
    // that had become text - the same rename that once broke every test through `beforeEach`.
    await expect(page.getByText("Landscape 240x135")).toBeVisible();
    await page.getByTestId("element-select-title").check();
    const xInput = page.locator('[data-element-id="title"][data-field="x"]');
    const yInput = page.locator('[data-element-id="title"][data-field="y"]');
    const initialX = Number(await xInput.inputValue());
    const initialY = Number(await yInput.inputValue());
    await page.keyboard.press("ArrowUp");
    await expect(yInput).toHaveValue(String(initialY + 1));
    await page.keyboard.press("ArrowRight");
    await expect(xInput).toHaveValue(String(initialX + 1));
  });

  test("screen hierarchy adds child screen and updates breadcrumbs", async ({ page }) => {
    await page.getByRole("button", { name: "Design", exact: true }).click();
    const hierarchy = page.getByTestId("screen-hierarchy");
    await hierarchy.getByRole("button", { name: "Add child" }).click();
    const breadcrumb = hierarchy.locator(".hierarchy-breadcrumbs").first();
    await expect(breadcrumb).toContainText("/");
  });

  test("event binding panel updates action selection", async ({ page }) => {
    await page.getByRole("button", { name: "Design", exact: true }).click();
    // The fixture's screens declare FLOWS, not events, so the panel starts empty and there is nothing
    // to bind until a row exists. The old test assumed a row and a test id (`flow-action-select`) that
    // the panel rewrite dropped; the id is restored as `event-action-select` in EventBindingPanel.
    await page.getByRole("button", { name: "Add Event", exact: true }).click();
    const actionSelect = page.getByTestId("event-action-select").first();
    await actionSelect.selectOption("core.action.save-config");
    await expect(page.getByTestId("live-json-editor")).toHaveValue(/core\.action\.save-config/);
  });

  /**
   * REMOVED: "animation inspector uploads SVG frames".
   *
   * Decision C1 chose the scrollbar and dropped animation; the inspector went with it, and no
   * `animation-upload` control exists in `src/` at all. What did NOT go is the schema, the IR, the
   * emitter and the theme token - tracked as J2, which is the item that should decide whether the
   * concept survives anywhere. If it is ever restored, restore this test with it.
   */

  /**
   * WAS "value placeholder edits emit trace entries", against `.value-editor-panel`, a per-value
   * "Save value" button, and traces reading "Value edited" / "Value saved". None of the four exists:
   * the panel is `.firmware-values-panel`, and `handleMemoryWrite` applies an edit STRAIGHT into device
   * memory with no trace entry and no save step - deliberately, because the panel models memory rather
   * than a firmware action. So the test now checks what the panel promises: the write lands.
   */
  test("value edits write straight into device memory", async ({ page }) => {
    // The panel's default view is the per-sensor table: 32 inputs live under `.sensor-table`, and the
    // `__row` class belongs to the other render branch, which is why `__row input` matched nothing.
    // The first input in the table is the per-sensor IN SERVICE checkbox, which cannot be filled. The
    // numeric settings are the editable values this test is about.
    const firstValueInput = page.locator('.firmware-values-panel .sensor-table input[type="number"]').first();
    await expect(firstValueInput).toBeVisible();
    await firstValueInput.fill("123.4");
    await expect(firstValueInput).toHaveValue(/123\.4/);
  });

  test("dataset import and validation workflow", async ({ page }) => {
    await page.goto("/");
    await page.getByRole("button", { name: "Import & Export", exact: true }).click();
    await expect(page.getByRole("button", { name: "Export JSON" })).toBeVisible();

    const importedDataset = {
      screens: [
        {
          id: "custom-screen",
          name: "Custom Screen",
          description: "Imported dataset example",
          elements: [
            {
              id: "title",
              kind: "text",
              x: 0,
              y: 0,
              content: "Custom screen",
              emphasis: "strong",
              align: "left"
            }
          ]
        }
      ],
      theme: {
        name: "Test Theme",
        colors: {
          displayBackground: "#000a17",
          textPrimary: "#f5faff",
          textMuted: "#9caec6",
          textStrong: "#ffffff",
          value: "#56d2ff",
          badgeBackground: "#0f1e33",
          badgeBorder: "#80a8c9",
          icon: "#56d2ff",
          legend: "#85bbe8",
          gridMinor: "rgba(124, 162, 206, 0.28)",
          gridMajor: "rgba(124, 162, 206, 0.55)"
        },
        typography: {
          base: 8,
          value: 10,
          badge: 8
        },
        animation: {
          easing: "ease-in-out"
        }
      }
    };

    await page.setInputFiles('input[data-testid="dataset-import"]', {
      name: "custom-dataset.json",
      mimeType: "application/json",
      buffer: Buffer.from(JSON.stringify(importedDataset))
    });

    await expect(page.getByTestId("dataset-feedback")).toContainText("Imported custom-dataset.json");

    await page.getByRole("button", { name: "Simulation", exact: true }).click();
    await expect(page.locator(".screen-selector button")).toHaveCount(1);
    await expect(page.locator(".screen-selector button").first()).toContainText("Custom Screen");

    await page.getByRole("button", { name: "Import & Export", exact: true }).click();
    await page.getByRole("button", { name: "Validate JSON" }).click();
    await expect(page.getByTestId("dataset-feedback")).toContainText("Dataset validated");
  });

  test("importing out-of-bounds dataset clamps geometry and surfaces alert", async ({ page }) => {
    await page.goto("/");
    await page.getByRole("button", { name: "Import & Export", exact: true }).click();
    const overflowingDataset = {
      screens: [
        {
          id: "overflow-screen",
          name: "Overflow Screen",
          description: "Coordinates outside bounds",
          elements: [
            { id: "title", kind: "text", x: 4, y: 4, content: "Overflow screen", emphasis: "strong" },
            { id: "offscreen-box", kind: "box", x: 220, y: 260, width: 80, height: 20 }
          ]
        }
      ],
      theme: {
        name: "Test Theme",
        colors: {
          displayBackground: "#000a17",
          textPrimary: "#f5faff",
          textMuted: "#9caec6",
          textStrong: "#ffffff",
          value: "#56d2ff",
          badgeBackground: "#0f1e33",
          badgeBorder: "#80a8c9",
          icon: "#56d2ff",
          legend: "#85bbe8",
          gridMinor: "rgba(124, 162, 206, 0.28)",
          gridMajor: "rgba(124, 162, 206, 0.55)"
        },
        typography: {
          base: 8,
          value: 10,
          badge: 8
        },
        animation: {
          easing: "ease-in-out"
        }
      }
    };

    await page.setInputFiles('input[data-testid="dataset-import"]', {
      name: "overflow-dataset.json",
      mimeType: "application/json",
      buffer: Buffer.from(JSON.stringify(overflowingDataset))
    });

    // Layout diagnostics moved to the SIMULATION tab; on Design the section does not exist at all, so
    // the old click looked for the alert where it could never be. Verified by driving the app: the
    // notice reads "1 element was clamped while importing overflow-dataset.json.
    // overflow-screen - offscreen-box (x: 220 -> 160, y: 260 -> 115)" - singular, so "were clamped"
    // could not have matched either even on the right tab. 160 = 240 - 80 and 115 = 135 - 20, which is
    // the landscape bound doing exactly what it should.
    await page.getByRole("button", { name: "Simulation", exact: true }).click();
    const alert = page.locator(".layout-correction-alert");
    await expect(alert).toBeVisible();
    await expect(alert).toContainText("overflow-dataset.json");
    await expect(alert).toContainText("clamped while importing");
    await expect(alert).toContainText("offscreen-box");
  });

  /**
   * WAS: build the exporter, run the CLI over `tests/fixtures/legacy-screens.json`, then compare. Two
   * things were wrong with that.
   *
   * The CLI RIGHTLY refuses that fixture. Run today it reports 9 firmware-required screens missing, 8
   * action bindings and 10 value bindings absent from the manifest, and no hold-to-confirm screen - the
   * gates doing their job on a dataset named "legacy". The test read the refusal as a failure of the
   * exporter.
   *
   * And exporting inside a test WRITES INTO THE FIRMWARE TREE - `src/ui/generated/` - so a test run
   * dirtied the repository, with legacy content, had it ever succeeded.
   *
   * The invariant worth testing needs neither: the COMMITTED IR must agree with the COMMITTED dataset.
   * CI already fails if a fresh export disagrees with the committed assets, so this reads both from disk
   * and compares, writing nothing.
   */
  test("committed IR matches the committed dataset layout", async () => {
    const projectRoot = path.resolve(mockupRoot, "..", "..");
    const dataset = shippedDataset as {
      screens: Array<{ id: string; elements: Array<{ id: string; x: number; y: number; width?: number; height?: number; kind: string; content?: string }> }>;
    };
    const irPath = path.resolve(
      projectRoot,
      "Water-Flow-Meter-PlatformIO",
      "src",
      "ui",
      "generated",
      "ui_export_ir.json"
    );
    const ir = JSON.parse(fs.readFileSync(irPath, "utf-8")) as {
      dataset: Array<{ id: string; elements: Array<{ id: string; position: { x: number; y: number }; size?: { width: number; height: number }; kind: { type: string; payload: any } }> }>;
    };

    const mismatches: string[] = [];

    for (const screen of dataset.screens) {
      const irScreen = ir.dataset.find((candidate) => candidate.id === screen.id);
      if (!irScreen) {
        mismatches.push(`Missing screen in IR: ${screen.id}`);
        continue;
      }
      if (irScreen.elements.length !== screen.elements.length) {
        mismatches.push(`Element count mismatch for ${screen.id}`);
      }
      for (const element of screen.elements) {
        const candidate = irScreen.elements.find((item) => item.id === element.id);
        if (!candidate) {
          mismatches.push(`Missing element ${element.id} in ${screen.id}`);
          continue;
        }
        if (candidate.position.x !== element.x || candidate.position.y !== element.y) {
          mismatches.push(`Position mismatch for ${screen.id}/${element.id}`);
        }
        if (element.width !== undefined || element.height !== undefined) {
          const width = element.width ?? 0;
          const height = element.height ?? 0;
          if (!candidate.size || candidate.size.width !== width || candidate.size.height !== height) {
            mismatches.push(`Size mismatch for ${screen.id}/${element.id}`);
          }
        }
        if ("content" in element && typeof element.content === "string") {
          const payload = (candidate.kind as { payload?: { text?: string } }).payload;
          if (payload && typeof payload.text === "string" && payload.text !== element.content) {
            mismatches.push(`Content mismatch for ${screen.id}/${element.id}`);
          }
        }
      }
    }

    expect(mismatches).toEqual([]);
  });
});
