#pragma once

namespace plc {

/**
 * Where the configuration portal's HTTP task is allowed to run (WiFi_MQTT_Connectivity §7.6).
 *
 * This header exists because `esp_http_server`'s defaults are actively wrong for this project and
 * nothing in the SDK will tell you so. From the installed header —
 * `framework-arduinoespressif32/tools/sdk/esp32s3/include/esp_http_server/include/esp_http_server.h`,
 * `HTTPD_DEFAULT_CONFIG()` at lines 26–49 — `httpd_start()` gets:
 *
 *     .task_priority = tskIDLE_PRIORITY + 5   →  5
 *     .core_id       = tskNO_AFFINITY
 *     .stack_size    = 4096
 *
 * Priority 5 is above the polling task's 2, and `tskNO_AFFINITY` lets the scheduler put the task on
 * whichever core is free — including the polling core. So the shipped defaults produce a task that
 * **will** preempt pulse polling, which R2.1.0 ("CORE 0 IS DEDICATED TO SENSOR POLLING AND NOTHING
 * ELSE") forbids outright.
 *
 * Both fields are runtime members of `httpd_config_t` (`unsigned task_priority`, `BaseType_t
 * core_id`), exactly like esp-mqtt's `task_prio` that R4.1.5 demotes to 1. So this is a fixable
 * problem, unlike lwIP's `tiT`, whose core is a literal compiled into `liblwip.a` (§2.1.3).
 *
 * ── Why a header of constants rather than three lines at the call site ────────────────
 *
 * §2.1.3's conclusion is "priority is the lever that matters, not affinity", and a lever nobody can
 * see is a lever that gets moved by accident. The numbers here are the entire defence of the
 * measurement R2.1.1 is acceptance-tested against, and they are two assignments deep in a function
 * that also does routing and authentication. Putting them here makes them reviewable, testable
 * without hardware, and — via the `static_assert`s below — impossible to break silently.
 *
 * ── Two distinct failure modes, not one ──────────────────────────────────────────────
 *
 * 1. **The measurement.** A task at priority ≥ 2 scheduled on the polling core delays
 *    `M5StamPLC.readPlcInput()`, and a pulse missed because the CPU was elsewhere is indistinguishable
 *    from a pulse that did not happen. R2.1.1 accepts a 5 % degradation; a priority-5 HTTP task
 *    servicing a form is not a 5 % effect.
 *
 * 2. **The watchdog.** `CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU0=y` with `CONFIG_ESP_TASK_WDT_PANIC=y`
 *    and a 5 s timeout means core 0's idle task MUST be scheduled. A busy priority-5 task on core 0
 *    starves it, and the result is a panic and a reboot — not a degraded reading. This is the stronger
 *    argument, and it is why the priority floor below is exclusive of the idle priority as well: the
 *    core-affinity review may swap the layout, and the portal must stay safe on the WDT-checked core
 *    too.
 *
 * Note also that §2.1.3 justifies tolerating `tiT` on core 0 partly because the high-traffic mode is
 * "the configuration portal (§7.6) … user-initiated, menu-gated, and time-boxed". **R7.9 superseded
 * that on the same day**: the web server is now active whenever WiFi is. The HTTP task is permanent,
 * so its priority and affinity are load-bearing in the steady state rather than for ten minutes.
 *
 * Arduino-free and `esp_http_server`-free on purpose, so it host-compiles (house pattern:
 * `net_settings.*`, `wifi_manager.*`). The two SDK values above are therefore *quoted* here, not
 * included — and the adapter must assign both fields unconditionally rather than rely on what the
 * default happens to be.
 */
namespace httpd_policy {

/**
 * `tskNO_AFFINITY` for THIS toolchain: `0x7FFFFFFF`, per
 * `sdk/esp32s3/include/freertos/include/freertos/task.h:70` (IDF v4.4.7).
 *
 * Spelled out because it is a portability trap: IDF 5.x redefines `tskNO_AFFINITY` to `-1`. Any
 * comparison written against -1 would silently stop matching here, and "the unpinned case is no
 * longer detected" is precisely the bug this header is meant to prevent.
 */
inline constexpr int kNoAffinity = 0x7FFFFFFF;

/** `tskIDLE_PRIORITY` (task.h:189). The idle task's priority, on both cores. */
inline constexpr unsigned kIdlePriority = 0;

/**
 * `configMAX_PRIORITIES` for this build: 25, per
 * `sdk/esp32s3/include/freertos/include/esp_additions/freertos/FreeRTOSConfig.h:81`.
 *
 * Here so the admissible band can be checked across the WHOLE scheduler range rather than across a
 * range chosen to make the answer come out right.
 */
inline constexpr unsigned kMaxPriorities = 25;

/**
 * The pulse-polling task's priority — the number every decision here is reasoning against.
 *
 * NOT derived: no core-layout header exists in this tree as of this slice, so `2` lives as a literal
 * at `xTaskCreatePinnedToCore(pollingTaskCode, "PollingTask", 4096, NULL, 2, &PollingTask, 0)` in
 * `firmware.cpp` and is duplicated here. That duplication is a defect of the same shape as the one
 * `mqtt_limits.h` was written to fix, and it should be resolved by making `firmware.cpp` read these
 * constants — not by adding a third copy.
 */
inline constexpr unsigned kPollingTaskPriority = 2;

/** The core the polling task is pinned to. Same provenance, same caveat, as the priority above. */
inline constexpr int kPollingTaskCore = 0;

/**
 * The eModbus RTU server's priority, pinned to core 1 alongside the logic task.
 *
 * Here because R2.1.4 requires network tasks to sit *below* it: a Modbus frame is delimited by a
 * 3.5-character silence, so starving the handler does not slow a frame down — it corrupts it.
 */
inline constexpr unsigned kModbusServerPriority = 8;

/** What `HTTPD_DEFAULT_CONFIG()` yields, kept so the hazard is asserted rather than described. */
inline constexpr unsigned kSdkDefaultTaskPriority = 5;
inline constexpr int kSdkDefaultCoreId = kNoAffinity;

/**
 * The other core. One line, and it is what makes this policy survive the layout swap the
 * core-affinity review is considering: nothing below names core 1 by number.
 *
 * `core` must stay read in the body — replacing this with `return 1;` is caught by
 * `-Wunused-parameter` under `-Werror` before any test gets a chance to notice.
 */
constexpr int otherCoreThan(int core) { return core == 0 ? 1 : 0; }

/**
 * Can a task at `priority` delay the polling loop, if the scheduler places it on the polling core?
 *
 * `>=`, not `>`, deliberately. A strictly higher priority preempts immediately; an EQUAL priority
 * round-robins with polling under `configUSE_TIME_SLICING`, which costs the polling loop roughly
 * half its scheduling opportunities. For a loop whose whole job is to observe edges in real time
 * those two outcomes differ in degree, not in kind, so both count as unsafe.
 *
 * Core-independent by design, which is §2.1.3's actual finding: an unpinned task may land on the
 * polling core, so priority alone has to be sufficient.
 *
 * `pollingPriority` is a parameter, not a read of the constant, so the tests can drive a layout the
 * firmware does not currently have. It MUST stay read in the body: `-Wunused-parameter` under
 * `-Werror` is what stops a future edit from quietly hardcoding today's layout back in.
 */
constexpr bool canDelayPolling(unsigned priority, unsigned pollingPriority = kPollingTaskPriority) {
  return priority >= pollingPriority;
}

/**
 * Does pinning to `core` violate R2.1.0's dedication of the polling core?
 *
 * `kNoAffinity` counts as a violation. "The scheduler is free to place it there" is not a defence —
 * it is the failure, deferred to whichever load pattern happens to trigger it, which is worse than a
 * deterministic one because it will not reproduce on a bench.
 *
 * Same rule as `canDelayPolling` about `pollingCore`: it must stay read in the body, and
 * `-Wunused-parameter` under `-Werror` is the enforcement. Substituting `kPollingTaskCore` for it
 * here would leave every check green while the policy stopped tracking the layout.
 */
constexpr bool violatesCoreDedication(int core, int pollingCore = kPollingTaskCore) {
  return core == pollingCore || core == kNoAffinity;
}

/**
 * The admissible priority band: strictly above idle, strictly below polling.
 *
 * Upper bound: `canDelayPolling`, above. Lower bound: the idle task of whichever core the portal
 * lands on must still be scheduled, and on a WDT-checked core (core 0 today) failing to schedule it
 * is a panic. A task AT the idle priority merely time-slices with idle instead of yielding to it.
 */
constexpr bool priorityIsAdmissible(unsigned priority,
                                    unsigned pollingPriority = kPollingTaskPriority) {
  return priority > kIdlePriority && !canDelayPolling(priority, pollingPriority);
}

/**
 * Does a given polling priority leave ANY legal priority for the HTTP task?
 *
 * With polling at 2 the band `(0, 2)` contains exactly one value, so this policy has no slack at
 * all. Lowering the polling priority to 1 would leave nothing admissible — the portal would have no
 * safe priority to run at, and that is a design decision, not a number to nudge. Asserted below so
 * it cannot be discovered on hardware.
 */
constexpr bool layoutAdmitsAnyPriority(unsigned pollingPriority) {
  return pollingPriority > kIdlePriority + 1;
}

/**
 * The priority `httpd_config_t.task_priority` must be set to: **1**.
 *
 * Below the polling task's 2 (R2.1.0, via §2.1.3's "priority is the lever") and far below the
 * priority-8 Modbus server (R2.1.4). Identical to the value R4.1.5 fixes the MQTT task at, and for
 * the same reason: at priority 1 the affinity question stops being load-bearing, so this number is
 * the defence that holds even if the pinning below is ever lost.
 */
inline constexpr unsigned kTaskPriority = 1;

/**
 * The core `httpd_config_t.core_id` must be set to — derived, never written as a literal.
 *
 * Pinning is belt-and-braces on top of the priority: it is what removes the watchdog failure mode
 * described above, which `kTaskPriority` alone does not fully (the WDT watches whether idle gets
 * scheduled, and priority 1 still round-robins with the priority-0 idle task).
 */
inline constexpr int kTaskCore = otherCoreThan(kPollingTaskCore);

// The point of the whole header. If someone raises the HTTP priority, or lowers the polling task's,
// this is a build failure rather than a field report of a device that reboots under load.
static_assert(kTaskPriority < kPollingTaskPriority,
              "The portal's HTTP task must run strictly below the pulse-polling task (R2.1.0, "
              "§2.1.3). Raising it reintroduces esp_http_server's default hazard.");
static_assert(kTaskPriority > kIdlePriority,
              "The HTTP task must not sit at the idle priority: on a watchdog-checked core that is "
              "CONFIG_ESP_TASK_WDT_PANIC territory.");
static_assert(kTaskPriority < kModbusServerPriority,
              "R2.1.4 — network tasks run below the Modbus handler, or a frame in progress gets a "
              "framing error rather than a delay.");
static_assert(layoutAdmitsAnyPriority(kPollingTaskPriority),
              "No priority is admissible under this core layout: the band between the idle priority "
              "and the polling priority is empty.");
static_assert(priorityIsAdmissible(kTaskPriority), "kTaskPriority must satisfy its own band.");
static_assert(!violatesCoreDedication(kTaskCore),
              "The HTTP task must be pinned off the polling core (R2.1.0).");

// And the defaults, asserted rather than merely described, so the reason this header exists is
// itself checked by the compiler.
static_assert(canDelayPolling(kSdkDefaultTaskPriority),
              "HTTPD_DEFAULT_CONFIG's priority is supposed to be unsafe here; if this ever fails, "
              "the quoted SDK value has drifted and the whole rationale needs re-reading.");
static_assert(violatesCoreDedication(kSdkDefaultCoreId),
              "HTTPD_DEFAULT_CONFIG's tskNO_AFFINITY is supposed to be unsafe here.");

}  // namespace httpd_policy

}  // namespace plc
