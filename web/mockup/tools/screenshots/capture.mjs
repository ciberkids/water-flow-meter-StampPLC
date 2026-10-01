#!/usr/bin/env node
/**
 * Captures every screen of the SHIPPED dataset as the web designer draws it, display only.
 *
 *   cd web/mockup && npm run capture:screens        (builds first, then captures)
 *
 * Writes `graphics/screens/<screen-id>.png` (the 240 × 135 panel at the designer's default 200 % zoom) for
 * every screen in `src/data/screens.json`, and deletes any PNG there whose screen no longer exists, so the
 * folder is always exactly the current menu. `tools/wiki/sync.sh` builds the wiki's Screen Gallery from it.
 *
 * NOBODY HAS TO REMEMBER TO RUN THIS. CI runs it on every push and publishes the pictures into the wiki
 * from `main`, and the folder is gitignored, so a menu change shows up in the gallery on its next merge.
 * Run it locally only to preview. The pictures are not committed for a second reason too: the device font
 * falls back to the machine's monospace (the bundled `press-start-2p.woff2` is not a font), so a capture
 * on one machine never matches another's byte for byte, and a committed copy could only drift or churn.
 *
 * WHY THE DESIGNER AND NOT THE DEVICE. There is no way to screenshot the panel from a host, and the
 * designer renders the same dataset the exporter turns into the firmware's tables. The pictures are the
 * DEFAULT menu with the simulator's default values — what a fresh clone shows, not a live device.
 *
 * It serves the BUILT app (`vite preview` over `dist/`), like the visual suite, so run `npm run build`
 * first, which `npm run capture:screens` does: capturing a stale `dist/` is how that suite lost two rounds.
 */
import { spawn } from "node:child_process";
import fs from "node:fs";
import path from "node:path";
import { chromium } from "playwright";

const mockupRoot = path.join(import.meta.dirname, "..", "..");
const repoRoot = path.join(mockupRoot, "..", "..");
const outDir = path.join(repoRoot, "graphics", "screens");
const port = 4174; // not the visual suite's 4173, so the two can run side by side

const dataset = JSON.parse(fs.readFileSync(path.join(mockupRoot, "src", "data", "screens.json"), "utf-8"));

if (!fs.existsSync(path.join(mockupRoot, "dist", "index.html"))) {
  console.error("dist/ is missing. Run `npm run build` in web/mockup first.");
  process.exit(1);
}

const server = spawn(
  process.execPath,
  ["./node_modules/vite/bin/vite.js", "preview", "--host", "127.0.0.1", "--port", String(port), "--strictPort"],
  { cwd: mockupRoot, stdio: ["ignore", "pipe", "inherit"] }
);
await new Promise((resolve, reject) => {
  server.stdout.on("data", (chunk) => String(chunk).includes(String(port)) && resolve());
  server.on("exit", (code) => reject(new Error(`vite preview exited with ${code}`)));
});

const browser = await chromium.launch();
try {
  const page = await browser.newPage({ viewport: { width: 1440, height: 900 }, deviceScaleFactor: 1 });
  // A FIXED clock. The session-start and clock readouts derive from Date.now(), so without this the
  // images change on every run and every re-capture is a diff of nothing.
  await page.clock.setFixedTime(new Date("2026-01-01T08:00:00Z"));
  await page.goto(`http://127.0.0.1:${port}/`);
  await page.evaluate(() => {
    window.localStorage.clear();
    window.sessionStorage.clear();
  });
  await page.reload();

  // The designer's default 200 % zoom, which a cleared localStorage restores. Checked rather than set:
  // an `input` event dispatched on the slider does not reach React's onChange, so "setting" it silently
  // left it at 200 and produced images twice the intended size.
  const zoom = await page.locator("#zoom").inputValue();
  if (zoom !== "200") throw new Error(`expected the designer at 200 % zoom, found ${zoom} %`);
  const grid = page.getByLabel("Show grid overlay");
  if (await grid.isChecked()) await grid.uncheck();

  fs.mkdirSync(outDir, { recursive: true });
  const surface = page.locator(".display-surface");
  for (const screen of dataset.screens) {
    // The selector's title ends in the screen id, the one label that is unique ("M.BACK - Back" is not).
    await page.locator(`.screen-selector button[title$="· ${screen.id}"]`).click();
    await page.waitForTimeout(150);
    await surface.screenshot({ path: path.join(outDir, `${screen.id}.png`), animations: "disabled", caret: "hide" });
  }

  const current = new Set(dataset.screens.map((s) => `${s.id}.png`));
  for (const file of fs.readdirSync(outDir)) {
    if (file.endsWith(".png") && !current.has(file)) fs.rmSync(path.join(outDir, file));
  }
  console.error(`captured ${dataset.screens.length} screens into graphics/screens/`);
} finally {
  await browser.close();
  server.kill();
}
