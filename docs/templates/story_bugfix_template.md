# Bugfix Story Template

> **File naming:** store each bugfix plan as `BUG-<story-id>-<slug>-bugfix.md` (example: `BUG-20250601-03-display-origin-bugfix.md`).

**Bug ID:** <!-- e.g., BUG-20250601-03 -->  
**Origin Story:** <!-- link back to the original SI number / requirement -->  
**Affected Area:** <!-- component(s) or file(s) exhibiting the bug -->  
**Reporter:** <!-- name -->  
**Review Date:** <!-- YYYY-MM-DD -->  
**Fix Recommendation:** <!-- ready for fix | needs investigation -->

---

## 1. Bug Summary
- **Observed Behaviour:** <!-- what happens today -->
- **Expected Behaviour:** <!-- requirement/spec reference -->
- **Repro Steps:** <!-- numbered list or concise steps to reproduce -->

---

## 2. Test-First Plan

> Describe the failing test you will write **before** touching implementation.

| Test ID | Test Type (unit/e2e/etc.) | Description | Status* |
|---------|---------------------------|-------------|---------|
| T1 | <!-- unit/e2e/etc. --> | <!-- what the test asserts --> | <!-- pending/failing/passing --> |
| T2 |  |  |  |

> *Status: **pending** (not implemented), **failing** (written and reproduces bug), **passing** (after fix).

---

## 3. Root Cause Analysis
- **Suspected Module:** <!-- file or subsystem -->
- **Hypothesis:** <!-- what is wrong internally -->
- **Evidence:** <!-- logs, test output, code refs -->

---

## 4. Fix Outline
1. <!-- Step 1: e.g., update clamp logic in `foo.ts` -->
2. <!-- Step 2: adjust schema, etc. -->
3. <!-- Step 3: clean-up / docs -->

> Keep this scoped strictly to eliminating the defect. No feature expansion.

---

## 5. Verification Matrix

| Check | Method | Result |
|-------|--------|--------|
| Original failing test | <!-- e.g., `npm run test:unit` T1 --> | <!-- pass/fail --> |
| Regression tests | <!-- e.g., `npm run test:visual` subset --> |  |
| Manual confirmation | <!-- short description --> |  |

---

## 6. Notes & Follow-ups
- **Risks:** <!-- regressions to watch for -->
- **Related Bugs:** <!-- ids -->
- **Deferred Actions:** <!-- anything intentionally postponed -->

---

Use this template to document bugfix work that is driven by failing tests and avoids unrelated feature updates.
