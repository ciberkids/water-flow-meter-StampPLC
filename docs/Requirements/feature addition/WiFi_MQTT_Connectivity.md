# Requirement: WiFi and MQTT Connectivity

**Version:** 0.1 (proposal — open questions in §11 need answers before implementation)
**Date:** 2026-08-01

> **0.1** — first draft. Specifies WiFi station configuration, an MQTT client that Home
> Assistant discovers without hand-written YAML, and the status the display must show.
>
> Two findings in §2 determine how large this feature actually is, and both were discovered
> by reading the existing code rather than assumed: the settings catalogue **cannot represent
> a string at all** (§2.2), and the menu-pack completeness rule means new settable values
> **invalidate every existing pack** at export time (§2.3). Neither is a reason not to build
> this. Both are reasons the estimate is not "add a WiFi library".

**Depends on:** the navigation model in `Display_UI_Requirements.md` §5, the settings
catalogue (`ui/core/ui_settings_types.h`), the generated manifest (decision **D2**, done),
and the pack format in `Loadable_UI_Menu_Packs.md`.

**Hardware:** ESP32-S3FN8 on the M5Stack StampPLC. WiFi 2.4 GHz station mode. The S3 has
BLE but **no Bluetooth Classic**, which rules out one provisioning route (§3.3).

---

## 1. Purpose and scope

The device measures water flow and already exposes everything over Modbus RTU. That serves
a PLC or a SCADA head-end. It does not serve a home automation system, which is where this
device will actually live.

**In scope**

- WiFi station configuration, settable **both** on the display and over Modbus.
- An MQTT client that publishes readings, and that Home Assistant discovers automatically —
  entities appear without the user writing configuration YAML.
- WiFi and MQTT connection status shown on the main screen, and in detail on their own
  screens.
- All of the above expressed as catalogue values and menu-pack screens, exactly like every
  other setting, so nothing about this feature is special-cased.

**Explicitly out of scope for this version** — each is noted where it would otherwise look
like an omission:

| Deferred | Why |
| --- | --- |
| Static IP / manual DNS | DHCP covers the realistic deployment. Adds four more text settings, each of which the completeness rule would then force into every menu pack (§2.3). |
| WiFi as a Modbus **TCP** transport | Different requirement. This feature is MQTT only; RTU over RS485 remains the industrial path. |
| OTA firmware update over WiFi | Deserves its own requirement, including rollback and signing. Attractive once the radio works, and the partition table should leave room for it (§9.4). |
| IPv6 | No consumer MQTT broker deployment needs it. |

---

## 2. The three constraints that shape everything below

Read this section before the design. Each of these was verified against the code, and each
one changes what the feature costs.

### 2.1. WiFi must not degrade the measurement

This is the binding constraint, and it outranks every convenience in this document.

Core 0 busy-polls the flow sensors and reports its achieved rate in **kHz**
(`firmware.cpp`, `pollingRate_kHz`). Core 1 runs Modbus RTU at task priority 8 plus the
logic and UI at priority 1. The ESP32's WiFi stack is not a passive library: it has its own
tasks, its own ISRs, and it takes locks that can stall the other core.

If enabling WiFi lowers the achieved polling rate, the device counts fewer pulses, and
**every published value becomes less accurate than it was before we added the feature that
publishes it.** That would be a self-defeating outcome.

Therefore:

> **R2.1.0 — CORE 0 IS DEDICATED TO SENSOR POLLING AND NOTHING ELSE.** Stated by the project
> owner on 2026-08-03 as a standing invariant, and it outranks R2.1.1: the 5 % budget below is a
> *measurement* that must hold, while this is a *structural* rule about what may run where.
>
> Nothing belonging to WiFi, MQTT, the display, the card or Modbus may be scheduled on core 0.
> Everything else lives on core 1, which today runs Modbus RTU at priority 8 and the logic/UI at
> priority 1 and will have to accommodate the radio too.
>
> The reason is the product's reason for existing: the device counts pulses, and a pulse missed
> because core 0 was doing something else is a measurement error that no amount of downstream
> processing recovers. Every other feature is negotiable against this one.
>
> **If the framework will not let the WiFi stack off core 0, the feature does not ship in that
> form.** Two consequences follow, and neither is "accept the degradation quietly":
> re-examine whether the stack can be pinned via sdkconfig, and if it truly cannot, report the
> measured cost and let the owner decide. §11 Q1 is where that answer goes.

> **R2.1.1** — Enabling WiFi **must not** reduce the measured `pollingRate_kHz` by more than
> **5 %** from its radio-off baseline, measured over 60 s at steady state with the radio
> associated and MQTT publishing at its configured cadence. This is the empirical check that
> R2.1.0 was actually achieved, not an allowance to violate it.
>
> **R2.1.2** — The device must record the radio-off baseline once, at the first boot after a
> firmware update, and expose both the baseline and the live rate. A regression must be
> visible on the device, not only in a lab.
>
> **R2.1.3** — WiFi scanning is the worst offender for long blocking periods. A scan must
> only ever be initiated by an explicit user action, never automatically, and never while a
> countdown or an editor is open.

The acceptance test is deliberately expressed against a value the device already publishes:
`diagnostics.pollingRateKhz` reached the display and register 0 as part of decision **D2**,
so this criterion is measurable the day the hardware arrives, with no extra instrumentation.
That is also the answer to open decision **G1** — the same measurement serves both.

### 2.1.1. The radio also contends with RS485, not only with the counter

Raised by the project owner on 2026-08-03, and it widens the blast radius: this feature touches
more than the menu work did, because **core 1 already carries Modbus RTU at priority 8** and the
radio has to fit alongside it.

Modbus RTU is timing-sensitive in a way MQTT is not. A frame is delimited by a 3.5-character
silent interval, so a task that starves the RS485 handler for a few milliseconds does not slow it
down — it makes the master see a framing error and retry. So:

> **R2.1.4** — The WiFi and MQTT tasks must run at a priority **below** the Modbus handler's, so
> a radio event can never delay a frame in progress.
>
> **R2.1.5** — Acceptance must measure the **Modbus error rate** as well as the polling rate,
> before and after enabling the radio. A retry rate that climbs is the symptom, and it is
> invisible to any test that only watches `pollingRate_kHz`.

### 2.1.2. The counter is I²C-bound, not CPU-bound — so core affinity is not enough

Established 2026-08-03 by reading the board library, and it reframes R2.1.0.

`M5StamPLC.readPlcInput()` reads the **AW9523B at 0x59**. It is an I²C expander, not a GPIO. Two
things follow immediately:

- **Hardware pulse counting is unavailable.** There is no pin to attach PCNT or `attachInterrupt`
  to, so "move the counter out of harm's way" — normally the right answer — is off the table
  without changing the board.
- **The polling loop is I²C traffic.** What starves it is contention for that bus, not for a CPU.

And the RGB LED is the **PI4IOE5V6408 at 0x43 — the same bus**, SCL 15 / SDA 13, one driver mutex
(`StampPLC specifications.md` §1.1). `LedController::applyOutputs` wrote it unconditionally, and
`ledController.update()` runs every pass of a loop that ends in `vTaskDelay(1)` at 1000 Hz.

> Measured by disabling the fix and re-running the host suite: **1000 expander writes per 1000
> idle passes, now 0.** Core 1 was competing with the measurement a thousand times a second, radio
> off, before WiFi entered the picture at all.

> **R2.1.6 — NO SUBSYSTEM MAY WRITE THE SENSOR I²C BUS WITHOUT NEED.** Any device on
> SCL 15 / SDA 13 — the LED expander, the INA226, the LM75B, the RTC — must be written only when
> its state actually changes, and polled only as often as its purpose requires. A dirty check is
> the minimum; a rate limit is better for anything that legitimately changes often.
>
> This sits alongside R2.1.0 rather than under it, because **core affinity does not protect the
> measurement.** R2.1.0 keeps foreign *computation* off core 0; R2.1.6 keeps foreign *bus traffic*
> away from the sampler. Both are needed and neither implies the other.

Fixed in `led_controller.cpp` on 2026-08-03, deliberately **before** the radio-off baseline is
recorded: a post-fix radio-ON measurement beating a pre-fix radio-OFF one would have made
R2.1.1's 5 % comparison flatter the radio.

### 2.1.3. What the framework will and will not allow

Verified 2026-08-03 against the installed toolchain — arduino-esp32 **2.0.17** (package
3.20017.0, IDF v4.4.7), by reading the prebuilt sdkconfig and disassembling the shipped archives.
The answer is a split, not a yes or no:

| Task | Core | Movable? |
| --- | --- | --- |
| WiFi (`pp`) | 0 by default (`CONFIG_ESP32_WIFI_TASK_PINNED_TO_CORE_0=y`) | **Yes** — `wifi_task_core_id` is a *runtime* field of `wifi_init_config_t`, filled by `WIFI_INIT_CONFIG_DEFAULT()` in `WiFiGeneric.cpp`, which is compiled from source. `-DCONFIG_ESP32_WIFI_TASK_PINNED_TO_CORE_1` moves it. |
| lwIP `tiT` | 0, priority **18** | **No.** `liblwip.a(sys_arch.c.obj)`'s `sys_thread_new` emits the core id as a literal `movi.n a7, 0`, compiled into a shipped archive. No build flag reaches it. |
| Supplicant / timer helpers | `tskNO_AFFINITY` | No — they float onto either core. |
| `mqtt_task` (esp-mqtt) | `tskNO_AFFINITY` — `CONFIG_MQTT_TASK_CORE_SELECTION_ENABLED` is **not** set, so `esp_mqtt_client_start` takes the `xTaskCreate` branch, and `task.h:450` forwards `tskNO_AFFINITY` | Core: no. **Priority: yes** — `task_prio` is a runtime field. See R4.1.5: setting it to 1 makes the core question moot. |
| Arduino event + main loop | 1 already | n/a |

**Priority is the lever that matters, not affinity.** The polling task is priority 2 on core 0. Any
task at priority 1 cannot preempt it *on either core*, so an unpinned priority-1 task is harmless
even when the scheduler places it on core 0. This is why R4.1.5 fixes the MQTT task at priority 1
rather than trying to pin it: the affinity question stops being load-bearing. The same reasoning
does **not** rescue lwIP, which is priority 18 and therefore preempts polling whenever it has a
packet to process.

**What the residual actually costs.** Steady-state MQTT is one publish per `publishPeriod` (default
10 s) — a handful of packets, so `tiT` is idle almost all the time. The one genuinely high-traffic
mode is the configuration portal (§7.6), and it is user-initiated, menu-gated, and time-boxed by
R7.4 rather than always-on. The invariant therefore survives in the case that runs 99.99 % of the
time, and degrades only in a mode the user explicitly opened. That is a characteristic to measure
(N9), not a reason to redesign.

So **R2.1.0 as written cannot be fully satisfied under `framework = arduino`.** The options:

1. Move the WiFi task, accept `tiT` on core 0, and measure. This is what R2.1.0's own escape clause
   prescribes, and `tiT` only runs when there is traffic to process — at a 10 s publish cadence
   that is very little. Unmeasured, though: no hardware figure exists yet.
2. Rebuild the SDK (`framework = espidf`, or a fork with `custom_sdkconfig`) and set
   `CONFIG_LWIP_TCPIP_TASK_AFFINITY_CPU1`. Real, but it requires arduino-esp32 3.x / IDF 5.x —
   where `DNSServer` was rewritten onto AsyncUDP and **spawns a task**, so this choice interacts
   with the provisioning design rather than being independent of it. The symbol is also renamed to
   `CONFIG_ESP_WIFI_TASK_PINNED_TO_CORE_*` in 5.x.
3. Move the counter to hardware — **unavailable**, per §2.1.2.

