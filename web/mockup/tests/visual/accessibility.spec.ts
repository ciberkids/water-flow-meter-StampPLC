import { expect, test, type Page } from "@playwright/test";

/**
 * The accessibility gate the workspace did not have (J6).
 *
 * `SI-20251111-04` listed "accessibility verified" among its acceptance criteria and marked it not
 * delivered, and it was right: no `axe`, `jest-axe`, `pa11y` or `lighthouse` appears in `package.json`, so
 * the hand-written `aria-*` attributes scattered through the workspace were checked by nothing. That is the
 * dangerous half of an accessibility claim — attributes that LOOK right, verified by nobody, in a tool whose
 * own Help tab documents them.
 *
 * WHAT THIS CHECKS, and it is deliberately a short list of things that can be decided mechanically:
 *
 *  1. every interactive control on every tab has an accessible name;
 *  2. no duplicate `id` on a page, since a duplicate silently breaks `label[for]` and `aria-labelledby`;
 *  3. no `label[for]` pointing at an element that does not exist — a label that names nothing;
 *  4. every `<img>` carries `alt`.
 *
 * The first one found fifteen unnamed controls when it was written: the trace filter (a `placeholder` is not
 * a name — it is announced inconsistently and vanishes as soon as anything is typed), ten
 * element-selection radios (a `data-testid` names a control for TESTS, never for a reader), and three file
 * inputs, two of them hidden behind buttons. All fifteen are named now, which is what makes this file a gate
 * rather than a report.
 *
 * WHAT IT DOES NOT CHECK, stated so the coverage is not overclaimed:
 *
 *  - Contrast, focus order, and the ARIA rules that need a real engine. That is an `axe` pass, it needs a
 *    dependency this project has not taken, and choosing one is the owner's call rather than a side effect
 *    of a test file.
 *  - Landmarks and heading structure. There is no `<h1>` on any tab — a genuine finding, left alone because
 *    fixing it changes document structure and would move all 21 workspace baselines, which is its own
 *    round. It is recorded in J6 rather than silently checked or silently ignored.
 *
 * AND IT IS NOT YET A CI GATE. Nothing in `.github/workflows/` runs `test:visual` (DF21), so this file is
 * exactly as fresh as the last person to run it locally. That is the whole of DF21 and is not fixed here.
 */

const TABS = ["Simulation", "Design", "Import & Export", "Help & Documentation"] as const;

type A11yReport = {
  controls: number;
  unnamed: string[];
  duplicateIds: string[];
  orphanLabels: string[];
  imagesWithoutAlt: number;
};

/**
 * The accessible-name check, run in the page.
 *
 * Deliberately a SUBSET of the real accessible-name computation: aria-label, aria-labelledby, a `label[for]`
 * or wrapping `<label>`, `title`, or the control's own text. A control satisfying none of those has no name
 * under any reading of the spec, so a violation here is unambiguous — which is the property that lets this
 * fail a build.
 */
async function auditTab(page: Page): Promise<A11yReport> {
  return page.evaluate(() => {
    const named = (element: Element): boolean => {
      const el = element as HTMLElement;
      const aria = el.getAttribute("aria-label");
      if (aria && aria.trim()) return true;
      const labelledBy = el.getAttribute("aria-labelledby");
      if (labelledBy && labelledBy.split(/\s+/).some((id) => document.getElementById(id))) return true;
      const id = el.getAttribute("id");
      if (id && document.querySelector(`label[for="${CSS.escape(id)}"]`)) return true;
      if (el.closest("label")) return true;
      const title = el.getAttribute("title");
      if (title && title.trim()) return true;
      return Boolean((el.textContent || "").trim());
    };

    const describe = (element: Element) => {
      const el = element as HTMLElement;
      const type = el.getAttribute("type");
      const testId = el.getAttribute("data-testid");
      return `${el.tagName.toLowerCase()}${type ? `[${type}]` : ""}${testId ? ` (${testId})` : ""}`;
    };

    const controls = Array.from(
      document.querySelectorAll("input,select,textarea,button,[role=button]")
    );
    const ids = Array.from(document.querySelectorAll("[id]")).map((el) => el.id);

    return {
      controls: controls.length,
      unnamed: controls.filter((control) => !named(control)).map(describe),
      duplicateIds: [...new Set(ids.filter((id, index) => ids.indexOf(id) !== index))],
      orphanLabels: Array.from(document.querySelectorAll("label[for]"))
        .filter((label) => !document.getElementById((label as HTMLLabelElement).htmlFor))
        .map((label) => (label as HTMLLabelElement).htmlFor),
      imagesWithoutAlt: document.querySelectorAll("img:not([alt])").length
    };
  });
}

test.describe("every workspace tab is navigable by name", () => {
  for (const tab of TABS) {
    test(`${tab}: every control has an accessible name, and no id is duplicated`, async ({ page }) => {
      await page.setViewportSize({ width: 1440, height: 900 });
      await page.goto("/");
      await page.getByRole("button", { name: tab, exact: true }).click();
      // The tab's panel mounts synchronously, but the values panel fills from a first loop tick.
      await page.waitForTimeout(150);

      const report = await auditTab(page);

      // A tab with no controls means the click did not land, and every assertion below would pass vacuously.
      expect(report.controls).toBeGreaterThan(5);
      expect(report.unnamed, `unnamed controls on ${tab}`).toEqual([]);
      expect(report.duplicateIds, `duplicate ids on ${tab}`).toEqual([]);
      expect(report.orphanLabels, `label[for] naming nothing on ${tab}`).toEqual([]);
      expect(report.imagesWithoutAlt, `images without alt on ${tab}`).toBe(0);
    });
  }
});
