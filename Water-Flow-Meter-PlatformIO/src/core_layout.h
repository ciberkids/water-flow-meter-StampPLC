#pragma once

/**
 * WHERE EVERY TASK IN THIS FIRMWARE RUNS, AND AT WHAT PRIORITY. One definition, no copies.
 *
 * ── Why this file exists ─────────────────────────────────────────────────────────────
 *
 * `src/net/httpd_task_policy.h` used to carry its own copies of the polling task's core and
 * priority, transcribed by hand out of `firmware.cpp`, and then derive an entire placement policy
 * from them. Adversarial review found the hole that shape always has: mutating the copied
 * `kPollingTaskCore` from 0 to 1 left the whole 1283-check suite green, because the copy was the
 * only thing anything compared against. The header would have gone on asserting a conclusion drawn
 * from a fact that had stopped being true.
 *
 * That is the same defect `mqtt_limits.h` was written to fix — two definitions of one number, each
 * claiming to be the source of truth — and it is worse here, because the wrong answer is not a
 * dropped MQTT payload but a task scheduled on the core R2.1.0 reserves for pulse counting.
 *
 * So: the numbers live here once. `firmware.cpp` passes them to `xTaskCreatePinnedToCore`, and the
 * policy headers reason about them. Neither holds a copy.
 *
 * ── Scope: layout FACTS only ─────────────────────────────────────────────────────────
 *
 * Facts about what runs where. No policy, no predicates, no derived placements — those belong in the
 * per-subsystem policy header (`net/httpd_task_policy.h` for the §7.6 portal), which includes this
 * file. Kept at the top of `src/` rather than under `src/net/` deliberately: the polling task belongs
 * to the sensor path and the Modbus server to `src/modbus/`, so filing the whole layout under the
 * network subsystem would mean `firmware.cpp` including a *net* header to learn where its *sensor*
 * task runs. It is a whole-firmware fact and it sits where whole-firmware facts can be reached from
 * anywhere (`-I src` is on the host suite and the PlatformIO build alike).
 *
 * Arduino-free and FreeRTOS-free on purpose, so the host suite can compile it (house pattern:
 * `net_settings.*`, `mqtt_limits.h`). The FreeRTOS constants below are therefore *quoted* from the
 * installed toolchain with the file and line they came from, not `#include`d — every quote was
 * re-read against the installed SDK on 2026-08-05, and `test/host/httpd_task_policy_test.cpp` pins
 * them so a toolchain bump that changes one is a red test rather than a silent behaviour change.
 *
 * ── STATUS of the wiring, stated plainly ─────────────────────────────────────────────
 *
 * As of this file's introduction `firmware.cpp` still spells the polling and logic task numbers as
 * literals at the two `xTaskCreatePinnedToCore` calls; it does not yet `#include "core_layout.h"`.
 * Until it does, this header is one half of a pair whose halves could drift. That drift is not left
 * to review: `httpd_task_policy_test.cpp` reads `src/firmware.cpp` and fails unless each task's
 * priority and core argument either *names* the constant here or is a literal equal to it. So the
 * two agree today by test, and will agree by construction once the include lands.
 */