**Correction to an earlier claim in this document's history:** it was previously asserted that a
`-D` in `build_flags` cannot change what a prebuilt WiFi library already did. That is refuted for
the WiFi task, whose core is a runtime value. It remains true for lwIP.

**Mitigations** to be confirmed against the framework (see §11 Q1): pinning the WiFi and
MQTT tasks off core 0 per R2.1.0, keeping them below Modbus in priority per R2.1.4, disabling
modem power-save (which trades latency spikes for current), and keeping the MQTT client out of
any code path the UI or Modbus tasks call.

> A note on honesty: if it turns out the Arduino framework will not let us keep WiFi off
> core 0, the correct outcome is **not** to ship anyway and hope. It is to report the measured
> degradation and let the user decide, because they are the only one who knows whether their
> application can accept it.

### 2.2. The settings catalogue cannot represent a string

Verified in `ui/core/ui_settings_types.h` and `.cpp`. The entire settings model is integral:

| Element | Current type |
| --- | --- |
| `SettingKind` | ~~`{ Numeric, Enum, Boolean }` — no text kind~~ → **`Text` added 2026-08-01**, with `maxLength` and `writeOnly` |
| `SettingDescriptor::min` / `max` / `step` | `int32_t` |
| `readSetting(...)` | returns `int32_t` |
| `writeSetting(..., int32_t value, ...)` | takes `int32_t` |
| `adjustSetting(setting, int32_t value, int32_t delta)` | returns `int32_t` |
| `formatSetting(setting, int32_t value, char* out, ...)` | takes `int32_t` |

This feature needs **seven** text settings: WiFi SSID and passphrase, MQTT host, username,
password, base topic and discovery prefix. None of them fits.

The editor model does not fit either, and that is the deeper problem. A numeric editor is
built from `increment` / `decrement` / `commit` / `discard` with hold acceleration. A string
has no step. Editing text needs a different action set and a cursor, which means new actions,
which means the completeness rule's derived action set changes too (§2.3).

**This is the single largest piece of work in the feature**, and it is a change to the core
settings abstraction rather than an addition beside it. It is specified in §6.

**Progress, 2026-08-03.** The *type system* half has landed: `SettingKind::Text`, `maxLength`,
`writeOnly`, `formatSettingText` with masking, and the manifest and schema plumbing — commit
`2e37452`, with the character-wheel editor engine and its 47 host checks in `0392013`. What remains
is the storage half: `readSettingText` / `writeSettingText` and the register-block packing of §5. The
rows above are struck through where they are no longer true rather than deleted, so the size of what
was involved stays visible.

### 2.3. The completeness rule turns new settings into a migration

`Loadable_UI_Menu_Packs.md` §3.0.1 states:

> A menu is invalid unless every `category: "setting"` value in the catalogue has a
> reachable editor, and every action those editors require is referenced.

That rule is good, and it was deliberate — it makes it impossible to load a menu that
strands a setting. But it has a consequence nobody has had to face yet, because until now
the catalogue has not grown:

**This feature adds 14 settings, taking the required editor count from 10 to 24.** Every
existing pack, and the built-in default menu, becomes *incomplete* the moment the catalogue
grows. At export that is a hard failure (§5 of that document); at load it is a soft failure
that appends built-in editors.

**Decided 2026-08-03 (Q7): break it.** The owner's reasoning is that there is no live product yet,
so backwards compatibility is not worth paying for — the export gate keeps its full force and any
existing pack is simply re-authored. That is the right trade while the only packs in existence are
ones we generate ourselves.

Two consequences worth writing down so this decision is revisited deliberately rather than
inherited by accident:

- **The skeleton generator is the migration.** `tools/skeleton/generate.mjs` regenerates the default
  menu from the catalogue and already refuses to emit an incomplete one, so "re-author the pack"
  means "re-run the generator" for anything we ship.
- **This stops being free the day a customer has authored a pack.** At that point Q7's
  version-and-warn option becomes necessary, and the manifest will need the catalogue version it
  does not carry today. Recorded here rather than in a backlog because the trigger is a product
  event, not a development one.

---

## 3. WiFi

### 3.1. States

The radio is a state machine with exactly one owner. No other task may call WiFi APIs.

| State | Meaning | Display text (ASCII only — see §4.6) |
| --- | --- | --- |
| `Disabled` | No credentials, or explicitly turned off. Radio powered down. | `OFF` |
| `Connecting` | Associating and awaiting DHCP. | `CONN` |
| `Connected` | Associated with an IP address. | `OK` |
| `Retrying` | Failed; waiting out the backoff before another attempt. | `RETRY` |
| `Failed` | Repeated failure; likely wrong credentials or no AP. | `FAIL` |

> **R3.1.1** — Default state is `Disabled`. A device that has never been configured must not
> power the radio at all. This keeps §2.1 satisfied by default and means the feature cannot
> degrade a device whose owner does not want it.
>
> **R3.1.2** — Reconnection uses exponential backoff from 1 s to a 5 min ceiling, with
> jitter. A device that cannot reach its AP must not retry in a tight loop: that is the
> worst case for both §2.1 and for the current draw in §9.3.
>
> **R3.1.3** — Losing WiFi must never affect Modbus, the pulse counting, or the UI's
> responsiveness. The link going down is a normal condition, not a fault.

### 3.2. Configuration surface

Both paths write the same staged block and go through the same validation, exactly as the
RS485 link settings do (`link_settings.h`, register block 40–47). There is one
implementation, not a display path and a Modbus path that drift.

- **Display** — settings screens in the menu pack, one editor per value (§7).
- **Modbus** — the register block in §5, using the established stage-then-apply idiom:
  write the fields, then write `0x5AA5` to the apply register. Partial writes never take
  effect, so a master writing a 32-register SSID does not cause 16 reconnection attempts.

### 3.3. Credential entry, and why it is the ugly part

A WPA2 passphrase is 8–63 printable ASCII characters. The device has three buttons.

The honest arithmetic for the on-display character-wheel editor, using the existing
acceleration tiers in `ui_accel.h` (1 step <700 ms, then 5, then 25). These figures are
**measured by a host test, not estimated** — the test counts the presses for a real
passphrase and asserts the cost is high, so this table cannot quietly become optimistic:

| | |
| --- | --- |
| Charset | 95 printable ASCII, plus a `DEL` and an `END` pseudo-character |
| Mean presses to reach a character by short press | **~27**, measured — not the ~24 a uniform-distribution estimate gives, because real passphrases favour letters and digits over the punctuation that sits near the ring's origin |
| A realistic 14-character passphrase | **381 short presses**, measured by `test/host/text_editor_test.cpp` over `Tr0ub4dor&3-xK` |
| Extrapolated to 16 characters | **~435 short presses**, or roughly 30–60 s of held-button scrubbing plus 16 commits |

That is not a good experience, and this document is not going to pretend otherwise. It was
**required** for a while, on the reasoning that it is the only path needing nothing but the device
in your hand. That reasoning was wrong twice over: the path is unusable at passphrase length, and it
is not the only one — enabling the radio is a boolean, and the AP it raises leads to a form.

> **R3.3.1 — WITHDRAWN 2026-08-03.** The character-wheel editor was required, then built, then
> removed by the owner: three buttons are not an input method for a passphrase. See §6.3. The row
> for it is kept in the table below because the honest cost comparison is the reason it was dropped.
>
> **R3.3.2** — Text entry therefore happens **only** through the bulk paths. Since R3.3.1 is gone
> this is no longer "so nobody is *forced* through the wheel" — it is the entire input story, and at
> least one of these must work or the device cannot be provisioned at all. See **Q2**.

| Route | UX | Cost | Security exposure | Fails how |
| --- | --- | --- | --- | --- |
| ~~**Character wheel**~~ (R3.3.1, **withdrawn**) | Unusable at passphrase length | Medium — built, then deleted | PSK visible on screen while typing | Mis-typed; user retries |
| **Modbus write** | Good, if you have a Modbus master | Low — the block already needs to exist | **Plaintext over unauthenticated RS485** | Wrong value; rollback applies |
| **File on the SD card** | Good | **Low** — the card reader and parser already exist for menu packs | Plaintext on a removable card | File absent or malformed; report and stay disabled |
| **SoftAP + captive portal** | Best for a non-technical user | High — HTTP server, HTML, and the AP competes with §2.1 | Open AP during provisioning | Timeout, revert to `Disabled` |
| **WPS push-button** | Excellent when it works | Low | WPS is widely deprecated and often disabled in APs | Silently never associates |
| **SmartConfig / ESP-TOUCH** | Needs a vendor phone app | Medium | UDP broadcast of the PSK | Opaque failure |
| **BLE provisioning** | Good, needs an app | High, and BLE shares the radio with WiFi | Pairing model | — |

**Superseded 2026-08-03.** The owner chose the **web page** (§7.6), for both WiFi and MQTT, and
that is the better answer than the SD-card file this section originally recommended. Three reasons
the earlier reasoning was wrong:

- It judged SoftAP "the better product answer and the wrong first answer" on the assumption the
  portal needed a web stack we did not have. `WebServer` and `DNSServer` ship **bundled** with
  arduino-esp32 2.0.17 (verified), so the portal costs no dependency at all.
- It weighed only the WiFi passphrase. MQTT adds five more text fields, and at the measured ~27
  presses per character the card and the (now withdrawn) character wheel are both worse than a form by a wide
  margin.
- The card path would have put credentials in clear text on removable media — a comparable
  exposure to R7.10's clear-text HTTP, but persistent rather than confined to a ten-minute window.

The SD-card file remains a reasonable **later** addition for fleet provisioning, where preparing
many cards at a desk beats visiting many devices with a phone. It is not needed for one device.

### 3.4. What the display shows

> **R3.4.1** — The main screen (`info-p0-global-status`) shows a compact combined indicator:
> WiFi state and MQTT state, in that order, in the ASCII vocabulary of §3.1 and §4.5.
>
> **R3.4.2** — A WiFi detail screen shows SSID, state, IP address, and RSSI.
>
> **R3.4.3** — The passphrase is **never** displayed in full anywhere, including on its own
> editor screen, where only the character under the cursor is shown in clear (§6.3).

**Decided 2026-08-03 (Q4): the footer.** `W:OK M:OK` is nine characters and goes in the footer's
deterministic four-row stack, not in P0's body — P0 already carries the totals, the flow, the LED
legend and a scrollbar on 240×135. The footer also puts it on **every** info page rather than only
P0, which is more useful for the same cost.

---

## 4. MQTT

### 4.1. Client behaviour

> **R4.1.1** — The client runs in its own task, never on core 0 (§2.1), and never blocks a
> caller. Publishing is fire-and-forget from the UI's point of view.
>
> **R4.1.2** — Reconnection uses the same backoff policy as §3.1.2. MQTT reconnect storms
> against an unreachable broker are a common way to make a device unusable.
>
> **R4.1.3** — If the publish queue fills, the **oldest telemetry is dropped**, not the
> newest. Stale readings have no value. Discovery and availability messages are never
> dropped.
>
> **R4.1.4** — Client ID must be stable and unique, derived from the device MAC. Two devices
> with the same client ID will disconnect each other in a loop, and diagnosing that from the
> device end is miserable.
>
> **R4.1.5** — The MQTT task runs at **priority 1** — below the priority-2 polling task and far
> below the priority-8 Modbus server. This is the requirement that makes R2.1.0 and R2.1.4 hold
> for MQTT; see §2.1.3 for why priority rather than affinity is the lever.
>
> **R4.1.6** — `out_buffer_size` is set explicitly, and **every publish return value is
> checked**. See §4.4.7 — a payload larger than the buffer is dropped silently, and the
> observable symptom is an entity that never appears in Home Assistant, with nothing logged.

### 4.1.1. The client is the one already in the box

