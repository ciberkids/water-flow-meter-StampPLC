#pragma once

// kPollingTaskCore / kPollingTaskPriority and the scheduler's constants — the ONE definition. This
// header used to carry its own transcribed copies; see core_layout.h for what that cost.
#include "core_layout.h"

namespace plc {

/**
 * ═══════════════════════════════════════════════════════════════════════════════════════
 *  STATUS: NOTHING IN THIS FIRMWARE CREATES AN HTTP TASK YET. THIS FILE IS NOT WIRING.
 * ═══════════════════════════════════════════════════════════════════════════════════════
 *
 * Read that before anything below. `esp_http_server` appears nowhere in `src/`, no `httpd_config_t`
 * is constructed anywhere, and no `httpd_start()` is called. Adversarial review raised exactly this
 * and it was right to: a header that conditions the placement of a task nobody creates reads, at a
 * glance, like a subsystem that exists.
 *
 * What this file IS: a recorded fact about a shipped SDK default that is actively wrong for this
 * device, plus the policy derived from that fact, written down before the code that will consume it
 * so that the code cannot be written the easy way by accident. The §7.6 configuration portal is the
 * slice that will consume it (N8a — `portal_form.*`, the form generation and parsing half, is already
 * built and tested; the socket half is not).
 *
 * ── And a second honesty note, about WHICH web stack §7.6 will use ───────────────────
 *
 * The requirement's own plan is **not** `esp_http_server`. §3.3 and the N8a slice row both name
 * Arduino's bundled `WebServer` + `DNSServer` ("ship **bundled** with arduino-esp32 2.0.17
 * (verified), so the portal costs no dependency at all"), and `portal_form.h` is written against that
 * choice. Verified in the installed libraries on 2026-08-05: neither
 * `framework-arduinoespressif32/libraries/WebServer/src/` nor `.../DNSServer/src/` contains a single
 * `xTaskCreate*` call — `WebServer::handleClient()` (`WebServer.cpp:275`) runs to completion in
 * whichever task calls it. So on the planned stack **there is no HTTP task, no `httpd_config_t`, and
 * neither field below is ever assigned.**
 *
 * That does not make this file dead, and it is worth being precise about why:
 *
 *  - If the portal is pumped from `logicTaskCode`, its HTTP work inherits that task's placement —
 *    `core_layout::kLogicTaskCore` / `kLogicTaskPriority`, i.e. core 1 at priority 1. Which is
 *    exactly `kTaskCore` / `kTaskPriority` below. The policy is already satisfied, by construction
 *    rather than by configuration, and the numbers here are what say so.
 *  - `esp_http_server` remains the live alternative the moment the portal needs concurrent sockets
 *    (R7.9 made the server permanent rather than a ten-minute window, and R7.14 added a live status
 *    view), and that is the case where the defaults below bite. Recording the hazard now costs a
 *    header; rediscovering it costs a field report of a device that reboots under load.
 *
 * A reader must not take anything below as evidence of wiring. It is a fact and a conclusion, waiting
 * for a consumer.
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════
 *  VERIFIED FACT — `HTTPD_DEFAULT_CONFIG()` is unsafe on this device
 * ═══════════════════════════════════════════════════════════════════════════════════════
 *
 * Re-read on 2026-08-05 in the installed header,
 * `framework-arduinoespressif32/tools/sdk/esp32s3/include/esp_http_server/include/esp_http_server.h`.
 * `HTTPD_DEFAULT_CONFIG()` spans lines 26–48 and gives `httpd_start()`:
 *
 *     esp_http_server.h:27    .task_priority = tskIDLE_PRIORITY+5    →  5
 *     esp_http_server.h:28    .stack_size    = 4096
 *     esp_http_server.h:29    .core_id       = tskNO_AFFINITY        →  0x7FFFFFFF (task.h:70)
 *
 * Priority 5 is above the polling task's 2, and `tskNO_AFFINITY` lets the scheduler put the task on
 * whichever core is free — including the polling core. So the shipped defaults produce a task that
 * **will** preempt pulse polling, which R2.1.0 ("CORE 0 IS DEDICATED TO SENSOR POLLING AND NOTHING
 * ELSE") forbids outright. Nothing in the SDK warns about this; the struct is zero-initialised from a
 * macro and the numbers look like housekeeping.
 *
 * Both fields are runtime members of `httpd_config_t` (`unsigned task_priority` at
 * esp_http_server.h:137, `BaseType_t core_id` at :139), exactly like esp-mqtt's `task_prio` that
 * R4.1.5 fixes at 1. So this is a fixable problem, unlike lwIP's `tiT`, whose core is a literal
 * compiled into `liblwip.a` (§2.1.3).
 *
 * ── Two distinct failure modes, not one ──────────────────────────────────────────────
 *
 * 1. **The measurement.** A task at priority ≥ 2 scheduled on the polling core delays
 *    `M5StamPLC.readPlcInput()`, and a pulse missed because the CPU was elsewhere is
 *    indistinguishable from a pulse that did not happen. R2.1.1 accepts a 5 % degradation; a
 *    priority-5 HTTP task servicing a form is not a 5 % effect.
 *
 * 2. **The watchdog.** Re-verified 2026-08-05 in the shipped `sdkconfig.h` (`tools/sdk/esp32s3/
 *    <flash-variant>/include/sdkconfig.h`, lines 389–392, and all six flash variants agree):
 *
 *        CONFIG_ESP_TASK_WDT               1
 *        CONFIG_ESP_TASK_WDT_PANIC         1
 *        CONFIG_ESP_TASK_WDT_TIMEOUT_S     5
 *        CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU0  1
 *
 *    So core 0's idle task MUST be scheduled within 5 s, and failing that is a panic and a reboot —
 *    not a degraded reading. A busy priority-5 task on core 0 starves it. This is the stronger of the
 *    two arguments, and it is why the priority floor below excludes the idle priority as well.
 *
 *    Note what is NOT set: `CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU1` is absent from all six
 *    variants, so the watched core is core 0 and stays core 0 — it is fixed by sdkconfig, not by
 *    which core the polling task uses. That matters for the layout swap the core-affinity review is
 *    contemplating: move polling to core 1 and the portal lands on core 0, which is the WDT-checked
 *    one. The idle floor has to hold on either core, which is why it is not conditional on the
 *    layout.
 *
 * Note also that §2.1.3 justifies tolerating `tiT` on core 0 partly because the high-traffic mode is
 * "the configuration portal (§7.6) … user-initiated, menu-gated, and time-boxed". **R7.9 superseded
 * that on the same day**: the web server is now active whenever WiFi is. The HTTP task is permanent,
 * so its priority and affinity are load-bearing in the steady state rather than for ten minutes.
 *
 * ── Why a header of constants rather than three lines at the call site ────────────────
 *
 * §2.1.3's conclusion is "priority is the lever that matters, not affinity", and a lever nobody can
 * see is a lever that gets moved by accident. The numbers here are the entire defence of the
 * measurement R2.1.1 is acceptance-tested against, and at the call site they would be two assignments
 * deep in a function that also does routing and authentication. Here they are reviewable, testable
 * without hardware, and — via the `static_assert`s below — impossible to break silently.
 *
 * Arduino-free and `esp_http_server`-free on purpose, so it host-compiles (house pattern:
 * `net_settings.*`, `wifi_manager.*`). The SDK values above are therefore *quoted* here, not
 * included — and the adapter must assign both fields unconditionally rather than rely on what the
 * default happens to be.
 *
 * The polling task's core and priority are NOT quoted here. They used to be, transcribed out of
 * `firmware.cpp`, and review found that mutating the copy left the entire suite green. They now come
 * from `core_layout.h`, which is the one definition and which `firmware.cpp` is reconciled against by
 * `httpd_task_policy_test.cpp`.
 */
namespace httpd_policy {

/**
 * What `HTTPD_DEFAULT_CONFIG()` yields, kept so the hazard is asserted rather than described.
 * Provenance: esp_http_server.h:27 and :29, quoted in full above.
 */
inline constexpr unsigned kSdkDefaultTaskPriority = 5;
inline constexpr int kSdkDefaultCoreId = core_layout::kNoAffinity;

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
 * `pollingPriority` is a parameter, not a read of the constant, so the tests can drive layouts the
 * firmware does not currently have. It MUST stay read in the body: `-Wunused-parameter` under
 * `-Werror` is what stops a future edit from quietly hardcoding today's layout back in.
 */
constexpr bool canDelayPolling(unsigned priority,
                               unsigned pollingPriority = core_layout::kPollingTaskPriority) {
  return priority >= pollingPriority;
}

/**
 * Does pinning to `core` violate R2.1.0's dedication of the polling core?
 *
 * `core_layout::kNoAffinity` counts as a violation. "The scheduler is free to place it there" is not
 * a defence — it is the failure, deferred to whichever load pattern happens to trigger it, which is
 * worse than a deterministic one because it will not reproduce on a bench.
 *
 * Same rule as `canDelayPolling` about `pollingCore`: it must stay read in the body, and
 * `-Wunused-parameter` under `-Werror` is the enforcement. Substituting the constant for it here
 * would leave every check green while the policy stopped tracking the layout.
 */
constexpr bool violatesCoreDedication(int core, int pollingCore = core_layout::kPollingTaskCore) {
  return core == pollingCore || core == core_layout::kNoAffinity;
}

/**
 * The admissible priority band: strictly above idle, strictly below polling.
 *
 * Upper bound: `canDelayPolling`, above. Lower bound: the idle task of whichever core the portal
 * lands on must still be scheduled, and on a WDT-checked core (core 0 today) failing to schedule it
 * is a panic. A task AT the idle priority merely time-slices with idle instead of yielding to it.
 */
constexpr bool priorityIsAdmissible(unsigned priority,
                                    unsigned pollingPriority = core_layout::kPollingTaskPriority) {
  return priority > core_layout::kIdlePriority && !canDelayPolling(priority, pollingPriority);
}

/**
 * Does a given polling priority leave ANY legal priority for the HTTP task?
 *
 * With polling at 2 the band `(0, 2)` contains exactly one value, so this policy has no slack at
 * all. Lowering the polling priority to 1 would leave nothing admissible — the portal would have no
 * safe priority to run at, and that is a design decision, not a number to nudge. Asserted below so
 * it cannot be discovered on hardware.
 *
 * Deliberately NOT written by searching the band: it is an independent statement of when the band is
 * non-empty, so the test can hold the two against each other across every layout the scheduler
 * admits. Two implementations that agree by execution beat one that is agreed with by review.
 */
constexpr bool layoutAdmitsAnyPriority(unsigned pollingPriority) {
  return pollingPriority > core_layout::kIdlePriority + 1;
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
 * The core `httpd_config_t.core_id` must be set to — derived from the layout, never a literal.
 *
 * Pinning is belt-and-braces on top of the priority: it is what removes the watchdog failure mode
 * described above, which `kTaskPriority` alone does not fully (the WDT watches whether idle gets
 * scheduled, and priority 1 still round-robins with the priority-0 idle task).
 *
 * There is no runtime check that this is a derivation rather than a literal, and there cannot be —
 * `otherCoreThan(0)` and `1` are the same value today, so no test can tell them apart. What catches a
 * literal is the `static_assert` on `violatesCoreDedication(kTaskCore)` below, the moment the layout
 * moves: hardcode `1` here, move polling to core 1 in `core_layout.h`, and the build stops.
 */
inline constexpr int kTaskCore = core_layout::otherCoreThan(core_layout::kPollingTaskCore);

// ── The point of the whole header ──────────────────────────────────────────────────
//
// These are the invariants, and they are static_asserts rather than checks so that breaking one is a
// build failure rather than a field report of a device that reboots under load. Because
// `core_layout.h` is now the single definition of the polling task's placement, the first of these
// reaches ACROSS files: lowering the polling priority to 1 in core_layout.h — with or without
// touching this file — stops the build here.

static_assert(kTaskPriority < core_layout::kPollingTaskPriority,
              "The portal's HTTP task must run strictly below the pulse-polling task (R2.1.0, "
              "§2.1.3). Raising it — or lowering the polling task's priority in core_layout.h — "
              "reintroduces esp_http_server's default hazard.");
static_assert(kTaskPriority > core_layout::kIdlePriority,
              "The HTTP task must not sit at the idle priority: on a watchdog-checked core that is "
              "CONFIG_ESP_TASK_WDT_PANIC territory.");
static_assert(kTaskPriority < core_layout::kModbusServerPriority,
              "R2.1.4 — network tasks run below the Modbus handler, or a frame in progress gets a "
              "framing error rather than a delay.");
static_assert(layoutAdmitsAnyPriority(core_layout::kPollingTaskPriority),
              "No priority is admissible under this core layout: the band between the idle priority "
              "and the polling priority is empty.");
static_assert(priorityIsAdmissible(kTaskPriority), "kTaskPriority must satisfy its own band.");
static_assert(!violatesCoreDedication(kTaskCore),
              "The HTTP task must be pinned off the polling core (R2.1.0).");

// The STATUS block above claims that on the planned Arduino `WebServer` stack the portal inherits the
// logic task's placement and is therefore compliant "by construction". That is a conclusion drawn
// from core_layout.h exactly like every other one in this file, so it is asserted rather than merely
// written down — otherwise it is the same defect review just found, moved into prose. Note that the
// firmware-agreement test cannot catch this one: raise the logic task to priority 2 on BOTH sides and
// the two still agree, so only the compiler is in a position to object.
static_assert(priorityIsAdmissible(core_layout::kLogicTaskPriority),
              "The logic task is what would pump WebServer::handleClient(), so it must itself be a "
              "legal home for the portal (R2.1.0, §2.1.3). If it is not, this header's claim that "
              "the portal is compliant by construction on that stack is false.");
static_assert(!violatesCoreDedication(core_layout::kLogicTaskCore),
              "...and it must be off the polling core, for the same reason and by R2.1.0 directly.");

// And the defaults, asserted rather than merely described, so the reason this header exists is
// itself checked by the compiler.
static_assert(canDelayPolling(kSdkDefaultTaskPriority),
              "HTTPD_DEFAULT_CONFIG's priority is supposed to be unsafe here; if this ever fails, "
              "the quoted SDK value has drifted and the whole rationale needs re-reading.");
static_assert(violatesCoreDedication(kSdkDefaultCoreId),
              "HTTPD_DEFAULT_CONFIG's tskNO_AFFINITY is supposed to be unsafe here.");

}  // namespace httpd_policy

}  // namespace plc