namespace plc {

/**
 * The scheduler's own vocabulary, quoted from the installed toolchain (arduino-esp32 2.0.17,
 * IDF v4.4.7, esp32-s3).
 *
 * These are here rather than in a policy header because they are facts about the scheduler this
 * layout is expressed in, and more than one subsystem needs them: the §7.6 portal's placement
 * policy today, the MQTT task's (R4.1.5) and the Modbus server's if they are ever given one.
 */
namespace core_layout {

/**
 * `tskNO_AFFINITY` for THIS toolchain: `0x7FFFFFFF`, per
 * `framework-arduinoespressif32/tools/sdk/esp32s3/include/freertos/include/freertos/task.h:70`.
 *
 * Spelled out because it is a portability trap, not for convenience: IDF 5.x redefines
 * `tskNO_AFFINITY` to `-1`. Any policy written against -1 would silently stop matching here, and
 * "the unpinned case is no longer detected" is exactly the failure that must not be able to hide.
 */
inline constexpr int kNoAffinity = 0x7FFFFFFF;

/** `tskIDLE_PRIORITY` (`task.h:189`, `( ( UBaseType_t ) 0U )`). The idle task's priority, both cores. */
inline constexpr unsigned kIdlePriority = 0;

/**
 * `configMAX_PRIORITIES` for this build: 25, per
 * `sdk/esp32s3/include/freertos/include/esp_additions/freertos/FreeRTOSConfig.h:81`.
 *
 * Here so a policy's admissible band can be checked across the WHOLE scheduler range rather than
 * across a range chosen to make the answer come out right.
 */
inline constexpr unsigned kMaxPriorities = 25;

/**
 * The other core.
 *
 * One line, and it is what lets a placement policy survive the layout swap the core-affinity review
 * is considering without naming core 1 by number anywhere.
 *
 * `core` must stay read in the body — replacing this with `return 1;` is caught by
 * `-Wunused-parameter` under `-Werror` before any test gets a chance to notice.
 */
constexpr int otherCoreThan(int core) { return core == 0 ? 1 : 0; }

// ── The tasks this firmware creates ────────────────────────────────────────────────

/**
 * The pulse-polling task: core 0, priority 2.
 *
 * `firmware.cpp`'s `xTaskCreatePinnedToCore(pollingTaskCode, "PollingTask", 4096, NULL, 2,
 * &PollingTask, 0)` — the arguments this pair replaces.
 *
 * Core 0 is R2.1.0: "CORE 0 IS DEDICATED TO SENSOR POLLING AND NOTHING ELSE", a standing invariant
 * that the requirement itself says outranks R2.1.1's 5 % measurement budget, because a pulse missed
 * while the CPU was elsewhere is indistinguishable from a pulse that did not happen.
 *
 * Priority 2 is the number §2.1.3 turns into the project's actual defence: "priority is the lever
 * that matters, not affinity" — any task at priority 1 cannot preempt polling *on either core*, so
 * an unpinned priority-1 task is harmless even when the scheduler puts it on core 0. Every network
 * task's priority in this firmware is chosen against this one (R4.1.5 fixes esp-mqtt's `task_prio`
 * at 1 for precisely that reason).
 */
inline constexpr int kPollingTaskCore = 0;
inline constexpr unsigned kPollingTaskPriority = 2;

/**
 * The logic/UI task: core 1, priority 1.
 *
 * `firmware.cpp`'s `xTaskCreatePinnedToCore(logicTaskCode, "LogicTask", 10000, NULL, 1, &LogicTask,
 * 1)`. Priority 1 is below the Modbus server's 8 on the same core, so a slow UI redraw cannot delay
 * a Modbus response (R2.1.4, R2.1.5) — and it is also what makes the Arduino `WebServer` option for
 * §7.6 safe, since `handleClient()` runs in whichever task calls it and this is the task that would.
 */
inline constexpr int kLogicTaskCore = 1;
inline constexpr unsigned kLogicTaskPriority = 1;

/**
 * The eModbus RTU server task: core 1, priority 8.
 *
 * Different provenance from the two above, and the difference matters. The CORE is this firmware's
 * choice — `modbus.begin(RS485_SERIAL_PORT, kModbusCoreId)` in `firmware.cpp`, without which
 * `ModbusServerRTU::doBegin()` would create the task with `tskNO_AFFINITY` and let it land on the
 * polling core. The PRIORITY is not ours to pick: it is the literal `8` at
 * `.pio/libdeps/m5stack-stamplc/eModbus/src/ModbusServerRTU.cpp:100-101`, in both the
 * `HAS_FREERTOS` and non-FreeRTOS branches, with no parameter reaching it.
 *
 * Recorded here because R2.1.4 makes it a ceiling every network task is measured against: "The WiFi
 * and MQTT tasks must run at a priority **below** the Modbus handler's, so a radio event can never
 * delay a frame in progress". A Modbus frame is delimited by a 3.5-character silence, so starving
 * the handler does not slow a frame down — it corrupts it, and the master sees a framing error and
 * retries (R2.1.5 requires that retry rate be measured, because it is invisible to any test that
 * only watches `pollingRate_kHz`).
 */
inline constexpr int kModbusServerCore = 1;
inline constexpr unsigned kModbusServerPriority = 8;

/**
 * The WiFi driver task (`pp`): core 1, priority 23 — moved there by a build flag, not by us.
 *
 * `platformio.ini` defines `-DCONFIG_ESP32_WIFI_TASK_PINNED_TO_CORE_1`, which flips what
 * `WIFI_INIT_CONFIG_DEFAULT()` writes into `wifi_init_config_t::wifi_task_core_id` (§2.1.3, §11 Q1).
 * `firmware.cpp:22` build-asserts `WIFI_TASK_CORE_ID == 1` — that assert has to live there because
 * the macro needs `esp_wifi.h`, which this header deliberately cannot include. So this constant is
 * the host-visible statement of the same fact, and the authoritative check stays in `firmware.cpp`.
 *
 * Not the whole story, and §2.1.3 is blunt about it: lwIP's `tiT` is priority 18 on core 0 and
 * CANNOT be moved — its core is a literal `movi.n a7, 0` compiled into `liblwip.a`. R2.1.0 therefore
 * cannot be fully satisfied under `framework = arduino`. Nothing in this header pretends otherwise;
 * what it does is keep every task we DO control off the polling core.
 */
inline constexpr int kWifiTaskCore = 1;

// ── The invariants of the layout itself, as build failures ────────────────────────
//
// A comment saying "core 0 is only for polling" is worth nothing the day someone changes a number.

static_assert(kLogicTaskCore != kPollingTaskCore,
              "R2.1.0 — nothing but pulse polling may be scheduled on the polling core.");
static_assert(kModbusServerCore != kPollingTaskCore,
              "R2.1.0 — the Modbus server must stay off the polling core; without the explicit "
              "coreID eModbus would use tskNO_AFFINITY and be free to land there.");
static_assert(kWifiTaskCore != kPollingTaskCore,
              "R2.1.0, §2.1.3 — the WiFi task is moved off the polling core by "
              "-DCONFIG_ESP32_WIFI_TASK_PINNED_TO_CORE_1 in platformio.ini.");
static_assert(kLogicTaskPriority < kModbusServerPriority,
              "R2.1.4/R2.1.5 — Modbus must preempt the logic task on their shared core, or a slow "
              "UI redraw turns into a framing error on the wire.");
static_assert(kPollingTaskPriority < kModbusServerPriority,
              "The layout assumes Modbus outranks polling on the core it does not share with it; "
              "if this ever inverts, §2.1.3's 'priority is the lever' reasoning needs re-deriving.");
static_assert(kPollingTaskPriority > kIdlePriority + 1,
              "Polling must leave at least one usable priority beneath it: every network task in "
              "this firmware (R4.1.5, and the §7.6 portal) is placed strictly below it, and at "
              "polling priority 1 there is nowhere left to put them but the idle priority.");
static_assert(kPollingTaskPriority < kMaxPriorities && kModbusServerPriority < kMaxPriorities,
              "A priority at or above configMAX_PRIORITIES is silently clamped by FreeRTOS.");

}  // namespace core_layout

}  // namespace plc