**Decided 2026-08-03.** Verified against the installed toolchain, not chosen from a comparison of
third-party libraries: ESP-IDF's own `esp-mqtt` is **already present and already linked**.

| Evidence | Finding |
| --- | --- |
| `tools/sdk/esp32s3/lib/libmqtt.a` | Prebuilt and shipped |
| `include/mqtt/esp-mqtt/include/mqtt_client.h` | On the default include path |
| `CONFIG_MQTT_PROTOCOL_311=y` | MQTT 3.1.1 compiled in |
| `CONFIG_MQTT_TRANSPORT_SSL=y` | TLS available if ever wanted |

So `lib_deps` does not grow, which matters here beyond mere tidiness: PubSubClient and
arduino-mqtt publish from the **caller's** task, which would put broker I/O on whichever task
called `publish()` and hand us the exact core-0 and priority-inversion problems §2.1.3 is about.
`esp-mqtt` owns a task, and exposes the two fields that make it conform:

```c
esp_mqtt_client_config_t cfg = {};   // IDF 4.4 — FLAT struct
cfg.task_prio       = 1;             // R4.1.5
cfg.out_buffer_size = 2048;          // R4.1.6
cfg.lwt_topic       = "<base>/status";   // R4.5 availability, for free
cfg.lwt_msg         = "offline";
cfg.lwt_retain      = 1;
cfg.disable_auto_reconnect = true;   // R4.1.2 — see the correction below; WE own retry timing
```

> ⚠️ **This is the IDF 4.4 flat config struct.** Every ESP-IDF 5.x example on the internet writes
> `.broker.address.uri`, `.credentials.username`, `.session.keepalive` — a nested layout that does
> **not exist** in IDF v4.4.7 and will not compile here. Fields are flat: `.uri`, `.host`, `.port`,
> `.username`, `.password`, `.keepalive`, `.lwt_topic`. Confirmed by reading the installed header.

LWT being a library feature rather than our code is the real prize: R4.5 is satisfied by
configuration instead of by a state machine we would have to test.

> **CORRECTION 2026-08-05 — reconnect backoff is NOT free, and this section claimed it was.** I wrote
> that `reconnect_timeout_ms` satisfied R4.1.2. It does not: it is a **fixed interval**, and R4.1.2
> requires §3.1.2's **exponential** ladder. A fixed 10 s retry against a broker that is down is
> exactly the reconnect storm R4.1.2 exists to prevent.
>
> The defect was in this document, not in the code — the reviewer who found it was right to say so.
> Owner decision 3B: implement the ladder ourselves. `disable_auto_reconnect = true` so the library
> stops its own fixed-interval retry, and `src/net/mqtt_reconnect.h` drives
> `esp_mqtt_client_reconnect()` on the same ladder shape `WifiManager` already uses — one backoff
> behaviour on the device rather than two that drift.

### 4.2. Topic layout

Base topic is configurable, defaulting to `watermeter/<mac-suffix>`.

```
<base>/status                     online | offline   (retained, LWT)
<base>/sensor/<n>/state           JSON, one payload per sensor
<base>/total/state                JSON, aggregate
<base>/diagnostics/state          JSON, polling rate and undersampling
```

**One JSON payload per sensor, not a topic per value.** With 8 sensors and 5 metrics each,
per-value topics would mean 40 publishes per cycle; a single payload per sensor is 8. Home
Assistant handles this natively with `value_template` against a shared `state_topic`, so
this costs nothing on the consuming end and saves an order of magnitude of broker traffic
and radio airtime — which §2.1 cares about directly.

### 4.3. Publish cadence

> **R4.3.1** — Publish on change, rate-limited to a configurable minimum interval
> (`config.mqtt.publishPeriod`, default 10 s, range 1–3600 s).
>
> **R4.3.2** — Publish a full set at least every 60 s regardless of change, so a subscriber
> that joined late is not left with an empty state.
>
> **R4.3.3** — Cumulative totals are published at the same cadence as everything else. They
> must **never** be published from the flash-write path — persistence cadence and publish
> cadence are unrelated, and coupling them would put a network condition in the way of a
> durability guarantee.

### 4.4. Home Assistant discovery

**Verified 2026-08-03** against two primary sources rather than written from memory: Home
Assistant's own `developers.home-assistant.io` sensor-entity reference and
`home-assistant.io/integrations/sensor.mqtt`, cross-checked against a **live HA instance**
running `core-2026.7.4` with the MQTT integration loaded. Verified this way because a wrong string
here produces an entity that silently never appears — there is no error to read.

What the live instance confirmed, from its own MQTT config entry:

| Setting | Value on the live instance |
| --- | --- |
| `discovery_prefix` | `homeassistant` |
| `birth_message` | topic `homeassistant/status`, payload `online` |
| `will_message` | topic `homeassistant/status`, payload `offline` |

and, from a real `platform: mqtt` sensor's registry entry, that `suggested_display_precision`
is honoured — it lands in the entity registry as
`options.sensor.suggested_display_precision`. That closes the open worry that HA would infer
0 decimals and render a litres-per-second reading as a bare integer.

The intent is fixed even where the spelling is not:

> **R4.4.1** — Entities appear in Home Assistant with **no user-written YAML**.
>
> **R4.4.2** — All entities group under **one device**, so the eight sensors and the
> diagnostics appear as one water meter rather than as unrelated entities.
>
> **R4.4.3** — Every entity carries a stable `unique_id`, so the user can rename and
> reconfigure it in the HA UI and have that survive a restart.
>
> **R4.4.4** — Cumulative volume must be published in the form HA's **Water dashboard**
> consumes, so that long-term statistics and the energy-style dashboard work. This is the
> single most valuable integration detail in the feature and the most likely to be got
> subtly wrong.
>
> **R4.4.5 — CORRECTED 2026-08-05.** Discovery messages are retained, but this requirement had the
> justification backwards and the priority with it.
>
> It read: "so HA re-discovers after its own restart without the device republishing", presenting
> retain as the mechanism and R4.4.7's birth-message republish as a backstop for the cases retain
> misses. Home Assistant's own MQTT-discovery documentation says close to the opposite — it presents
> **birth-triggered republish as the primary and better approach**, and retained discovery as an
> alternative carrying an explicit warning that it **can create ghost entities**: an entity the device
> no longer publishes survives in HA because the retained config outlives it.
>
> So retain is a CHOICE WITH A COST, not the mechanism. It is kept, because it makes a device that is
> asleep or off at the moment HA restarts still discoverable — but it is no longer the thing being
> relied on.
>
> **R4.4.7 is therefore the PRIMARY mechanism**, not the backstop: the device subscribes to
> `homeassistant/status` and republishes discovery on `online`. That is the path HA documents, it
> cannot produce a ghost entity, and it is the one that must be correct.
>
> The cost of retain that we accept: an entity removed from the firmware's catalogue keeps appearing
> in HA until somebody clears the retained topic. Worth knowing before a sensor is renamed.

> **R4.4.7** — The device **subscribes to `homeassistant/status`** and republishes discovery on
> receiving `online`. This is the primary mechanism (see the correction on R4.4.5). It also covers the
> case retain cannot: a broker restart that dropped retained state, or an HA database migration.

> **R4.4.9 — TWO DIAGNOSTICS ARE PUBLISHED WITH NO ENTITY** (found 2026-08-05). `uptimeS` and
> `baselineKhz` appear in the diagnostics JSON payload but have no `HaEntity`, so no discovery message
> ever announces them and they will never appear in Home Assistant. Either give them entities or stop
> publishing them — a value on the wire that nothing consumes is bytes and airtime for nobody, and it
> reads to the next person as though the integration is broken.

#### 4.4.a. The exact strings

Quoted literally from HA's sensor-entity reference. The MQTT discovery payload uses the
snake-case string form of each `SensorDeviceClass` member.

| Our value | `device_class` | `unit_of_measurement` | `state_class` |
| --- | --- | --- | --- |
| Instantaneous flow | `volume_flow_rate` | `L/min` | `measurement` |
| Cumulative volume, session | `water` | `L` | `total_increasing` |
| Cumulative volume, lifetime | `water` | `m³` | `total_increasing` |
| Board temperature | `temperature` | `°C` | `measurement` |
| Polling rate, undersampling | *(none)* | `kHz` / *(none)* | `measurement` |

Permitted units, so a later change of mind stays inside the allowed set rather than silently
breaking the entity:

- `volume_flow_rate` → `m³/h`, `m³/min`, `m³/s`, `ft³/min`, `L/h`, `L/min`, `L/s`, `gal/d`,
  `gal/h`, `gal/min`, `mL/s`
- `water` → `L`, `gal`, `m³`, `ft³`, `CCF`, `MCF`
- `state_class` → `measurement`, `measurement_angle`, `total`, `total_increasing`

**R4.4.4's answer, concretely:** the Water dashboard needs `device_class: water` **and** a
`state_class` of `total_increasing` (or `total`) **and** a unit from the `water` list. All three,
together — that combination is what makes long-term statistics accumulate. `total_increasing` is
correct for our lifetime counter because it is monotonic and HA handles the reset-to-zero case
itself. Diagnostics carry `entity_category: diagnostic`, which for sensors is the **only**
permitted value of that key.

#### 4.4.b. One topic per entity, not one payload per device

Discovery topic, per entity:

```
homeassistant/sensor/<node_id>/<object_id>/config      retained
```

with a `device` block repeated in each payload — that repetition is what groups them under one
device (R4.4.2):

```json
{
  "device": {
    "identifiers": ["wfm_<mac-suffix>"],
    "name": "Water Flow Meter",
    "manufacturer": "M5Stack",
    "model": "StampPLC",
    "sw_version": "<firmware version>",
    "configuration_url": "http://<ip>/"
  },
  "unique_id": "wfm_<mac-suffix>_s1_flow",
  "state_topic": "<base>/sensor/1/state",
  "value_template": "{{ value_json.flow }}",
  "availability_topic": "<base>/status",
  "device_class": "volume_flow_rate",
  "unit_of_measurement": "L/min",
  "state_class": "measurement",
  "suggested_display_precision": 2
}
```

HA also offers a newer **device-based** discovery format — a single
`homeassistant/device/<id>/config` carrying every component at once. **Rejected**, and for the
reason R4.1.6 exists: 8 sensors × 5 metrics is 40 components in one payload, which would run to
several kilobytes and is exactly the shape that overruns the client buffer and vanishes without a
diagnostic. Forty small retained messages cost nothing after the first connect and each stays
comfortably inside the buffer. `unique_id` follows the pattern the reference implementation on the
live instance uses (`<device>_<metric>_<origin>`), which is what lets a user rename an entity in
the HA UI and keep the rename across restarts (R4.4.3).

#### 4.4.7. The buffer is a silent failure, so it gets a test

`esp_mqtt_client_config_t.buffer_size` defaults to **1024 bytes**. A discovery payload carrying the
full device block sits in the same order of magnitude. On overflow `esp_mqtt_client_publish`
returns `-1` and the message is simply not sent: no entity, no log line, no clue.

> **R4.4.8** — A **host test** serialises the worst-case discovery payload — longest sensor name,
> longest base topic, longest `sw_version` — and asserts its length is under the configured
> `out_buffer_size` with margin. The buffer value and the test share one constant.

This gets a test rather than just a bigger buffer because a bigger buffer is a guess that stops
being true the next time a field is added, whereas the test fails at that moment. Given this
project's history, the check that computes the real length is worth more than the margin.

### 4.4.1. Command topics — Home Assistant can act, not only observe

**Decided 2026-08-03 (Q8): commands are in scope**, against my own recommendation to defer them.
The owner wants HA to trigger resets, and buttons in a dashboard are the obvious reason to have
integrated at all. So this specifies them properly, including the parts that make accepting control
over a network safe on a metering device.

