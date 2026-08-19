import test from "node:test";
import assert from "node:assert/strict";
import { checkRenderableElementKinds, checkRingClosure, runExportValidations } from "../validation.js";
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


/**
 * J1: the ring-closure gate, one test per failure mode.
 *
 * Each of these is a pack that looks fine on inspection — every flow has a target, every screen exists —
 * and strands the operator on the device, because paging wraps in the DATASET and `UiNavigator` has no
 * modulo arithmetic to fall back on. A three-screen ring is the smallest shape that can be broken in all
 * four ways.
 */
function ringDataset(): ScreenDataset {
  const page = (id: string, next: string, previous: string) => ({
    id,
    name: id,
    elements: [{ id: `${id}-title`, kind: "text" as const, x: 0, y: 0, content: id }],
    flows: [
      {
        id: `${id}-next`,
        label: "Next page",
        trigger: { type: "button" as const, button: "down" as const, gesture: "short" as const },
        actionId: "ui.action.page.next",
        targetScreenId: next
      },
      {
        id: `${id}-prev`,
        label: "Previous page",
        trigger: { type: "button" as const, button: "up" as const, gesture: "short" as const },
        actionId: "ui.action.page.previous",
        targetScreenId: previous
      }
    ]
  });
  return {
    screens: [page("p0", "p1", "p2"), page("p1", "p2", "p0"), page("p2", "p0", "p1")],
    theme: cloneTheme(defaultTheme)
  } satisfies ScreenDataset;
}

test("checkRingClosure passes on a ring that closes both ways", () => {
  const check = checkRingClosure(ringDataset());
  assert.equal(check.status, "pass");
  assert.match(check.message, /1 ring\(s\) covering 3 screen\(s\)/);
});

test("checkRingClosure fails on a DOWN target that is not a screen", () => {
  const dataset = ringDataset();
  dataset.screens[2].flows![0].targetScreenId = "p3-that-was-never-authored";
  const check = checkRingClosure(dataset);
  assert.equal(check.status, "fail");
  assert.match(check.recommendation ?? "", /not a screen in this dataset/);
});

test("checkRingClosure fails on a dead end — a member that pages no further", () => {
  const dataset = ringDataset();
  // p2 keeps its UP and loses its DOWN: the ring runs p0 -> p1 -> p2 and stops.
  dataset.screens[2].flows = dataset.screens[2].flows!.filter(
    (flow) => flow.actionId !== "ui.action.page.next"
  );
  const check = checkRingClosure(dataset);
  assert.equal(check.status, "fail");
  assert.match(check.recommendation ?? "", /dead end|pages UP but has no DOWN/);
});

test("checkRingClosure fails when the ring closes on a member further along", () => {
  const dataset = ringDataset();
  // p2's DOWN returns to p1 instead of p0, so p0 is unreachable once you leave it — the "points back
  // into the middle" case, which every individual flow still satisfies.
  dataset.screens[2].flows![0].targetScreenId = "p1";
  const check = checkRingClosure(dataset);
  assert.equal(check.status, "fail");
  assert.match(check.recommendation ?? "", /closes on a member further along|inverse of DOWN/);
});

test("checkRingClosure fails when UP is not the inverse of DOWN", () => {
  const dataset = ringDataset();
  // Every screen still pages both ways and every target exists; only the pairing is wrong.
  dataset.screens[1].flows![1].targetScreenId = "p1";
  const check = checkRingClosure(dataset);
  assert.equal(check.status, "fail");
  assert.match(check.recommendation ?? "", /UP must be the exact inverse of DOWN/);
});

test("checkRingClosure fails on a screen that pages DOWN but not UP", () => {
  const dataset = ringDataset();
  dataset.screens[1].flows = dataset.screens[1].flows!.filter(
    (flow) => flow.actionId !== "ui.action.page.previous"
  );
  const check = checkRingClosure(dataset);
  assert.equal(check.status, "fail");
  assert.match(check.recommendation ?? "", /no UP flow/);
});

test("checkRingClosure ignores screens with no paging at all", () => {
  const dataset = ringDataset();
  // An editor: entered by ENTER, left by hold, and part of no ring. Demanding one of these would fail the
  // shipped dataset for being correct — twenty of its eighty screens are this shape.
  dataset.screens.push({
    id: "editor",
    name: "editor",
    elements: [{ id: "editor-title", kind: "text", x: 0, y: 0, content: "editor" }]
  });
  const check = checkRingClosure(dataset);
  assert.equal(check.status, "pass");
});
