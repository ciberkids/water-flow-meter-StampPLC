import test from "node:test";
import assert from "node:assert/strict";
import { checkRenderableElementKinds, runExportValidations } from "../validation.js";
import { buildIntermediateRepresentation } from "../ir.js";
import type { ScreenDataset, ScreenElement } from "../../../src/types.js";
import { cloneTheme } from "../../../src/theme/types.js";
import { defaultTheme } from "../../../src/theme/defaultTheme.js";
import type { FirmwareManifest } from "../../../shared/schemaDefinitions.js";

/**
 * Manifest covering exactly the actions and bindings used by makeDataset().
 * The coverage checks are strict now (no `ui.`/`core.` prefix exemption), so
 * every test that expects a clean pass has to supply one.
 */
function makeManifest(): FirmwareManifest {
  return {
    version: "1",
    actions: [{ id: "ui.action.page.next", label: "Next page" }],
    values: [
      { id: "legend.led", type: "string" },
      { id: "telemetry.status", type: "string" },
      { id: "countdown.value", type: "string" }
    ],
    screens: [
      { id: "info", role: "info-page-0" },
      { id: "countdown", role: "countdown-overlay" }
    ]
  };
}

function makeDataset(): ScreenDataset {
  return {
    screens: [
      {
        id: "info",
        name: "Info",
        elements: [
          {
            id: "legend-led",
            kind: "text",
            x: 0,
            y: 0,
            content: "Legend",
            binding: "legend.led"
          },
          {
            id: "diagnostics-badge",
            kind: "badge",
            x: 0,
            y: 12,
            content: "Diagnostics",
            binding: "telemetry.status"
          }
        ]
      },
      {
        id: "countdown",
        name: "Countdown",
        elements: [
          {
            id: "timer-value",
            kind: "value",
            x: 10,
            y: 20,
            content: "30 s",
            binding: "countdown.value"
          }
        ],
        // The confirm-screen check is semantic now: a hold countdown is a timeout
        // trigger carrying holdButton, not a screen whose id says "countdown".
        flows: [
          {
            id: "f-confirm",
            label: "Confirm",
            trigger: { type: "timeout", durationMs: 3000, holdButton: "enter" },
            actionId: "ui.action.page.next"
          }
        ]
      }
    ],
    theme: cloneTheme(defaultTheme)
  } satisfies ScreenDataset;
}

test("runExportValidations passes when anchors exist", () => {
  const dataset = makeDataset();
  const ir = buildIntermediateRepresentation(dataset, dataset.theme);
  const report = runExportValidations(dataset, ir, makeManifest());
  assert.equal(report.status, "pass");
  assert.equal(report.issues.length, 0);
});

test("runExportValidations fails without LED legend", () => {
  const dataset = makeDataset();
  dataset.screens[0].elements = dataset.screens[0].elements.filter(
    (element) => element.binding !== "legend.led"
  );
  const ir = buildIntermediateRepresentation(dataset, dataset.theme);
  const report = runExportValidations(dataset, ir, makeManifest());
  assert.equal(report.status, "fail");
  assert.ok(report.issues.some((issue) => issue.includes("legend")));
});

test("runExportValidations fails without countdown screen", () => {
  const dataset = makeDataset();
  dataset.screens = dataset.screens.filter((screen) => screen.id !== "countdown");
  const ir = buildIntermediateRepresentation(dataset, dataset.theme);
  const report = runExportValidations(dataset, ir, makeManifest());
  assert.equal(report.status, "fail");
  assert.ok(report.issues.some((issue) => issue.includes("countdown")));
});

test("runExportValidations fails when no manifest is supplied", () => {
  const dataset = makeDataset();
  const ir = buildIntermediateRepresentation(dataset, dataset.theme);
  const report = runExportValidations(dataset, ir);
  assert.equal(report.status, "fail");
  assert.ok(report.issues.some((issue) => issue.includes("manifest")));
});

test("runExportValidations fails on an unknown binding", () => {
  const dataset = makeDataset();
  dataset.screens[0].elements[0].binding = "legend.does-not-exist";
  const ir = buildIntermediateRepresentation(dataset, dataset.theme);
  const report = runExportValidations(dataset, ir, makeManifest());
  assert.equal(report.status, "fail");
  const check = report.checks.find((entry) => entry.id === "manifest-value-coverage");
  assert.equal(check?.status, "fail");
  assert.match(check?.recommendation ?? "", /legend\.does-not-exist/);
});

test("runExportValidations fails when a firmware-required screen is renamed", () => {
  // The failure mode that produced a blank display: renaming a screen the router
  // resolves by name passes every other check and compiles cleanly.
  const dataset = makeDataset();
  dataset.screens[1].id = "countdown-renamed";
  const ir = buildIntermediateRepresentation(dataset, dataset.theme);
  const report = runExportValidations(dataset, ir, makeManifest());
  assert.equal(report.status, "fail");
  const check = report.checks.find((entry) => entry.id === "manifest-screen-coverage");
  assert.equal(check?.status, "fail");
  assert.match(check?.recommendation ?? "", /countdown \(countdown-overlay\)/);
});

test("runExportValidations fails when the manifest declares no required screens", () => {
  const dataset = makeDataset();
  const ir = buildIntermediateRepresentation(dataset, dataset.theme);
  const { screens: _screens, ...manifest } = makeManifest();
  const report = runExportValidations(dataset, ir, manifest);
  assert.equal(report.status, "fail");
  const check = report.checks.find((entry) => entry.id === "manifest-screen-coverage");
  assert.equal(check?.status, "fail");
});

test("checkRenderableElementKinds rejects kinds firmware cannot render", () => {
  const dataset = makeDataset();
  dataset.screens[0].elements.push({
    id: "flow-animation",
    // Deliberately not an ElementKind: the guard must reject anything without an
    // IR mapping, including a kind added to the union but not yet plumbed through.
    kind: "animation" as unknown as ScreenElement["kind"],
    x: 0,
    y: 40,
    width: 50,
    height: 24
  });
  // Deliberately not going through buildIntermediateRepresentation: the CLI runs
  // this check first precisely because the IR converter throws on these kinds.
  const check = checkRenderableElementKinds(dataset);
  assert.equal(check.status, "fail");
  assert.match(check.recommendation ?? "", /flow-animation/);
});

test("checkRenderableElementKinds passes on a firmware-renderable dataset", () => {
  const check = checkRenderableElementKinds(makeDataset());
  assert.equal(check.status, "pass");
});

test("buildIntermediateRepresentation throws on an unmappable element kind", () => {
  const dataset = makeDataset();
  dataset.screens[0].elements.push({
    id: "flow-mystery",
    kind: "mystery" as unknown as ScreenElement["kind"],
    x: 0,
    y: 60,
    width: 10,
    height: 80
  });
  assert.throws(
    () => buildIntermediateRepresentation(dataset, dataset.theme),
    /no firmware IR mapping/
  );
});