```
<base>/cmd/reset-session      payload: "RESET"
<base>/cmd/reset-totals       payload: "RESET"
<base>/cmd/republish          payload: anything
```

Discovered as HA `button` entities, so they appear as buttons rather than as switches that imply
state.

> **R4.4.1 — A DESTRUCTIVE COMMAND REQUIRES AN EXPLICIT PAYLOAD.** `reset-session` and
> `reset-totals` act only on the exact payload `RESET`. Any other payload, including an empty one,
> is logged and ignored.
>
> This mirrors the `0x5AA5` magic the register block already uses for `NET_APPLY` and
> `REG_LINK_APPLY` — the project's established idiom for "this is destructive, prove you meant it".
> Without it, any stray publish to the topic zeroes a customer's totals.

> **R4.4.2 — A RESET IS RATE-LIMITED, AND THE LIMIT IS THE PRIMARY GUARD.**
> Owner's design, 2026-08-03, and it is better than the retain check that preceded it.
>
> The principle, in the owner's words: *a reset failing remotely is not a breaking thing; the device
> entering a reset loop is.* So the guard is built to fail in that direction — when in doubt, swallow
> the command, log it, and carry on.
>
> **Why a rate limit rather than only a retain check.** A retained message is one *cause* of a loop.
> A rate limit addresses the *failure mode*, so it also catches the causes nobody predicted: an
> automation republishing every ten seconds, a flapping connection redelivering QoS 1, a second
> controller nobody knew was subscribed. The retain check is kept as well (R4.4.2c) because it is
> cheap and stops the commonest cause at source — but it is no longer what the safety rests on.
>
> **R4.4.2a — the clock must be the right kind of clock.** The interval is measured on
> `millis()`, not on the RTC. A monotonic uptime cannot jump, whereas an NTP sync can move the wall
> clock backwards mid-operation and would silently re-arm the limit. The RTC timestamp is recorded
> and reported *alongside* it, because that is what an operator can act on — but it is never what
> the comparison is made against. `kResetMinIntervalMs` = **60 s** per command kind: far longer than
> any plausible loop, far shorter than any legitimate repeat.
>
> **R4.4.2b — the limit must survive a reboot, because the worst loop reboots.** An in-RAM
> `millis()` guard handles a device that stays up. It does nothing for the shape that matters most:
> a reset that triggers a crash, a reboot, a reconnect, a redelivered command, another reset —
> because `millis()` starts again each time. So the last accepted reset is also persisted, and the
> guard consults both.
>
> This is the same structure as the menu-pack anti-boot-loop counter (`Loadable_UI_Menu_Packs` §3.6),
> for the same reason, and it inherits the same discipline: **the persisted value is written only
> when a reset is ACCEPTED.** Writing on every rejection would mean a looping command loops NVS
> writes too, turning a nuisance into flash wear.
>
> **R4.4.2c — a retained command message is still discarded**, unconditionally, and logged. It is a
> fault the operator has to clear at the broker, so saying so is worth more than silently
> rate-limiting it forever.
>
> **R4.4.2d — a refusal must be visible, not merely logged.** Home Assistant will show a button; a
> button that silently does nothing is worse than one that reports why. So a rejected command sets
> `mqtt.lastCommandResult` — a catalogue value, published on the status topic and shown on the MQTT
> info page — to one of `accepted`, `rate-limited`, `retained-ignored` or `bad-payload`. The operator
> pressing the button learns the answer without reading a serial log they do not have.

> **R4.4.3** — Commands go through the SAME path as everything else: `reset-session` issues the
> Modbus command a master would (`REG_MASTER_RESET_ALL_SESSION`), not a private code path. So a
> reset from HA, from the display's confirm screen, and from a Modbus master are one implementation.

> **R4.4.4** — A command is acknowledged by the resulting telemetry, not by a reply topic. The
> totals publish immediately after a reset, so HA shows the effect. A separate ack channel would be
> a second thing to keep in step with reality.

> **R4.4.6 — `wifi.rssi` is published as a Home Assistant entity** in the **diagnostic** category
> (Q10, decided 2026-08-03), so it is available without cluttering the main view. RSSI is the first
> thing anyone checks when a device drops off, and the value of publishing it is the *history* — an
> intermittent dropout leaves nothing to look at after the fact if it only ever appeared on the
> panel.

> **R4.4.5** — Commands are accepted only while MQTT is enabled and connected, and are never
> queued. A command that arrives during a disconnect is lost, which is correct: a reset the operator
> asked for two hours ago is not one they still want.

**The security position, stated plainly.** This makes an MQTT broker a control path into a metering
device: a broker compromise, or anyone able to publish to it, can zero the totals. R4.4.1's magic
payload stops accidents, not intent. That is an accepted trade for a device on a home network, and
it is the reason §8's out-of-scope list still excludes anything that changes *configuration* over
MQTT — a reset is recoverable by re-reading the meter, a repointed broker or a changed calibration
is not.

### 4.5. Availability

> **R4.5.1** — A Last Will and Testament on `<base>/status` set to `offline`, with an
> `online` publish on connect, both retained. Entities must show as *unavailable* in Home
> Assistant when the device drops — silently stale values are worse than a visible gap,
> because a flow meter reading zero and a flow meter that is switched off look identical.

### 4.6. Text on the display is ASCII only

The generated UI renders with M5GFX **Font0**, whose glyph table covers codepoints 0–255 and
whose lookup increments above 176. Status strings, state names and error text destined for
the display must therefore stay in printable 7-bit ASCII. No `✓`, no `✗`, no degree signs.

**Decided 2026-08-03 (Q5): the export FAILS.** Not a warning.

This is the worst-shaped bug the pipeline can produce — correct in the design tool, garbage only on
hardware — and it is the exact pattern that has cost this project the most: the mirrored Y axis, the
portrait clamp, the manifest that over-claimed. Each looked right where it was authored. A warning
would join the list of warnings this project has already watched being ignored until they became
defects, so the gate refuses and names the element and screen.

---

## 5. Modbus register block

The sensor blocks occupy 100–419 (`SENSOR_1_BASE_ADDR = 100`, `SENSOR_BLOCK_SIZE = 40`,
eight sensors). Registers 420–499 are left free so the sensor count can grow. The network
block is therefore placed at **500**.

Strings are packed two characters per register, high byte first, `NUL`-padded, not
`NUL`-terminated when exactly filling the field.

| Range | Field | Type |
| --- | --- | --- |
| 500 | `NET_WIFI_ENABLED` | bool |
| 501 | `NET_WIFI_STATE` | enum, read-only |
| 502 | `NET_WIFI_RSSI` | int16, read-only |
| 503–508 | `NET_WIFI_IP`, `NET_WIFI_MAC` | packed, read-only |
| 510–525 | `NET_WIFI_SSID` | text, 32 bytes |
| 526–557 | `NET_WIFI_PSK` | text, 64 bytes, **write-only** |
| 560 | `NET_MQTT_ENABLED` | bool |
| 561 | `NET_MQTT_STATE` | enum, read-only |
| 562 | `NET_MQTT_PORT` | uint16 |
| 563 | `NET_MQTT_PUBLISH_PERIOD_S` | uint16 |
| 564 | `NET_MQTT_FLAGS` | bitfield: TLS, HA discovery |
| 570–601 | `NET_MQTT_HOST` | text, 64 bytes |
| 602–617 | `NET_MQTT_USER` | text, 32 bytes |
| 618–633 | `NET_MQTT_PASSWORD` | text, 32 bytes, **write-only** |
| 634–657 | `NET_MQTT_BASE_TOPIC` | text, 48 bytes |
| 658–673 | `NET_MQTT_DISCOVERY_PREFIX` | text, 32 bytes |
| 674 | `NET_PORTAL_ENABLED` | **SPECIFIED, NOT BUILT** — reserved. bool, the STA-side config page ONLY (R7.9). Never touches the radio. No constant declares it in `net_register_map.h`; a read returns whatever the unclaimed address returns |
| 675 | `NET_PORTAL_REMAINING_S` | uint16, read-only — seconds left on the AP/portal timer |
| 676–691 | `NET_AP_SSID` | text, 32 bytes, read-only (§5.2) |
| 692–707 | `NET_AP_PASSWORD` | text, 32 bytes, read-only (§5.2) |
| 708–709 | `NET_AP_IP` | packed, read-only — the portal address. **TWO** registers: `kApIp = 708` |
| 711 | `NET_AP_REQUEST` | **SPECIFIED, NOT BUILT** — reserved. bool, raise or drop the provisioning AP (R5.4a). 711 is genuinely free, verified against the header rather than assumed |
| 712–719 | `NET_PORTAL_USER` | text, 16 bytes |
| 720–729 | *reserved* | formerly `NET_PORTAL_PASSWORD`; writes ignored — see the note below |
| 730 | `NET_APPLY` | write `0x5AA5` to commit the staged block |
| 731 | `NET_REVISION` | increments on each successful apply |
| 732 | `NET_LAST_ERROR` | enum, read-only |
| 736–751 | `NET_PORTAL_PASSWORD` | text, 32 bytes, **write-only** |

> **TWO ROWS ABOVE DESCRIBE REGISTERS THAT DO NOT EXIST, and are labelled so** (DF15). `674`
> (`NET_PORTAL_ENABLED`, R7.9) and `711` (`NET_AP_REQUEST`, R5.4a) are decided features with addresses
> chosen and nothing implemented: neither appears in `net_register_map.h`. They stay in this table, marked,
> rather than being deleted — deleting them would discard a requirement and let a future block reuse an
> address that is already spoken for. An integrator reads this table to decode real hardware, so a row that
> silently describes nothing is the worst outcome of the three.
>
> **`NET_AP_IP` is 708–709, not 708–711.** The header declares `kApIp = 708` over two registers, packed;
> `kPortalReset` is at 710 and `kPortalUser` at 712. The old four-register range double-booked 711 against
> `NET_AP_REQUEST` **inside this same table**, while R5.4a's own decision note said 711 was chosen
> *because* it is free. The header was right and the table was wrong in two places at once.
>
> The `static_assert`s in `net_register_map.h` cover overlaps between things that EXIST; they cannot catch
> an address that exists only on paper, which is why this note is the gate for these two rows.

> **The portal password is at 736, not 720.** It was placed at 720 with 16 registers for its 32 bytes
> and did not have them: `NET_APPLY` is at 730, so registers 720–729 staged only bytes 0..19, a write
> aimed at byte 20 landed on the apply register and **committed the block**, and 731/732 were ignored
> as read-only. So a 32-byte password could be set from the web form and never over RS485 — against
> the rule that RS485 is the source of truth for everything. The field moved rather than the apply
> protocol, because 730–732 are the three addresses this table already published and a master may be
> written against them. 720–729 is left reserved so a master built against the old prose is ignored
> rather than writing into the middle of another field, and `net_register_map.h` now `static_assert`s
> that no text field overlaps a scalar or runs past the end of the block.

> **One frame does not reach the whole region.** FC16 carries its byte count in a single byte, so the
> protocol caps one write at 123 registers. A master zero-filling or configuring the whole block
> necessarily issues a SEQUENCE of frames; §5.1's requirement holds per frame.

### 5.2. Remote setup over RS485 is a first-class path, not a side effect

Specified by the project owner on 2026-08-03: a remote operator on the bus should be able to enable
WiFi, retrieve the AP information, and do the same for MQTT.

This works out well because **the register block already IS the model** — §3.2 requires one
implementation, so a Modbus write and a display edit are the same operation. What the original
block was missing is the *readback* half. Enabling WiFi remotely is useless if you cannot then see
what the device is waiting for, so five registers are added above.

**Two distinct remote flows, and the second is the more useful one:**

