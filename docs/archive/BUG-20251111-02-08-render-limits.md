# Bugfix Story Template

> **File naming:** store each bugfix plan as `BUG-<story-id>-<slug>-bugfix.md` (example: `BUG-20250601-03-display-origin-bugfix.md`).

**Bug ID:** BUG-SI-20251111-02-08  
**Origin Story:** SI-20251111-02-05 (viewport axes & trigger fidelity)  
**Affected Area:** Live preview rendering (`web/mockup/src/components/DisplayViewport.tsx` / layout utils), keyboard bindings (`web/mockup/src/App.tsx`)  
**Reporter:** Codex  
**Review Date:** 2025-11-13  
**Fix Recommendation:** resolved

---

## 1. Bug Summary
- **Observed Behaviour:**  
  1. In the Design tab, moving text downward stops halfway through the preview; the live preview’s render area seems clipped so `(0,0)` maps correctly but vertical motion is truncated.  
  2. Keyboard axes remain swapped: pressing Up moves horizontally and Left/Right behave vertically.
- **Expected Behaviour:**  
  - The preview should allow full movement across the display bounds; elements should reach the bottom edge when X approaches the maximum.  
  - Keyboard arrows must align with their axes (Up increases vertical/X, Down decreases, Left decreases horizontal/Y, Right increases).
- **Repro Steps:**  
  1. Set an element’s X to a large value via keyboard arrows; movement stops near the middle (see described line).  
  2. Press arrow keys and note axis swap.

---

## 2. Test-First Plan

| Test ID | Test Type (unit/e2e/etc.) | Description | Status* |
|---------|---------------------------|-------------|---------|
| T1 | unit (`layout.test.ts`) | Assert preview/top offset reflects full vertical range (no mid clipping) | passing |
| T2 | e2e (`mockup.spec.ts`) | Verify keyboard arrows map to intended axes | passing |

> *Status: **pending** (not implemented), **failing** (written & reproduces bug), **passing** (after fix).

---

## 3. Root Cause Analysis
- **Suspected Module:** `DisplayViewport` height/scale or layout clamps may cap vertical offset; keyboard handler still uses swapped axes.
- **Hypothesis:** Preview container height or transformation is halving the available range; `getNudgeDelta` still misinterprets axes.
- **Evidence:** Manual interaction described; previous bugfix attempts only partially addressed the coordinate system.

---

## 4. Fix Outline
1. Write unit/e2e tests covering full vertical range and keyboard mapping.
2. Adjust preview layout/clamp math so elements can reach the bottom edge.
3. Fix keyboard arrow mapping to align with intended axes, ensuring UI button pad mirrors it.

---

## 5. Verification Matrix

| Check | Method | Result |
|-------|--------|--------|
| T1 | `npm run test:unit` | passing |
| T2 | `npm run test:visual -- tests/visual/mockup.spec.ts --grep 'element selection|keyboard'` | passing |
| Manual | Verified preview allows full vertical movement | passing |

---

## 6. Notes & Follow-ups
- **Risks:** Changing preview math could affect exporter alignment; need to ensure dataset/export remains unaffected.
- **Related Bugs:** BUG-SI-20251111-02-05, BUG-SI-20251111-02-06, BUG-SI-20251111-02-07.
- **Deferred Actions:** None.

---

Use this template to document bugfix work that is driven by failing tests and avoids unrelated feature updates.
