import test from "node:test";
import assert from "node:assert/strict";
import { fileURLToPath } from "node:url";
import path from "node:path";
import fs from "node:fs/promises";
import { ensureValidDataset, ensureValidTheme, ExportValidationError } from "../schema.js";

async function readJson(relativePath: string) {
  const dir = path.dirname(fileURLToPath(import.meta.url));
  const projectRoot = path.resolve(dir, "..", "..", "..", "..", "..", "..");
  const absolute = path.resolve(projectRoot, relativePath);
  const raw = await fs.readFile(absolute, "utf-8");
  return JSON.parse(raw);
}

test("ensureValidDataset validates the canonical screens.json payload", async () => {
  const dataset = ensureValidDataset(await readJson("web/mockup/src/data/screens.json"));
  assert.ok(dataset.screens.length > 0);
  assert.ok(dataset.screens.every((screen) => Array.isArray(screen.elements)));
});

test("ensureValidTheme validates the default theme tokens", async () => {
  const theme = ensureValidTheme(await readJson("web/mockup/src/data/themeTokens.json"));
  assert.equal(theme.name, "StampPLC Default");
  assert.ok(theme.colors.displayBackground.startsWith("#") || theme.colors.displayBackground.startsWith("rgba"));
});

test("ensureValidDataset allows screens without elements but rejects missing screens", () => {
  const theme = ensureValidTheme({
    name: "Test",
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
    animation: { easing: "ease-in-out" }
  });

  assert.ok(
    ensureValidDataset({
      screens: [{ id: "empty", name: "Empty", elements: [] }],
      theme
    })
  );

  assert.throws(() => ensureValidDataset({ screens: [], theme }), ExportValidationError);
});

test("ensureValidTheme rejects malformed color tokens", () => {
  assert.throws(
    () =>
      ensureValidTheme({
        name: "Invalid",
        colors: {
          displayBackground: "not-a-color",
          textPrimary: "#ffffff",
          textMuted: "#ffffff",
          textStrong: "#ffffff",
          value: "#ffffff",
          badgeBackground: "#ffffff",
          badgeBorder: "#ffffff",
          icon: "#ffffff",
          legend: "#ffffff",
          gridMinor: "#ffffff",
          gridMajor: "#ffffff"
        },
        typography: { base: 8, value: 10, badge: 8 },
        animation: { easing: "ease-in-out" }
      }),
    ExportValidationError
  );
});