| | |
| --- | --- |
| **Fully remote** | Write `NET_WIFI_SSID` and `NET_WIFI_PSK`, apply. The device joins the network; read `NET_WIFI_IP` to confirm. Then write the MQTT block and apply again. **No AP, no portal, no site visit.** |
| **Remote-assisted** | Write `NET_WIFI_ENABLED` = 1 with no credentials. The device raises its AP. Read `NET_AP_SSID`, `NET_AP_PASSWORD` and `NET_AP_IP` and read them out to whoever is on site, who joins and completes provisioning in a browser. |

The first is the answer for a device on a bus you already reach. The second is for handing off to
someone standing at a device you cannot see — which is precisely why the AP password has to be
*readable* rather than only displayed on the panel.

> **R5.3** — `NET_AP_SSID`, `NET_AP_PASSWORD` and `NET_AP_IP` are **readable**, and are the only
> credential registers that are. They describe an access point the device is currently offering,
> which is not a secret the device is keeping — it is one it is broadcasting the existence of, and
> any radio in range already sees the SSID.
>
> This is a deliberate asymmetry with §5.1: the WiFi PSK and MQTT password remain **write-only**,
> because those are the *operator's* secrets and the device has no business handing them back. The
> AP password is the *device's* own, generated per-device from the MAC, and disclosing it to whoever
> already controls the bus grants nothing they could not do anyway — they can write credentials
> directly.

> **R5.4** — `NET_PORTAL_ENABLED` is writable over RS485, so the sequence "provision the network
> remotely, then open the config page and browse to it" is available without ever touching the
> device. Combined with `NET_PORTAL_REMAINING_S` a supervisory system can see the window closing.
>
> **R5.4a — THE AP WINDOW GETS ITS OWN REGISTER, `NET_AP_REQUEST` AT 711** (owner, decision 6B,
> 2026-08-04).
>
> Two different intentions were sharing register 674, and the code had picked the wrong one. §5's
> table defines 674 as the **STA-side config page** — the form you browse to once the device is
> already on your network (R7.9). §5.2's remote-assisted flow needs something else entirely: "write
> `NET_WIFI_ENABLED` = 1 with no credentials, the device raises its AP", so a remote operator can read
> `NET_AP_SSID`/`NET_AP_PASSWORD` and read them out to somebody standing on site.
>
> `WifiManager::requestApPortal()` cited R5.4 and treated a write to 674 as "raise the AP". The
> consequence, found in review: **a write to 674 dropped a working link** — a supervisory system
> opening the config page on a happily-associated device would knock it off its network to raise an
> access point nobody asked for.
>
> One register per intention. 674 toggles the config page and is inert with respect to the radio; 711
> raises and drops the AP. 711 was chosen because it is free and sits beside `NET_PORTAL_RESET` at
> 710 in the portal section — verified against the block map rather than assumed.
>
> Writing 0 to 711 drops the AP early, which is the manual counterpart to R7.6's ten-minute timeout:
> a remote operator who realises they raised it by mistake should not have to wait the window out.

> **R5.5a — ONE APPLY PATH, ACCEPTED WITH ITS CONSEQUENCE** (owner, 2026-08-03). R5.5's single
> apply path means an apply promotes **every** pending field, not only the one the caller touched.
> So if a master is midway through writing a multi-register field when an operator commits any
> setting at the panel, the master's partial value is promoted with it.
>
> Per-surface staging was considered and rejected: it would contradict R5.5, triple the storage for
> every field, and need merge semantics of its own. So would refusing a panel commit while a master
> is mid-write, which lets an off-site party block the panel in front of somebody's face.
>
> **The accepted behaviour is: concurrent configuration from two surfaces is not supported, and the
> last apply wins.** This is a documented characteristic, not a defect to be fixed later. It needs
> two parties configuring one device within seconds of each other to occur at all.
>
> One narrower case WAS fixed rather than accepted, because it failed in the opposite direction:
> the read-modify-write on the shared flags register (564) used to rebuild from LIVE, which
> *discarded* a master's staged bits instead of promoting them. It now reads staged, so it composes.
> See `NetRegisterMap::mqttFlagsStaged`.
>
> **R5.5** — Every one of these is the SAME staged write the display uses: fields, then `0x5AA5` to
> `NET_APPLY`. A 32-register SSID must not cause sixteen reconnection attempts, and a partially
> written MQTT block must not be half-applied. There is one apply path and the portal, the display
> and the bus all go through it.

**The security position, stated rather than assumed.** Writing credentials over Modbus RTU sends
them in clear text over an unauthenticated bus — already noted in §8's R8.2, and unchanged by this.
What R5.3 adds is that reading the AP password over the same bus is *also* clear text. Both are
acceptable for the same reason: **the RS485 pair is already a trust boundary.** Anyone who can read
it can also write it, and anyone who can write it can already repoint the broker or change the
network. Adding readback does not widen the surface; it makes an existing level of access useful.
What it does mean is that a deployment where the bus is not trusted should not enable WiFi from it —
and that belongs in the operator documentation, not in a mitigation this firmware can apply.

> **R5.1** — Reading a write-only field returns zeros. It must not return the stored
> secret, and it must not raise an exception either, because a master doing a block read
> across the region should not fail.
>
> **R5.2** — Applying a change that leaves the device unable to associate does **not** roll
> back automatically. This deliberately differs from the RS485 link block: losing WiFi does
> not cost you the connection you are configuring over, so an automatic revert would fight a
> user who is deliberately moving the device to a new AP. `NET_LAST_ERROR` reports the
> failure instead.

Growing the bank from 420 to 752 registers costs about **664 bytes** of RAM
(`register_bank.h` stores one `uint16_t` per register). Against 327 KB — 8.2 % used today — that is
not a consideration, and it buys full remote configurability.

---

## 6. Settings catalogue additions

### 6.1. New settings

**Thirteen**, of which seven are text.

> **Corrected 2026-08-04.** This table listed fourteen, including `config.mqtt.tls` — which
> **Q3/R8.3 had already ruled out**, in the same document, on the grounds that "a toggle that does
> nothing implies protection that is not there". I implemented the table without noticing it
> contradicted the decision, so the setting reached the catalogue, bit 2 of register 564, the menu
> and the portal before the contradiction was caught in review. The decision wins; the setting is
> removed. Bit 2 of 564 is left **reserved rather than reused**, so a master written against the
> interim build cannot silently come to mean something else.

| Binding | Kind | Range / length | Default | Note |
| --- | --- | --- | --- | --- |
| `config.wifi.enabled` | Boolean | — | off |
| `config.wifi.ssid` | **Text** | 32 | empty |
| `config.wifi.psk` | **Text**, write-only | 63 | empty |
| `config.mqtt.enabled` | Boolean | — | off |
| `config.mqtt.host` | **Text** | 64 | empty |
| `config.mqtt.port` | Numeric | 1–65535 | 1883 |
| `config.mqtt.user` | **Text** | 32 | empty |
| `config.mqtt.password` | **Text**, write-only | 32 | empty |
| `config.mqtt.baseTopic` | **Text** | 48 | `watermeter/<mac>` |
| `config.mqtt.discoveryPrefix` | **Text** | 32 | *per §4.4* |
| `config.mqtt.publishPeriod` | Numeric | 1–3600 s | 10 |
| `config.mqtt.haDiscovery` | Boolean | — | on |
| `config.mqtt.qos` | Enum | 0, 1 | 0 | **TELEMETRY only** (8A). Discovery and availability are always QoS 1 regardless — a lost reading is superseded by the next one within `publishPeriod`, whereas a lost discovery or availability message is not self-correcting and must not inherit a best-effort setting chosen for telemetry. |

### 6.2. What must change to support text settings

Concretely, in the units decision **D2** just made Arduino-free:

1. `SettingKind` gains `Text`.
2. `SettingDescriptor` gains `maxLength`, and a `writeOnly` flag for secrets.
3. A parallel accessor pair — `readSettingText` / `writeSettingText` — because overloading
   the `int32_t` API would mean every caller has to know which kind it holds. The numeric
   API stays exactly as it is.
4. `formatSetting` gains a text arm that masks when `writeOnly`.
5. `adjustSetting` does **not** gain a text arm. Text is not adjusted by a delta — and since §6.3
   removed on-device text entry entirely, nothing replaces it either: `min`, `max` and `step` are
   all zero for a text setting because none of the three means anything for a string.
6. The generated manifest gains `maxLength` and `writeOnly`, and emits `type: "string"`.
   `tools/manifest_gen` and `shared/schemaDefinitions.ts` follow — and because the manifest
   is now generated (**D2**), the new gate `firmware-manifest-resolvable` will *force* a
   resolver case for every one of these before the export will pass. The gates built
   yesterday will police this feature without further work.

### 6.3. There is no on-device text editor

**Decided 2026-08-03 by the owner, reversing R3.3.1.** A character-wheel editor was specified,
built (a 97-position ring, 47 host checks) and wired into the UI before being removed. The
judgement that killed it: *three buttons and a wheel is insane to handle and manage.* That is
correct, and the arithmetic §3.3 already contained says so — a 63-character WPA2 passphrase averages
around 24 presses per character, so a realistic passphrase is on the order of a thousand button
presses, performed while standing at a wall-mounted panel, with the characters visible on screen.

> **R6.3.1** — Text settings are **never editable at the panel.** No editor screen, no new screen
> kind, no new actions. The character wheel and its engine are deleted rather than left dormant:
> dead code that advertises a capability is worse than absent code, because the next person reads it
> as available.
>
> **R6.3.2** — Text settings **are displayed** at the panel, so an operator can read which network
> and which broker the device is configured for without a laptop. This is the diagnostic half of
> §7.1's information pages at zero input cost.
>
> **R6.3.3** — A `writeOnly` text setting renders as a **fixed run of asterisks** — not one
> asterisk per character. The length of a passphrase is information too, and a wall-mounted display
> has no access control. A never-set field renders `(not set)`, which is distinguishable from a
> field that failed to read.

**Where text actually comes from.** All three were already specified; removing the wheel does not
create a gap, it removes a bad fourth option:

| Surface | Requirement | Reaches the device how |
| --- | --- | --- |
| Configuration web portal | §7.6 | Join the provisioning AP, fill in a form |
| RS485 register block | §5.2, R5.5 | A Modbus master writes the fields and applies |
| SD credential file | Q2 | Drop a file on the card the menu-pack reader already mounts |

**This makes the portal load-bearing rather than a convenience** (§7.6, slice N8a). It is now the
only way to provision a device that has no Modbus master attached, so it moves up in priority and
its failure modes matter more. The bootstrap has no chicken-and-egg: `config.wifi.enabled` is a
**boolean**, so it is settable at the panel; enabling it with no credentials raises the AP; the AP's
name and password are readable on the AP info page and over RS485 (R5.3).

**Consequence for the completeness rule.** `Loadable_UI_Menu_Packs` §3.0.1 requires every
`category: "setting"` value to have a reachable editor, and `assertCoversEverySetting` enforces it —
that rule is what forced the wheel into existence. It now **exempts `type: "string"`**, and the
exemption is safe for the same reason guarded editors were rejected under R7.3: it is decided by
**kind**, which is statically knowable, not by a runtime condition. The gate still proves that every
setting an operator can change at the panel has an editor there.

---

## 7. The menu tree

Specified by the project owner on 2026-08-03. **WiFi is never enabled automatically and AP mode is
never entered automatically** — both are consequences of a setting the operator changed on the
display. A radio that switches itself on is a radio the owner did not consent to.

### 7.1. Shape

Two new entries at the **root level**, siblings of the existing info pages, reachable by paging
UP/DOWN like everything else:

