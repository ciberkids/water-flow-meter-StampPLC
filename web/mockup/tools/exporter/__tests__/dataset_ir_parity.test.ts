// Dataset -> IR parity: every screen and element the designer authored must survive
// translation, with its geometry and its binding intact.
//
// This replaces a Cypress e2e spec that checked the same property in a way that made it
// unusable. That spec ran the real exporter CLI with `--screens tests/fixtures/legacy-screens.json`
// and NO `--out` override, so every run overwrote the committed firmware assets in
// Water-Flow-Meter-PlatformIO/src/ui/generated with a build from an obsolete fixture, then read
// them back to compare. It also could not pass, and it was not in CI — so it corrupted the tree
// and told nobody anything.
//
// The property is worth keeping; the mechanism was not. buildIntermediateRepresentation is a
// pure function, so this runs it in memory against the REAL shipped dataset and writes nothing.
import test from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

import { buildIntermediateRepresentation } from "../ir.js";
import type { IRElementKind } from "../types.js";
import type { ScreenDataset } from "../../../src/types.js";
import type { ThemeTokens } from "../../../src/theme/types.js";

const here = path.dirname(fileURLToPath(import.meta.url));
const projectRoot = path.resolve(here, "..", "..", "..");

function loadShippedDataset(): ScreenDataset {
  const raw = fs.readFileSync(path.join(projectRoot, "src", "data", "screens.json"), "utf-8");
  return JSON.parse(raw) as ScreenDataset;
}

/**
 * The text an element carries, or undefined for the kinds that carry none.
 *
 * Narrows POSITIVELY. Narrowing negatively — "not a box, not an icon, therefore it has text" —
 * is how the first attempt at this file failed to compile: `scrollbar` was added to the union
 * later and its payload has no `text`, so the residual union no longer supported the access.
 * A positive switch means adding a kind is a compile error here rather than a silent gap.
 */
function textOf(kind: IRElementKind): string | undefined {
  switch (kind.type) {
    case "text":
    case "value":
    case "badge":
      return kind.payload.text;
    case "box":
    case "icon":
    case "scrollbar":
      return undefined;
  }
}

test("dataset -> IR parity", async (t) => {
  const dataset = loadShippedDataset();
  const theme = dataset.theme as ThemeTokens;
  const ir = buildIntermediateRepresentation(dataset, theme);

  await t.test("every authored screen appears in the IR, and none is invented", () => {
    const authored = dataset.screens.map((s: ScreenDataset["screens"][number]) => s.id).sort();
    const translated = ir.dataset.map((s) => s.id).sort();
    assert.deepEqual(translated, authored);
    assert.equal(ir.screenCount, dataset.screens.length);
  });

  await t.test("every element survives with its geometry and binding", () => {
    const mismatches: string[] = [];

    for (const screen of dataset.screens) {
      const irScreen = ir.dataset.find((s) => s.id === screen.id);
      if (!irScreen) {
        mismatches.push(`screen missing from IR: ${screen.id}`);
        continue;
      }
      for (const element of screen.elements) {
        const irElement = irScreen.elements.find((e) => e.id === element.id);
        if (!irElement) {
          mismatches.push(`${screen.id}/${element.id}: missing from IR`);
          continue;
        }
        if (irElement.position.x !== element.x || irElement.position.y !== element.y) {
          mismatches.push(
            `${screen.id}/${element.id}: position ${element.x},${element.y} -> ` +
              `${irElement.position.x},${irElement.position.y}`
          );
        }
        if (irElement.kind.type !== element.kind) {
          mismatches.push(`${screen.id}/${element.id}: kind ${element.kind} -> ${irElement.kind.type}`);
        }
        // A dropped binding is the failure that renders a blank element on the device, which
        // is the single most expensive class of bug this pipeline has produced.
        if ((element.binding ?? undefined) !== (irElement.binding ?? undefined)) {
          mismatches.push(
            `${screen.id}/${element.id}: binding ${element.binding ?? "(none)"} -> ` +
              `${irElement.binding ?? "(none)"}`
          );
        }
        const authoredText = element.content ?? undefined;
        const translatedText = textOf(irElement.kind);
        if (authoredText !== undefined && translatedText !== authoredText) {
          mismatches.push(
            `${screen.id}/${element.id}: text "${authoredText}" -> "${translatedText ?? "(none)"}"`
          );
        }
      }
    }

    assert.deepEqual(mismatches, [], `dataset and IR disagree:\n  ${mismatches.join("\n  ")}`);
  });

  await t.test("the element count is the sum of what was authored, not an estimate", () => {
    const authored = dataset.screens.reduce(
      (n: number, s: ScreenDataset["screens"][number]) => n + s.elements.length,
      0
    );
    assert.equal(ir.elementCount, authored);
  });

  await t.test("translation writes nothing — it is a pure function", () => {
    // The property that made the Cypress spec unusable. Running the translation twice must
    // produce identical output apart from the timestamp, and must not touch the filesystem.
    const again = buildIntermediateRepresentation(dataset, theme);
    const strip = (x: typeof ir) => JSON.stringify({ ...x, generatedAt: "" });
    assert.equal(strip(again), strip(ir));
  });
});
