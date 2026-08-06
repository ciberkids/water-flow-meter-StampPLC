import { expect, test } from "@playwright/test";
import fs from "node:fs";
import path from "node:path";
import { execSync } from "node:child_process";
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

  test("loads with a blank canvas by default", async ({ page }) => {
    await normaliseWorkspace(page, 1440, 900, { importFixture: false });

    await expect(page.locator(".screen-selector button")).toHaveCount(1);
    await expect(page.locator(".screen-selector button").first()).toContainText(/Blank Canvas/i);

    await page.getByRole("button", { name: "Design", exact: true }).click();
    await expect(page.getByTestId("design-element-list")).toContainText("No elements yet.");
  });

  test("display preview places edge coordinates at the pixel bounds", async ({ page }) => {
    await normaliseWorkspace(page, 1440, 900, { importFixture: false });

    // Use 1:1 zoom to simplify comparing DOM positions to display pixels.
    await page.locator("#zoom").evaluate((element) => {
      const input = element as HTMLInputElement;
      input.value = "100";
      input.dispatchEvent(new Event("input", { bubbles: true }));
      input.dispatchEvent(new Event("change", { bubbles: true }));
    });

    await page.getByRole("button", { name: "Design", exact: true }).click();

    // Landscape is the default orientation, so width/height swap.
    const layoutBounds = { width: DISPLAY_HEIGHT, height: DISPLAY_WIDTH };
    const maxX = DISPLAY_WIDTH;
    const maxY = DISPLAY_HEIGHT;

    const corners = [
      { label: "TL", x: 0, y: 0 },
      { label: "BL", x: 0, y: maxY },
      { label: "TR", x: maxX, y: 0 },
      { label: "BR", x: maxX, y: maxY }
    ] as const;

    for (const corner of corners) {
      await page.getByTestId("design-add-text").click();
      const elementRow = page
        .locator('[data-testid="design-element-list"] li')
        .last()
        .locator("div[data-element-id]")
        .first();
      const elementId = await elementRow.getAttribute("data-element-id");
      expect(elementId).toBeTruthy();

      await page.locator(`[data-element-id="${elementId}"] input[type="text"]`).fill(corner.label);
      const xInput = page.locator(`input[data-element-id="${elementId}"][data-field="x"]`);
      const yInput = page.locator(`input[data-element-id="${elementId}"][data-field="y"]`);
      await xInput.fill(String(corner.x));
      await yInput.fill(String(corner.y));
      await expect(xInput).toHaveValue(String(corner.x));
      await expect(yInput).toHaveValue(String(corner.y));
    }

    await page.getByRole("button", { name: "Simulation", exact: true }).click();

    const surface = page.locator(".display-surface");
    await expect(surface).toBeVisible();
    const surfaceBox = await surface.boundingBox();
    if (!surfaceBox) {
      throw new Error("Could not measure display surface");
    }

    const scaleX = surfaceBox.width / layoutBounds.width;
    const scaleY = surfaceBox.height / layoutBounds.height;
    const tolerance = 1; // pixels

    for (const corner of corners) {
      const element = page.locator(".display-element.kind-text", { hasText: corner.label });
      await expect(element).toBeVisible();
      const box = await element.boundingBox();
      if (!box) {
        throw new Error(`Could not measure element ${corner.label}`);
      }

      const relLeft = box.x - surfaceBox.x;
      const relTop = box.y - surfaceBox.y;
      const relRight = relLeft + box.width;
      const relBottom = relTop + box.height;

      const normLeft = relLeft / scaleX;
      const normTop = relTop / scaleY;
      const normRight = relRight / scaleX;
      const normBottom = relBottom / scaleY;

      if (corner.label === "TL") {
        expect(normLeft).toBeLessThanOrEqual(tolerance);
        expect(normTop).toBeLessThanOrEqual(tolerance);
      } else if (corner.label === "TR") {
        expect(Math.abs(normRight - layoutBounds.width)).toBeLessThanOrEqual(tolerance);
        expect(normTop).toBeLessThanOrEqual(tolerance);
      } else if (corner.label === "BL") {
        expect(normLeft).toBeLessThanOrEqual(tolerance);
        expect(Math.abs(normBottom - layoutBounds.height)).toBeLessThanOrEqual(tolerance);
      } else if (corner.label === "BR") {
        expect(Math.abs(normRight - layoutBounds.width)).toBeLessThanOrEqual(tolerance);
        expect(Math.abs(normBottom - layoutBounds.height)).toBeLessThanOrEqual(tolerance);
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

  test("transition preview highlights target screen", async ({ page }) => {
    await page.keyboard.press("ArrowDown");
    const overlay = page.locator(".transition-overlay");
    await expect(overlay).toBeVisible();
    await expect(overlay.locator(".transition-overlay__target")).toHaveText(
      new RegExp(screenLabel("info-cumulative", "Cumulative Liters"))
    );
  });

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
    await widthInput.fill("200");
    const clampButton = page.getByTestId("element-clamp-sensor-grid-left");
    await expect(clampButton).toBeVisible();
    await clampButton.click();
    await expect(widthInput).toHaveValue("135");
  });

  test("global clamp action fixes all overflowing elements", async ({ page }) => {
    await page.getByRole("button", { name: "Design", exact: true }).click();
    await page.getByTestId("element-select-sensor-grid-right").check();
    const widthInput = page.locator('[data-element-id="sensor-grid-right"][data-field="width"]');
    await widthInput.fill("200");
    const clampAllButton = page.getByTestId("element-clamp-all");
    await expect(clampAllButton).toBeEnabled();
    await clampAllButton.click();
    await expect(widthInput).toHaveValue("135");
  });

  test("landscape orientation keeps keyboard arrows intuitive", async ({ page }) => {
    await page.getByRole("button", { name: "Landscape", exact: true }).click();
    await page.getByRole("button", { name: "Design", exact: true }).click();
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
    const actionSelect = page.getByTestId("flow-action-select").first();
    await actionSelect.selectOption("core.action.save-config");
    await expect(page.getByTestId("live-json-editor")).toHaveValue(/core\.action\.save-config/);
  });

  test("animation inspector uploads SVG frames", async ({ page }) => {
    await page.getByRole("button", { name: "Design", exact: true }).click();
    const svgRed = Buffer.from(
      '<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16"><rect width="16" height="16" fill="red"/></svg>'
    );
    const svgBlue = Buffer.from(
      '<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16"><rect width="16" height="16" fill="blue"/></svg>'
    );
    await page.getByTestId("animation-upload").setInputFiles([
      { name: "frame-red.svg", mimeType: "image/svg+xml", buffer: svgRed },
      { name: "frame-blue.svg", mimeType: "image/svg+xml", buffer: svgBlue }
    ]);
    await expect(page.locator(".animation-card").first()).toContainText("frame-red.svg");
  });

  test("value placeholder edits emit trace entries", async ({ page }) => {
    const firstValueInput = page.locator(".value-editor-panel input").first();
    await firstValueInput.fill("S1 123.4 L/s");
    const latestTrace = page.locator(".simulation-trace-panel li").first();
    await expect(latestTrace).toContainText("Value edited");

    const saveButton = page.locator(".value-editor-panel li").first().getByRole("button", { name: "Save value" });
    await saveButton.click();
    const saveTrace = page.locator(".simulation-trace-panel li").first();
    const persistedTrace = page.locator(".simulation-trace-panel li").nth(1);
    await expect(saveTrace).toContainText("Save configuration");
    await expect(persistedTrace).toContainText("Value saved");
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

    await page.getByRole("button", { name: "Design", exact: true }).click();
    const alert = page.locator(".layout-correction-alert");
    await expect(alert).toBeVisible();
    await expect(alert).toContainText("overflow-dataset.json");
    await expect(alert).toContainText("were clamped");
  });

  test("exported IR matches dataset layout", async () => {
    const projectRoot = path.resolve(mockupRoot, "..", "..");
    const tscBin = path.resolve(mockupRoot, "node_modules/typescript/bin/tsc");
    const datasetPath = path.resolve(mockupRoot, "tests", "fixtures", "legacy-screens.json");
    execSync(`node "${tscBin}" --project tsconfig.exporter.json`, {
      cwd: mockupRoot,
      stdio: "inherit"
    });
    execSync(`node dist-exporter/tools/exporter/cli.js --screens "${datasetPath}"`, {
      cwd: mockupRoot,
      stdio: "inherit"
    });

    const dataset = JSON.parse(
      fs.readFileSync(datasetPath, "utf-8")
    ) as {
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