```
L0  P0 Global status … P6 Factory reset ─► WIFI ─► MQTT ─► (wraps to P0)
    nine ring entries: seven info pages plus these two network roots
                                            │        │
L1  ┌───────────────────────────────────────┘        └──────────────────────────┐
    W1 Enabled              true/false   editor       M1 Enabled       true/false   editor
    W2 Network (SSID)       read-only ✎                M2 Broker  ──────────────────┐
    W3 Passphrase           read-only ✎ masked         M.BACK                       │
    W4 Reset portal login   3 s hold ─► confirm                                     │
    W.BACK                                                                          │
                                                                                    │
L2  ┌───────────────────────────────────────────────────────────────────────────────┘
    B1  Broker host      read-only ✎        B7   Publish period   editor  1-3600 s
    B2  Port             editor  1-65535    B8   HA discovery     editor  on/off
    B3  Username         read-only ✎        B9   TLS              editor  on/off
    B4  Password         read-only ✎ masked B10  QoS              editor  0/1
    B5  Base topic       read-only ✎        B.BACK
    B6  HA prefix        read-only ✎
```

`✎` marks a value that is **displayed but not editable here** — §6.3 removed on-device text entry, so
every text field is a read-only row whose footer reads "Set via web portal or RS485". Secrets show a
fixed run of asterisks. Five of the ten broker rows are read-only, which is why the entry is called
**Broker** rather than "Broker setup": a name promising setup would be half a lie, and the row the
descent lands on is one of the read-only ones.

**Not built yet, and deliberately so.** The three information pages of the original shape — `AP
info`, `WiFi info`, `MQTT info` — are absent. They display association state, the DHCP address and
the AP password, none of which exist as catalogue values until **N4** builds the state machine that
produces them. Emitting them earlier would mean binding ids the resolver cannot serve (which the
`firmware-manifest-resolvable` gate rejects) or placeholder text impersonating status. They arrive
with the state they describe.

**No entry is guarded.** The original shape marked the information pages `⟨only when configured⟩`
and the broker descent `⟨only when enabled⟩`. Guards are specified in R7.2 and still unimplemented;
since this slice emits only editors and read-only rows, leaving everything unconditional keeps the
completeness rule statically decidable and costs nothing but showing a broker row before MQTT is
switched on — which is the friendlier order anyway, since it lets an operator configure before
enabling.

These are **ordinary dataset screens**, not firmware-drawn like the Select Menu page. So a menu
pack can restyle or relocate them, which is what the owner asked for. The Select Menu page is
firmware-drawn because it is the recovery route (§3.4.1 of the pack requirement); these are not,
and should not be special-cased.

> **R7.1** — WiFi and MQTT entries follow the navigation contract of
> `Display_UI_Requirements` §3 without exception: UP/DOWN page siblings, ENTER-short descends,
> ENTER-long escapes to P0, each level ends with a `BACK` entry, and every editor commits on
> ENTER-short and discards on ENTER-long.

### 7.2. Conditional entries need flow guards, which do not exist yet

The owner's shape is explicitly state-dependent — "if enabled is true and there are no settings
then there is a menu item called AP info". Every screen in the tree today is unconditional, so
this is new machinery.

**The vocabulary already exists and is unimplemented.** `ScreenFlow.guard` is in the web schema
(`src/types.ts:94`, `schemaDefinitions.ts:113`) and `Flow::guard` is in the emitted C++ struct —
and every guard in the current dataset is `nullptr`, with nothing in `interaction_handler.cpp`
ever reading it. This feature is the reason to implement it.

> **R7.2** — A flow may carry a `guard`: a catalogue value id evaluated at navigation time. When
> the guard resolves false the flow is skipped, so paging steps over the entry as if it were not
> there and the ring closes without it.

> **R7.3 — A GUARD MAY HIDE AN INFORMATION PAGE. IT MAY NEVER HIDE AN EDITOR.**
>
> This is the constraint that keeps the design honest, and it comes from the completeness rule
> (`Loadable_UI_Menu_Packs` §3.0.1): every `category: "setting"` value must have a **reachable**
> editor, and the export gate has to decide that statically. A guarded editor is only
> conditionally reachable, and no static check can evaluate a runtime guard — so allowing it would
> silently turn the completeness rule into a rule about nothing.
>
> Consequently `Enabled` is always present in both submenus, and only the *information* pages and
> the MQTT `Setup` descent are guarded. A setting is therefore always reachable: enable the
> feature and its editors appear, and `Enabled` itself is never hidden.

### 7.3. What each page shows

**Guard conditions**, as catalogue values so they are testable and bindable:

| Guard | True when |
| --- | --- |
| `wifi.enabled` | `config.wifi.enabled` is set |
| `wifi.configured` | an SSID is stored (Q13, decided 2026-08-03: **stored, not association-proven**) |
| `mqtt.enabled` | `config.mqtt.enabled` is set |
| `mqtt.configured` | a broker host is stored |

**WIFI ▸ AP info** — shown when enabled and *not* configured, which is the state that means "the
operator wants WiFi and has not told us which network". Displays what someone needs to reach the
portal: the AP's SSID, its password (§7.4), and the portal address. Nothing here is editable.

**WIFI ▸ WiFi info** — shown when configured. The SSID, the connection state (§3.1's ASCII
vocabulary), the DHCP-assigned IP, and the RSSI. Read-only: changing the network is done through
the portal, RS485 or the SD file — never from a status page, and never at the panel (§6.3).

> **R7.12** — "Configured" means an SSID is **stored**, not that association ever succeeded (Q13).
> So this page appears the moment credentials exist, showing `CONN`, `RETRY` or `FAIL` and
> `(waiting)` for the IP. That is deliberate: the page is most needed when the device is *not*
> connecting, and gating it on a successful association would hide the only diagnostic the operator
> has at exactly the wrong moment.

**MQTT ▸ Setup** — shown when enabled. Shows **the address to browse to** (§7.6) plus a `Portal`
toggle, and descends to the L2 editors as the fallback. The password editor is `writeOnly`, so it
renders masked per §6.3's R6.3.1.

**MQTT ▸ MQTT info** — shown when configured. Broker host and port, username, connection state,
and the password as `********` when one is set or `(not set)` when not. Never the password itself.

> **R7.4** — A page that shows an IP address must show something meaningful before one is
> assigned. `0.0.0.0` reads as a fault; `(waiting)` reads as a state. Same for RSSI before
> association.

### 7.6. The configuration web page, for WiFi *and* MQTT

Specified by the project owner on 2026-08-03: MQTT is configured through a web page too, for the
same reason WiFi is. That is the right call and it generalises further than it first appears —
MQTT's fields are **worse** to type than a passphrase. A broker host is `homeassistant.local` or
`192.168.1.50`, a base topic is `watermeter/plant-3/inlet`, and §3.3 measured the character wheel
at roughly 27 presses per character. Five such fields on three buttons is not a user interface.

> **R7.14 — The site also serves a read-only status view** (Q15, decided 2026-08-03): live sensor
> readings, WiFi and MQTT state, the last MQTT error, and the polling rate. Every value is already
> in the catalogue and the server is already running, so it is close to free — and "why will MQTT
> not connect" is far easier to answer in a browser than on a 240×135 panel with three buttons.
>
> Now that R7.9 makes the server permanent this is worth more than it was when the page existed for
> ten minutes at a time: it becomes the ordinary way to look at the device.

> **R7.8** — One page serves both. It presents a WiFi section (scan, pick, passphrase) and an MQTT
> section (host, port, username, password, base topic, discovery prefix), and submits to the same
> settings catalogue every other input surface writes to.

**Where it is reachable from changes with the state, and that is the useful part:**

| WiFi state | Page served on | Address the menu shows |
| --- | --- | --- |
| Enabled, not configured | the provisioning **AP** | the AP's own address, e.g. `192.168.4.1` |
| Configured and associated | the operator's **network**, at the DHCP address | that address |

So the flow is: enable WiFi → join the device's AP → set the network → the device joins it → now
browse to the device on your own LAN and set up MQTT. MQTT setup needs a working network anyway
because the broker is on it, so serving that page over STA is not a compromise — it is the only
sequence that makes sense. Re-entering AP mode to configure MQTT would disconnect the device from
the very network the broker lives on.

The **MQTT info** and **AP info** pages already show the IP, which is exactly what a browser needs.

> **R7.9 — THE WEB SERVER IS ALWAYS ACTIVE WHEN WIFI IS, AND EVERY PAGE IS BEHIND A LOGIN.**
> Decided 2026-08-03 (Q14), replacing an earlier draft that had the page off by default with a
> ten-minute timer.
>
> The model is the conventional one for this class of device — a router, a network camera, a PLC
> gateway: the server runs whenever the device is on a network, and authentication rather than
> availability is what protects it. **WiFi is the on/off switch**, and MQTT has its own; the server
> itself is not separately gated. That is simpler to explain and simpler to use than a window the
> operator has to keep re-opening, and it makes the diagnostic value of §7.6 permanently available
> rather than available for ten minutes at a time.
>
> The consequence to be clear-eyed about: the device is reachable by **every host on the operator's
> network** for as long as WiFi is enabled, so the login is the entire defence. Which makes the next
> two requirements load-bearing rather than decorative.

> **R7.9a — A DEFAULT PASSWORD MUST BE CHANGED, NOT MERELY CHANGEABLE.**
>
> The device ships with `admin`, which is standard practice and also the single most exploited
> pattern in embedded equipment. What separates a device that survives that from one that does not is
> whether the default is *allowed to persist*. So:
>
> - Until the password has been changed, **every page shows a persistent warning** naming the risk,
>   and the change-password form is what the login lands on rather than the status page.
> - The device's own display says so too: `config.portal.passwordDefault` is a derived value, and
>   the WiFi info page shows `PASSWORD: DEFAULT` while it is unchanged. An operator who never opens
>   a browser still finds out.
> - It is **not** forced — a locked-out device on a wall is worse than a weakly-protected one on a
>   home LAN — but it is impossible to miss.
>
> This is the one place this document argues with the "standard practice" it is following: the
> practice that produced Mirai was default passwords that nothing ever nagged about.

> **R7.9b — The login gates every page, including on the AP.**
>
> One barrier, no exceptions, because an exception is what someone finds. The default password is
> displayed on the **AP info page** alongside the AP's WPA2 key (R7.5), for the same reason: an
> operator standing at the device can read it, and someone who is not standing there cannot. First
> provisioning therefore needs no prior knowledge, and no page is left unprotected to achieve that.
>
> Credentials are `config.portal.user` (default `admin`) and `config.portal.password`
> (default `admin`, `writeOnly`). Being `writeOnly` they read back as zeros over Modbus per §5.1 —
> the device's own login is the operator's secret, unlike the AP key of R5.3.

> **R7.9c — The page carries EVERY setting, and is generated from the catalogue.**
>
> The owner asked for "a settings page where all the settings can be set up". That is 24+ settings
> today — sensor calibration, the Modbus link block, LED behaviour, WiFi, MQTT — and hand-writing a
> form per setting would guarantee the page drifts from the catalogue the moment one is added.
>
> So the form is **generated from the same catalogue the manifest is generated from** (decision D2).
> Each `SettingDescriptor` already carries everything a form control needs: `kind` selects the
> widget, `min`/`max`/`step` bound a numeric, `options` populate a select, `maxLength` sizes a text
> input, `writeOnly` renders it as a password field. Adding a setting to the firmware catalogue adds
> it to the web page with no HTML written — the same property that makes the manifest trustworthy.
>
> This also means the page cannot offer a setting the firmware does not have, or omit one it does.

