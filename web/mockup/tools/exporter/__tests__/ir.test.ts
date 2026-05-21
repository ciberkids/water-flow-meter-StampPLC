import test from "node:test";
import assert from "node:assert/strict";
import { fileURLToPath } from "node:url";
import path from "node:path";
import fs from "node:fs/promises";
import { ensureValidDataset, ensureValidTheme } from "../schema.js";
import { buildIntermediateRepresentation, parseColorToArgb8888 } from "../ir.js";

async function readJson(relativePath: string) {
  const dir = path.dirname(fileURLToPath(import.meta.url));
  const projectRoot = path.resolve(dir, "..", "..", "..", "..", "..", "..");
  const absolute = path.resolve(projectRoot, relativePath);
  const raw = await fs.readFile(absolute, "utf-8");
  return JSON.parse(raw);
}

test("buildIntermediateRepresentation produces consistent counts", async () => {
  const dataset = ensureValidDataset(await readJson("web/mockup/src/data/screens.json"));
  const theme = ensureValidTheme(await readJson("web/mockup/src/data/themeTokens.json"));
  const ir = buildIntermediateRepresentation(dataset, theme);
  assert.equal(ir.screenCount, dataset.screens.length);
  const elementCount = dataset.screens.reduce((acc, screen) => acc + screen.elements.length, 0);
  assert.equal(ir.elementCount, elementCount);
  assert.ok(ir.theme.colors.displayBackground.argb8888 > 0);
});

test("parseColorToArgb8888 supports rgba() values with alpha", () => {
  const parsed = parseColorToArgb8888("rgba(124, 162, 206, 0.28)");
  assert.equal(parsed.source, "rgba(124, 162, 206, 0.28)");
  assert.equal(parsed.argb8888 >>> 24, 0x47);
});
