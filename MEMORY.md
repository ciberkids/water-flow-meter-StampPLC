# Project Memory & Status (Session Handoff)

**Date Updated:** May 22, 2026

## 1. Current Project Status
The repository has been fully realigned and cleaned up. We've successfully completed the major architectural push to align the Web Mockup (React/Node exporter) with the C++ Firmware expectations. All work has been merged and migrated from the `master` branch to the primary `main` branch. 

### Key Achievements this Session:
* **UI/Firmware Alignment:** The `screens.json` dataset strictly complies with the C++ firmware element types. Unsupported types (`animation`, `scrollbar`) were purged from the TypeScript exporter.
* **Global Status Screen (P0):** We added a new `info-p0-global-status` screen to serve as the initial landing page. It displays aggregate metrics (`Total Flow L/s` and `Total Volume L`).
* **Animated Flow Dots:** The bulky UI propeller design was scrapped. Instead, we implemented `drawFlowDots()` directly in the C++ hardware renderer (`ui_renderer.cpp`). The hardware dynamically alternates a pair of blue dots on the P0 screen at a frequency proportional to the system's `aggregateFlowLps`.
* **Cleaned 8-Sensor Pages:** By moving the global metrics to P0, the P1–P6 pages now beautifully fit the 8 sensors into the 135x240 display without overlapping constraints.
* **Tests Passing:** Restored the `node:test` suite for the exporter. All 21 tests pass, validating the new schema and UI design.
* **Branch Cleanup:** Unused `master` branches have been permanently deleted locally and remotely.

## 2. Firmware UI Architecture (How it works)
For future reference on how UI actions trigger firmware code:
1. **Definition:** Actions are defined in the web UI as strings inside `screens.json` (e.g., `"ui.action.page.next"`).
2. **Translation:** `npm run export:firmware` bakes these strings into `GeneratedUi.h`.
3. **Hardware Dispatch:** When a physical button is pressed, `interaction_handler.cpp` detects the gesture, matches it against the generated UI table, and routes the string to the `UiActionRegistry::dispatch()` method.
4. **C++ Callbacks:** `ui_actions.cpp` maps the string IDs to C++ function pointers (e.g., `controller.nextPage(nowMs)`).

## 3. Next Steps / Pending Work
For the next session, consider picking up the following:

* **CI/CD Pipeline Setup:** We currently have no automated GitHub Actions. The repository needs a `.github/workflows/ci.yml` file to automatically run `npm run test:exporter` and `pio run -e m5stack-stamplc` on every push to `main` to prevent regressions.
* **Hardware Validation:** Deploy the newly compiled firmware onto the physical M5Stack StampPLC hardware to verify that the dynamic flow dots render clearly and at the correct visual speeds on the actual LCD.
* **Beads Issue Tracking:** Check `bd ready` for any outstanding tasks, and remember to `bd sync` after creating or closing work!

## 4. Work Rules & Handoff
As always, remember to "Land the Plane" at the end of every session. Work is only considered complete once `git pull --rebase`, `bd sync`, and `git push` successfully execute.