> **R7.9d — Three fields on that page are NOT generated, and the clock is the third** (N-d1, built
> 2026-08-18).
>
> The portal login pair has no descriptor because R7.9a needs the login to land on the change-password
> form. The clock has none for a different and sharper reason: catalogue values are `int32_t`, and a Unix
> epoch outgrows that in **2038**. Making the clock a `SettingDescriptor` would have shipped a Y2038 bug
> into the one subsystem whose entire subject is being right about time, so `config.clock.epoch` is
> rendered and parsed explicitly as `uint32_t`.
>
> **What the section shows:** the device time as `YYYY-MM-DD HH:MM:SS UTC` — UTC, and it says so, because
> the device has no timezone and inventing one makes every timestamp ambiguous — plus **who set it**
> (`ClockSource`), because a reader deciding whether to trust a timestamp needs that answered too. An unset
> clock says *not set* rather than rendering a plausible 1970.
>
> **How it is set, and why not `datetime-local`.** That widget submits a local wall-clock string with no
> offset, so the firmware would have to parse a date AND guess a timezone — the two things the Modbus block
> at 50–52 was designed to avoid. Instead a small inline script asks the BROWSER, which knows both, to
> compute the epoch, and the firmware receives the same `uint32_t` it receives from RS485: one code path,
> one validation, one plausibility floor. The script only prefills, so with scripting off the field still
> submits and the hint tells the operator to type seconds.
>
> **Blank means leave it alone**, exactly like the write-only password: the page renders every setting on
> every save, and a blank clock field must never read as "set the clock to 1970". An out-of-range year is
> reported as `OutOfRange` before the clock is troubled at all, and a refusal by `DeviceClock` surfaces as
> `Refused`.
>
> **Dependency-inverted, like the settings store.** `PortalForm` is Arduino-free so its whole surface stays
> host-testable, and `DeviceClock::setTime` needs `millis()`; `PortalClockWriter` is the seam, implemented in
> `firmware.cpp`. A null writer renders the section saying the build supplied no clock rather than hiding it,
> because a section that vanishes is indistinguishable from a firmware that has no clock.

> **R7.10** — The page is served over **HTTP**. So the login password, the MQTT broker password and
> the WiFi passphrase all cross the LAN in clear text whenever the form is submitted.
>
> This must be documented for the operator, not quietly accepted. TLS is not a real option here: a
> self-signed certificate produces a browser warning that trains people to click through warnings,
> which is worse than the exposure it would fix.
>
> **And R7.9's change to an always-on server removes the mitigation the earlier draft relied on.**
> A ten-minute window bounded the exposure; a permanent server does not. What remains is the login
> (R7.9a/b) and the fact that the device is on a LAN behind NAT rather than on the internet. That is
> a reasonable posture for a home or plant network and an inadequate one for a device exposed to the
> world — so the operator documentation must say, in as many words: **do not port-forward this
> device.**

> **R7.11** — Submitting the form is a **staged write followed by an apply**, exactly as §5's
> register block requires. A form POST must not leave half a configuration live: the fields are
> staged, validated together, then committed. This is the same protocol a Modbus master uses and
> the same one the display editors use — the portal earns no special path.

**The on-display editors stay.** The completeness rule (§2.3) requires every `category: "setting"`
value to have a reachable editor, and R7.3 forbids hiding an editor behind a guard. So the L2 text
editors remain the fallback — for a site with no phone or laptop to hand, and for the case where
the portal itself will not come up. The web page is the path anyone will actually use; the editors
are the path that is always there.

### 7.4. The AP is not open, and it does not stay up

AP mode activating on "enabled but unconfigured" is the owner's specified behaviour and it is the
right default — it makes an unprovisioned device self-service. But an open access point appearing
on an industrial device that may sit on a wall unattended is an exposure worth closing, and it can
be closed almost for free:

> **R7.5a — THE AP IS NAMED `water_flow_meter_<n>`** (owner, 2026-08-03), where `<n>` is a
> per-device number derived from the MAC.
>
> Derived rather than freshly random **per boot**, and that distinction is load-bearing: R5.3 has a
> remote operator read `NET_AP_SSID` over RS485 and read it out to somebody standing on site. If the
> name changed on every boot, the value they read could be stale by the time it is spoken, and two
> devices on one wall could swap identities between power cycles. A MAC-derived suffix looks exactly
> as arbitrary to a human, stays put, and needs no storage.
>
> The name is deliberately recognisable rather than opaque: someone scanning for networks on a plant
> floor should be able to tell which access point is the meter they were sent to configure.
>
> **R7.5** — The provisioning AP is **WPA2-protected with a per-device password**, derived from
> the MAC and displayed on the AP info page. An operator standing at the device reads it off the
> screen; someone who is not standing there cannot. This costs one line of `softAP()` argument and
> removes "anyone in radio range can reconfigure it" entirely.
>
> **R7.13 — NTP SYNCS THE RTC ON ASSOCIATION** (Q9, decided 2026-08-03). One query when WiFi comes
> up, written to the RX8130CE at 0x32. It is the first thing that has ever set that clock.
>
> Two constraints from elsewhere in this document apply and are easy to overlook: the RTC is on the
> **sensor I²C bus** (§2.1.2), so it is written once on association and never polled — R2.1.6 covers
> it. And the query must not block the logic loop, so it goes through the same task the MQTT client
> uses.
>
> Not exposed over Modbus for now: nothing consumes a wall-clock time yet — the MQTT payloads carry
> no timestamps and Home Assistant stamps arrivals itself. The registers can be added when something
> needs them.

> **R7.6** — The AP shuts down after **10 minutes** without a completed provisioning, and on
> success. A portal that stays up forever is a portal nobody remembers is running. The countdown
> is shown on the AP info page.
>
> **R7.7 — While the AP is up the LEDs show a slow blue pulse** (Q11, decided 2026-08-03):
> `LedOverride::ApPortal`, on at `kApPortalPeriodMs` and off for the same, blue only.
>
> Blue already carries the network association in §3's vocabulary, and a slow single-channel pulse
> is distinguishable from everything else there: never solid (reset accepted), never accelerating (a
> countdown), and never two channels alternating (`CardBusy`). It is also distinguishable from
> §3.3's *blue blink on flow*, which is a short blink on a pulse rather than a steady rhythm — but
> that is the one collision worth testing for, so the host test asserts the two periods differ.

### 7.5. Consequences for the pack format

- **The catalogue grows by 14 settings and ~10 derived values.** Per the rule in
  `ui_value_catalogue.h`, additions alone do not require an ABI bump — but the completeness rule
  means existing packs become incomplete, which is exactly what **Q4** (catalogue versioning) has
  to answer before this ships.
- **The skeleton generator must emit these screens**, and `assertCoversEverySetting` will refuse to
  generate until every new setting has an editor. That gate is what will keep this honest.
- **Guards enter the pack format now, as `formatVersion` 2** (Q12, decided 2026-08-03). `PackFlow`
  gains a `guardStr` offset alongside `actionStr`. The reader already refuses a version it does not
  recognise, so a version-1 pack meets a version-2 firmware as a clean `BadFormatVersion` rather
  than as a flow whose guard field is read out of whatever follows it. The round-trip test compares
  every flow field, so the emitter and reader cannot drift apart on the new field either.
  Deferring would have meant shipping the WiFi tree with dead entries and revisiting every screen.

## 8. Security

This feature puts credentials on a device that previously held none, and gives it an
outbound network path. That deserves stating plainly rather than burying.

> **R8.1** — The WiFi PSK and MQTT password are write-only everywhere: not readable over
> Modbus (§5.1), not shown on the display (§3.4.3), not logged to serial, and not published
> over MQTT.
>
> **R8.2** — Writing credentials over Modbus RTU sends them **in plaintext over an
> unauthenticated bus**. Anyone with physical access to the RS485 pair can read them. This
> is inherent to Modbus RTU, not a defect we can fix here, and it must be documented for the
> operator rather than quietly accepted. The same applies to credentials in a file on a
> removable SD card (§3.3).
>
> **R8.3 — TLS IS OUT OF SCOPE FOR THIS VERSION (Q3, decided 2026-08-03).** MQTT runs over plain
> TCP, so the broker password and all telemetry cross the network in clear text.
>
> This is a deliberate choice for a device whose broker is on the same LAN, and it must be stated in
> the operator documentation rather than left to be discovered. `config.mqtt.tls` is **not** added
> as a setting: a toggle that does nothing is worse than its absence, because it implies protection
> that is not there. Adding TLS later is a catalogue addition plus a certificate story, and the
> §5 register block leaves room in `NET_MQTT_FLAGS` for it.
>
> What this does NOT excuse: R7.10's clear-text config page is mitigated by being off by default
> with a timeout. Nothing similar mitigates MQTT, because it runs continuously by design. A
> deployment that cannot accept a plain-text broker credential on its network should not enable
> MQTT until TLS exists.
>
> **R8.4** — A factory reset must erase the credentials. A device leaving one owner's hands
> must not carry their WiFi passphrase.

---

### 8.2. Losing the portal password must not cost the totals

**Decided 2026-08-03 by the owner.** The portal login ships as `admin`/`admin` and is changed
through the portal itself; §6.3 removed the only path that could have edited it at the panel. That
left one recovery: a factory reset, which also erases cumulative volume and every sensor's
calibration. Paying for a forgotten password with the measurement record is not a trade anybody
would choose deliberately, so it is not left as a consequence.

> **R8.2a** — The portal login is resettable to `admin`/`admin` from **the menu** and **over
> RS485** (`NET_PORTAL_RESET` = 710, write `0x5AA5`). The reset touches **only** the two portal
> fields; totals, calibration, WiFi and MQTT settings are untouched.
>
> **R8.2b** — Both paths act **immediately**, without a separate apply. Every other write in this
> block stages and waits for `0x5AA5` at `NET_APPLY`, and this one deliberately does not: it is
> reached by somebody who is already locked out and improvising, and a recovery step that silently
> needs a follow-up write is one they will conclude did not work.
>
> **R8.2d** — The menu path sits in the **WiFi level** (`W4 Reset portal login`), behind a **3 s
> hold-to-confirm** with an acknowledgement toast that reads `LOGIN: admin/admin`.
>
> The WiFi level because the portal is reached through the AP the radio raises, and because §6.3
> left the panel with no other way to influence the portal at all. 3 s rather than the factory
> reset's 30 s because nothing is destroyed and it is fully reversible — but not a single press
> either, since it does drop the device to a published default. The toast names the credential
> instead of saying "done": the operator has to go and use it, and a toast that omits it leaves them
> guessing what changed.
>
> **R8.2c** — The revision still bumps, so a master polling `NET_REVISION` can tell the command
> landed, and `portalPasswordIsDefault()` returns true again — which re-raises the §7.9a nag rather
> than leaving the device quietly back on a shipped default.

**The owner's rationale was "physical access to the device should recover it", and the menu path is
what delivers that.** The RS485 path is a convenience — and it is worth being precise that it costs
nothing in exposure: `NET_PORTAL_PASSWORD` (at 736; see §5) is already **writable**, so anyone who can reach
register 710 could already set the login to a value of their own choosing. The command adds
discoverability and an expressible intention ("restore the known default" is not the same operation
as "set this string"), not a new capability.

What the magic requirement buys is the accident case rather than the attacker case: §5.1 requires a
block write across the whole region to succeed rather than except, so a master zero-filling 500–751
must not silently reset the login on its way past. A host check zero-fills all 233 registers and
asserts the login survives.

---

## 9. Failure modes

| Condition | Required behaviour |
| --- | --- |
| No credentials configured | Radio stays powered down. Display shows `OFF`. Not an error. |
| Wrong PSK | Backoff to the 5 min ceiling; `FAIL` on the display; `NET_LAST_ERROR` set. Never a reboot loop. |
| AP disappears | `Retrying`. Modbus, counting and UI entirely unaffected (§3.1.3). |
| Broker unreachable | WiFi stays `OK`, MQTT shows its own state. The two are reported independently because they fail independently. |
| Broker rejects credentials | Distinct error from unreachable. "It doesn't work" is not a diagnosis. |
| Publish queue full | Drop oldest telemetry; never drop availability or discovery (§4.1.3). |
| Flash write during publish | Publishing must never be in the persistence path (§4.3.3). |
| Brownout on TX peak | See §9.3. |

### 9.3. Power

WiFi transmit peaks draw substantially more current than the device has ever needed. If the
StampPLC's regulator or the installation's supply cannot deliver it, the symptom is a
brownout reset — which looks like a firmware crash and will be debugged as one.

> **R9.3.1** — Supply headroom must be checked on real hardware before this feature is
> considered done, and the check recorded. A brownout under TX load is the kind of fault
> that wastes days because the code looks wrong.

### 9.4. Budget

Current usage: RAM 7.9 % of 327 680 B, flash 18.1 % of 3 342 336 B. WiFi plus an MQTT
client is a significant addition to both; TLS more so. The precise figures and whether TLS
fits are being confirmed (§11 Q3).

---

## 10. Acceptance criteria

Nothing here is satisfied by "it compiles".

| # | Criterion | Verifiable without hardware? |
| --- | --- | --- |
| A1 | `pollingRate_kHz` with the radio associated is within 5 % of the radio-off baseline (§2.1) | **No** — needs hardware |
| A2 | Home Assistant shows one device with all entities, no YAML written | **No** — needs a broker and an HA instance |
| A3 | Cumulative volume drives HA's Water dashboard | **No** |
| A4 | Entities go *unavailable* within the keep-alive window of unplugging the device | **No** |
| A5 | Credentials set over Modbus, from the portal and from the SD file produce identical stored state. (**Was** "on the display and over Modbus" — the display no longer sets credentials at all, §6.3, so that comparison no longer exists) | Partly — the staging logic is host-testable |
| A6 | A text setting round-trips through the register block, including exact-length and empty cases | **Yes** — host test |
| A6b | The catalogue contains no setting the requirement has ruled out — checked by the portal form's coverage walk, which compares against `settingCount()` rather than a literal | **Yes** — host test |
| A7 | Reading a write-only register returns zeros and does not except | **Yes** — host test |
| A8 | **No text setting is editable at the panel**: none has an editor screen, each still renders (masked when secret), and the descend handler refuses to open one (§6.3) | **Yes** — host test |
| A9 | The default menu satisfies the completeness rule with the enlarged catalogue | **Yes** — export gate |
| A10 | Every new catalogue value resolves in `UiBindingResolver` | **Yes** — `firmware-manifest-resolvable` |
| A11 | Discovery payloads are not truncated by the client's buffer | **Yes** — host test on the serialiser |
| A12 | No non-ASCII reaches a display-bound value (§4.6) | **Yes** — export gate |
| A13 | Factory reset erases credentials | Partly |

A11 deserves note: an MQTT client's default buffer is often smaller than a Home Assistant
discovery payload, and the failure mode is that the publish is silently dropped and no
entity ever appears, with nothing reporting an error. It is exactly the kind of silent gate
failure this project has been fixing all week, and it should be tested before it is
experienced.

---

## 11. Open questions

| # | Question | Options | Recommendation |
| --- | --- | --- | --- |
| ~~**Q1**~~ | Can WiFi be kept off core 0 under `framework = arduino`? | **ANSWERED 2026-08-03 by measurement, not decision — see §2.1.3.** Split: the WiFi task moves with `-DCONFIG_ESP32_WIFI_TASK_PINNED_TO_CORE_1` because its core is a runtime field; lwIP's `tiT` cannot move, its affinity is a literal compiled into `liblwip.a`. So R2.1.0 cannot be fully satisfied under this framework. |
| ~~**Q2**~~ | Which bulk credential path? | **DECIDED 2026-08-03: the web page**, for WiFi *and* MQTT — see §7.6. Supersedes this question's own recommendation of the SD card, and §3.3 records why that reasoning was wrong. |
| ~~**Q3**~~ | Is TLS in scope for v1? | **DECIDED 2026-08-03: (b) plaintext only.** `config.mqtt.tls` is deliberately NOT added — a toggle that does nothing implies protection that is not there. See R8.3. |
| ~~**Q4**~~ | Does the W:/M: indicator go on P0 or in the footer? | **DECIDED 2026-08-03: (b) footer.** Also puts it on every info page rather than only P0. |
| ~~**Q5**~~ | Should the exporter reject non-ASCII in display-bound text? | (a) Hard failure; (b) warning; (c) no check | **(a)** — it renders as garbage on Font0 and looks fine in the mockup, which is the worst combination. |
| **Q6** | Publish per-sensor JSON, or a topic per value? | (a) JSON per sensor; (b) topic per value | **(a)**, and already written into §4.2 as the specified behaviour with its reasoning — 8 publishes per cycle instead of 40, consumed natively by HA via `value_template`. **Not separately put to the owner**, so it stands as a specified default rather than a ratified decision; say so if (b) is wanted. |
| ~~**Q7**~~ | What happens to packs authored before the catalogue grew? | **DECIDED 2026-08-03: (a) break them.** No live product, so backwards compatibility is not worth paying for; the skeleton generator IS the migration. Revisit the moment a customer has authored a pack — see §2.3. |
| ~~**Q8**~~ | Should MQTT accept commands (reset session, etc.)? | **DECIDED 2026-08-03: (b) yes**, against my recommendation to defer. Specified in §4.4.1 with two safeguards the decision needs: an explicit `RESET` payload (the project's `0x5AA5` idiom) and unconditional rejection of RETAINED command messages, which would otherwise wipe the totals on every reconnect forever. |
| ~~**Q9**~~ | Sync the RX8130CE RTC from NTP once WiFi exists? | **DECIDED 2026-08-03: (a) yes**, on association only, not polled — the RTC is on the sensor I²C bus (R2.1.6). Not exposed over Modbus yet. See R7.13. |
| ~~**Q10**~~ | Should `wifi.rssi` be a Home Assistant entity? | (a) Yes, diagnostic category; (b) display only | **(a)** — it is the first thing anyone looks at when a device drops. |
| ~~**Q11**~~ | Which LED pattern for "AP portal up"? | (a) A new override; (b) reuse `CardBusy`'s amber/blue; (c) no LED signal | **(a)** — reuse would make a provisioning AP indistinguishable from a card read, and §3 of the LED spec is deliberately unambiguous. Needs a shape distinct from solid, accelerating, single-channel and amber/blue. |
| ~~**Q12**~~ | Do guards belong in the pack format now or later? | **DECIDED 2026-08-03: (a) now, `formatVersion` 2.** See §7.5. |
| ~~**Q13**~~ | Is `wifi.configured` "SSID stored" or "SSID stored AND association succeeded once"? | (a) SSID stored; (b) association proven | **(a)** — (b) would hide the WiFi info page precisely when the operator most needs to see why it is not connecting. |
| ~~**Q14**~~ | **REDESIGNED 2026-08-03.** The answer changed the model rather than picking an option: the web server is always active while WiFi is, every page sits behind a login defaulting to `admin`/`admin`, the password is changeable from a settings page that carries every setting, and WiFi and MQTT have their own on/off switches. See R7.9 and R7.9a-c. Original question: | (a) No — rely on R7.9's off-by-default plus the timeout; (b) yes, the same per-device password as the AP; (c) HTTP basic auth with an operator-set password | **(b)** — the per-device password already exists for the AP and is already shown on screen, so reusing it costs nothing and closes "any host on the LAN can repoint the broker during the ten-minute window". (a) leaves that window genuinely open. |
| ~~**Q15**~~ | Does the portal serve a live status page as well as the forms? | (a) Forms only; (b) forms plus a read-only status page | **(b)** — the information is already in the catalogue, the page is already being served, and "why is MQTT not connecting" is far easier to answer on a browser than on a 240×135 panel. Cheap, and it makes the ten-minute window useful for diagnosis rather than only for configuration. |

---

## 12. Implementation slices

Ordered so that each slice is verifiable when it lands, and so the riskiest unknown is
resolved before the expensive work.

| Slice | Content | Gate | State |
| --- | --- | --- | --- |
| **N0** | **Spike:** WiFi associated, task pinned off core 0, measure `pollingRate_kHz` against baseline. No MQTT, no UI. | A1. If this fails, §2.1 forces a redesign — which is exactly why it is first. | ⛔ needs hardware |
| **N1a** | `SettingKind::Text`, accessor pair, `maxLength`, `writeOnly`, manifest and schema changes | A6, A7 | ✅ |
| **N1b** | `NetSettings` — nine text fields, staged/apply, revision, secret masking | A6, A7 | ✅ 50 checks |
| **N1c** | Declare the 14 settings in the catalogue | A6, A7 | ✅ manifest regenerated |
| ~~**N2a**~~ | Text-editor engine — **built then deleted** (§6.3). Its 47 checks went with it. | A8 | ⊘ withdrawn |
| **N2b** | Text settings are **display-only** at the panel; the wiring was removed with the engine | A8 | ✅ 16 checks |
| **N3** | Network register block 500–751, staged apply, revision, error reporting | A5, A6, A7 | ✅ 50 checks |
| **N4** | WiFi state machine, backoff, NVS persistence, status bindings | A13 | ▶ next — also unblocks the §7.1 info pages |
| **N5** | MQTT client (`esp-mqtt`, §4.1.1), topic layout, cadence, LWT, queue policy | A11 | unblocked 2026-08-03 |
| **N6** | Home Assistant discovery payloads (§4.4.a/b) and republish rules | A2, A3, A4 | unblocked 2026-08-03 |
| **N7a** | Default-pack editors for all 14 settings, regenerated `.uipack` | A9, A12 | ✅ 34 screens, 82 total |
| **N7b** | Menu screens with flow guards, completeness migration (Q7) | A10 | |
| **N8a** | Configuration web portal (§7.6) — `WebServer` + `DNSServer`, catalogue-generated form. **Load-bearing since §6.3**: with no wheel this is the only way to provision a device with no Modbus master. | A9 | ▶ raised in priority |
| **N8b** | SD-card credential file (Q2) | — | |
| **N9** | Hardware validation: polling rate, brownout under TX, HA end-to-end, **task-WDT survival with the portal active** | A1–A4, R9.3.1 | ⛔ needs hardware |

**Why N1c and N7a are one change, not two.** `assertCoversEverySetting` in the exporter refuses to
emit a pack that does not reach every declared setting. Declaring the settings without their
editors therefore breaks the build, and adding editors for settings that do not exist is not
expressible. The two land together with a regenerated default pack, because these settings ship
*in* the provisioned default menu rather than being customer-added.

**N0 is not optional and must not be reordered.** Every slice after it assumes the answer to
§2.1 is favourable. If it is not, N1–N8 would be built on a premise the device disproves,
and the accuracy of the measurement is the reason this product exists. The host-verifiable slices
are being built first only because N0 needs hardware that is not to hand; nothing after N0 is
*shippable* until it has run.

**What the host suite cannot check, and why it is not faked.** §7 requires a button press to be
acknowledged on screen within 100 ms. The host suite now asserts the *scheduling* half of that
directly — measured from the gesture's completion, and mutation-tested by removing the
screen-change repaint trigger to confirm the check goes red. What it cannot assert is whether the
*work* in one repaint fits inside 100 ms on an ESP32-S3 driving a real SPI panel: host wall-clock on
x86 is not a proxy for that, and timing it there would read as covered while validating nothing.
So N9 owns the frame-cost measurement, and the host suite instead bounds the per-frame draw-primitive
count — a tripwire for the order-of-magnitude change that turns a comfortable budget into a missed
deadline.

**Why the WDT item is on N9.** `CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU0=y` with
`CONFIG_ESP_TASK_WDT_PANIC=y` and a 5 s timeout means core 0's idle task must get scheduled. The
polling loop is a tight loop with no `vTaskDelay`; it yields only because `readPlcInput` blocks on
I²C. Adding lwIP traffic on the same core eats that margin, so "does it still boot with the portal
open" is a real question with a panic as its failure mode.
