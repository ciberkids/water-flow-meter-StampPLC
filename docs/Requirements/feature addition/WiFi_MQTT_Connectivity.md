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
| MQTT **command** topics (reset session from Home Assistant) | Accepting control from the network is a materially different security posture than publishing telemetry. Worth doing, separately, once the read path is proven. |
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
| Arduino event + main loop | 1 already | n/a |

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
| `SettingKind` | `{ Numeric, Enum, Boolean }` — no text kind |
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

So the load path already degrades gracefully. The export path does not, and it should not
silently — but neither should a catalogue addition retroactively break packs a user authored
last month. §7 specifies how this is handled, and **Q7** asks for a decision on it.

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

That is not a good experience, and this document is not going to pretend otherwise. It is
still **required**, because the user asked for display configuration and because it is the
only path that works with nothing but the device in your hand.

> **R3.3.1** — The character-wheel editor is required and specified in §6.3.
>
> **R3.3.2** — At least one bulk-entry path must also exist, so that nobody is *forced*
> through R3.3.1. See **Q2** for which. The candidates, with honest costs:

| Route | UX | Cost | Security exposure | Fails how |
| --- | --- | --- | --- | --- |
| **Character wheel** (R3.3.1) | Poor but universal | Medium — new editor kind, new actions | PSK visible on screen while typing | Mis-typed; user retries |
| **Modbus write** | Good, if you have a Modbus master | Low — the block already needs to exist | **Plaintext over unauthenticated RS485** | Wrong value; rollback applies |
| **File on the SD card** | Good | **Low** — the card reader and parser already exist for menu packs | Plaintext on a removable card | File absent or malformed; report and stay disabled |
| **SoftAP + captive portal** | Best for a non-technical user | High — HTTP server, HTML, and the AP competes with §2.1 | Open AP during provisioning | Timeout, revert to `Disabled` |
| **WPS push-button** | Excellent when it works | Low | WPS is widely deprecated and often disabled in APs | Silently never associates |
| **SmartConfig / ESP-TOUCH** | Needs a vendor phone app | Medium | UDP broadcast of the PSK | Opaque failure |
| **BLE provisioning** | Good, needs an app | High, and BLE shares the radio with WiFi | Pairing model | — |

**Recommendation:** the **SD card file** as the bulk path. It is the cheapest by a wide
margin because `Loadable_UI_Menu_Packs.md` already puts a card reader, a filesystem and a
JSON parser in the firmware; this reuses all three. It also matches how the operator already
configures this device — by preparing a card at a desk. SoftAP is the better product answer
and the wrong first answer.

### 3.4. What the display shows

> **R3.4.1** — The main screen (`info-p0-global-status`) shows a compact combined indicator:
> WiFi state and MQTT state, in that order, in the ASCII vocabulary of §3.1 and §4.5.
>
> **R3.4.2** — A WiFi detail screen shows SSID, state, IP address, and RSSI.
>
> **R3.4.3** — The passphrase is **never** displayed in full anywhere, including on its own
> editor screen, where only the character under the cursor is shown in clear (§6.3).

Space on P0 is already contended: the footer is a deterministic four-row stack and the
240×135 display is full. The combined indicator must therefore be terse — `W:OK M:OK` is
9 characters. **Q4** asks whether it earns its place on P0 or belongs in the footer.

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

*Pending verification against the current Home Assistant documentation — the exact
`device_class`, `state_class` and unit strings, the discovery topic structure, and what is
required for the Water dashboard are being checked against primary sources rather than
written from memory, because HA changes these between releases and a wrong string produces
an entity that silently never appears.*

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
> **R4.4.5** — Discovery messages are **retained**, so HA re-discovers after its own restart
> without the device republishing.
>
> **R4.4.6** — Discovery is republished on every reconnect, and whenever the connected-sensor
> bitmap changes, so enabling a sensor makes its entity appear.

### 4.5. Availability

> **R4.5.1** — A Last Will and Testament on `<base>/status` set to `offline`, with an
> `online` publish on connect, both retained. Entities must show as *unavailable* in Home
> Assistant when the device drops — silently stale values are worse than a visible gap,
> because a flow meter reading zero and a flow meter that is switched off look identical.

### 4.6. Text on the display is ASCII only

The generated UI renders with M5GFX **Font0**, whose glyph table covers codepoints 0–255 and
whose lookup increments above 176. Status strings, state names and error text destined for
the display must therefore stay in printable 7-bit ASCII. No `✓`, no `✗`, no degree signs.

This is easy to get wrong precisely because it works in the web mockup, which renders in a
browser font with full Unicode. The exporter should reject non-ASCII in any element bound to
these values (**Q5**).

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
| 690 | `NET_APPLY` | write `0x5AA5` to commit the staged block |
| 691 | `NET_REVISION` | increments on each successful apply |
| 692 | `NET_LAST_ERROR` | enum, read-only |

> **R5.1** — Reading a write-only field returns zeros. It must not return the stored
> secret, and it must not raise an exception either, because a master doing a block read
> across the region should not fail.
>
> **R5.2** — Applying a change that leaves the device unable to associate does **not** roll
> back automatically. This deliberately differs from the RS485 link block: losing WiFi does
> not cost you the connection you are configuring over, so an automatic revert would fight a
> user who is deliberately moving the device to a new AP. `NET_LAST_ERROR` reports the
> failure instead.

Growing the bank from 420 to ~700 registers costs about 560 bytes of RAM
(`register_bank.h` stores one `uint16_t` per register). Against 327 KB, that is not a
consideration.

---

## 6. Settings catalogue additions

### 6.1. New settings

Fourteen, of which seven are text:

| Binding | Kind | Range / length | Default |
| --- | --- | --- | --- |
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
| `config.mqtt.tls` | Boolean | — | off |
| `config.mqtt.qos` | Enum | 0, 1 | 0 |

### 6.2. What must change to support text settings

Concretely, in the units decision **D2** just made Arduino-free:

1. `SettingKind` gains `Text`.
2. `SettingDescriptor` gains `maxLength`, and a `writeOnly` flag for secrets.
3. A parallel accessor pair — `readSettingText` / `writeSettingText` — because overloading
   the `int32_t` API would mean every caller has to know which kind it holds. The numeric
   API stays exactly as it is.
4. `formatSetting` gains a text arm that masks when `writeOnly`.
5. `adjustSetting` does **not** gain a text arm. Text is not adjusted by a delta; §6.3
   replaces that model rather than extending it.
6. The generated manifest gains `maxLength` and `writeOnly`, and emits `type: "string"`.
   `tools/manifest_gen` and `shared/schemaDefinitions.ts` follow — and because the manifest
   is now generated (**D2**), the new gate `firmware-manifest-resolvable` will *force* a
   resolver case for every one of these before the export will pass. The gates built
   yesterday will police this feature without further work.

### 6.3. The text editor

A new screen kind. It reuses the existing gesture contract's shape without violating it:

| Gesture | In a text editor |
| --- | --- |
| UP / DOWN short | Previous / next character in the charset at the cursor |
| UP / DOWN held | Accelerating scrub through the charset, using the existing tiers |
| ENTER short | Advance the cursor one position; at the `END` pseudo-character, **commit** |
| ENTER long | **Discard** and ascend — same meaning as every other editor |
| UP + DOWN short | Reserved by §3 for display-off. **Not** available as backspace. |

Backspace is the `DEL` pseudo-character in the charset, not a gesture — the gesture space is
full, and overloading `UP + DOWN` would break a contract that works from every screen at
every depth.

> **R6.3.1** — A `writeOnly` text setting shows `*` for every committed character and the
> character under the cursor in clear. Typing a passphrase blind on a 3-button device is not
> reasonable; showing the whole thing on a wall-mounted display is not either.

---

## 7. The menu tree

Specified by the project owner on 2026-08-03. **WiFi is never enabled automatically and AP mode is
never entered automatically** — both are consequences of a setting the operator changed on the
display. A radio that switches itself on is a radio the owner did not consent to.

### 7.1. Shape

Two new entries at the **root level**, siblings of the existing info pages, reachable by paging
UP/DOWN like everything else:

```
L0  P0 Global status … P8 Factory reset ─► WIFI ─► MQTT ─► (wraps to P0)
                                            │        │
L1  ┌───────────────────────────────────────┘        └──────────────────┐
    Enabled            true/false  (boolean editor)                     Enabled        true/false
    AP info            ⟨only when enabled and NOT configured⟩           MQTT info      ⟨only when configured⟩
    WiFi info          ⟨only when configured⟩                           Setup          ⟨only when enabled⟩
    BACK                                                                BACK
                                                                         │
L2                                                                       └─► Broker host   (text)
                                                                             Port          (numeric)
                                                                             Username      (text)
                                                                             Password      (text, masked)
                                                                             Base topic    (text)
                                                                             BACK
```

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
| `wifi.configured` | an SSID is stored — the operator has provisioned it |
| `mqtt.enabled` | `config.mqtt.enabled` is set |
| `mqtt.configured` | a broker host is stored |

**WIFI ▸ AP info** — shown when enabled and *not* configured, which is the state that means "the
operator wants WiFi and has not told us which network". Displays what someone needs to reach the
portal: the AP's SSID, its password (§7.4), and the portal address. Nothing here is editable.

**WIFI ▸ WiFi info** — shown when configured. The SSID, the connection state (§3.1's ASCII
vocabulary), the DHCP-assigned IP, and the RSSI. Read-only: changing the network is done through
the portal or the text editors, not from a status page.

**MQTT ▸ Setup** — shown when enabled. Descends to the L2 editors. The password editor is
`writeOnly`, so it renders masked per §6.3's R6.3.1.

**MQTT ▸ MQTT info** — shown when configured. Broker host and port, username, connection state,
and the password as `********` when one is set or `(not set)` when not. Never the password itself.

> **R7.4** — A page that shows an IP address must show something meaningful before one is
> assigned. `0.0.0.0` reads as a fault; `(waiting)` reads as a state. Same for RSSI before
> association.

### 7.4. The AP is not open, and it does not stay up

AP mode activating on "enabled but unconfigured" is the owner's specified behaviour and it is the
right default — it makes an unprovisioned device self-service. But an open access point appearing
on an industrial device that may sit on a wall unattended is an exposure worth closing, and it can
be closed almost for free:

> **R7.5** — The provisioning AP is **WPA2-protected with a per-device password**, derived from
> the MAC and displayed on the AP info page. An operator standing at the device reads it off the
> screen; someone who is not standing there cannot. This costs one line of `softAP()` argument and
> removes "anyone in radio range can reconfigure it" entirely.
>
> **R7.6** — The AP shuts down after **10 minutes** without a completed provisioning, and on
> success. A portal that stays up forever is a portal nobody remembers is running. The countdown
> is shown on the AP info page.
>
> **R7.7** — While the AP is up the LEDs must say so, and `CardBusy`'s amber/blue is taken —
> see **Q11** for which pattern.

### 7.5. Consequences for the pack format

- **The catalogue grows by 14 settings and ~10 derived values.** Per the rule in
  `ui_value_catalogue.h`, additions alone do not require an ABI bump — but the completeness rule
  means existing packs become incomplete, which is exactly what **Q4** (catalogue versioning) has
  to answer before this ships.
- **The skeleton generator must emit these screens**, and `assertCoversEverySetting` will refuse to
  generate until every new setting has an editor. That gate is what will keep this honest.
- **Guards must round-trip through the pack format.** `PackFlow` has no guard field today, so
  adding one is a `formatVersion` bump from 1 to 2 — the reader rejects a version it does not know,
  so old and new firmware cannot silently disagree.

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
> **R8.3** — MQTT without TLS sends the broker password in plaintext over the network. TLS
> is therefore offered (`config.mqtt.tls`), subject to the flash and RAM budget in §9.4. If
> TLS does not fit, that fact must be documented, not discovered.
>
> **R8.4** — A factory reset must erase the credentials. A device leaving one owner's hands
> must not carry their WiFi passphrase.

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
| A5 | Credentials set on the display and over Modbus produce identical stored state | Partly — the staging logic is host-testable |
| A6 | A text setting round-trips through the register block, including exact-length and empty cases | **Yes** — host test |
| A7 | Reading a write-only register returns zeros and does not except | **Yes** — host test |
| A8 | The text editor reaches every character in the charset, and `DEL`/`END` behave | **Yes** — host test |
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
| **Q1** | Can WiFi be kept off core 0 under `framework = arduino`? | (a) Build flags suffice; (b) needs a custom sdkconfig; (c) needs `framework = espidf`; (d) cannot be controlled | Being researched. If (d), report the measured cost and let the user decide — do not ship silently. |
| **Q2** | Which bulk credential path? | (a) SD card file; (b) SoftAP portal; (c) Modbus only; (d) SD now, SoftAP later | **(d)** — SD reuses the card reader, filesystem and JSON parser that menu packs already require. |
| **Q3** | Is TLS in scope for v1? | (a) Yes; (b) no, plaintext only; (c) only if it fits without a partition change | **(c)** — offer it if the budget allows, document plainly if not. |
| **Q4** | Does the W:/M: indicator go on P0 or in the footer? | (a) P0 body; (b) footer row; (c) both | **(b)** — P0 is full, and the footer stack is already deterministic. |
| **Q5** | Should the exporter reject non-ASCII in display-bound text? | (a) Hard failure; (b) warning; (c) no check | **(a)** — it renders as garbage on Font0 and looks fine in the mockup, which is the worst combination. |
| **Q6** | Publish per-sensor JSON, or a topic per value? | (a) JSON per sensor; (b) topic per value | **(a)** — 8 publishes per cycle instead of 40, and HA consumes it natively. |
| **Q7** | What happens to packs authored before the catalogue grew? | (a) Export fails until re-authored; (b) manifest carries a catalogue version and the exporter warns for values added after the pack's version; (c) completeness becomes per-category | **(b)** — keeps the rule's guarantee for new work without retroactively breaking old packs. |
| **Q8** | Should MQTT accept commands (reset session, etc.)? | (a) Not in v1; (b) yes | **(a)** — publishing is a much smaller security surface than accepting control. Worth a follow-up requirement. |
| **Q9** | Sync the RX8130CE RTC from NTP once WiFi exists? | (a) Yes; (b) no | **(a)** — cheap, and it makes the RTC useful for the first time. |
| **Q10** | Should `wifi.rssi` be a Home Assistant entity? | (a) Yes, diagnostic category; (b) display only | **(a)** — it is the first thing anyone looks at when a device drops. |
| **Q11** | Which LED pattern for "AP portal up"? | (a) A new override; (b) reuse `CardBusy`'s amber/blue; (c) no LED signal | **(a)** — reuse would make a provisioning AP indistinguishable from a card read, and §3 of the LED spec is deliberately unambiguous. Needs a shape distinct from solid, accelerating, single-channel and amber/blue. |
| **Q12** | Do guards belong in the pack format now or later? | (a) Now, `formatVersion` 2; (b) info pages always visible until then | **(a)** — (b) means shipping the tree with dead entries and revisiting every screen later. The version bump is cheap because the reader already rejects unknown versions. |
| **Q13** | Is `wifi.configured` "SSID stored" or "SSID stored AND association succeeded once"? | (a) SSID stored; (b) association proven | **(a)** — (b) would hide the WiFi info page precisely when the operator most needs to see why it is not connecting. |

---

## 12. Implementation slices

Ordered so that each slice is verifiable when it lands, and so the riskiest unknown is
resolved before the expensive work.

| Slice | Content | Gate |
| --- | --- | --- |
| **N0** | **Spike:** WiFi associated, task pinned off core 0, measure `pollingRate_kHz` against baseline. No MQTT, no UI. | A1. If this fails, §2.1 forces a redesign — which is exactly why it is first. |
| **N1** | Text settings: `SettingKind::Text`, the accessor pair, `maxLength`, `writeOnly`, manifest and schema changes, host tests | A6, A7 |
| **N2** | The text editor screen kind, charset, cursor, masking, new actions — **engine landed early**, see `ui/core/ui_text_editor.h` and its 47 host checks | A8 ✅ |
| **N3** | Network register block, staged apply, revision, error reporting | A5, A6, A7 |
| **N4** | WiFi state machine, backoff, NVS persistence, status bindings | A13 |
| **N5** | MQTT client, topic layout, cadence, LWT, queue policy | A11 |
| **N6** | Home Assistant discovery payloads and republish rules | A2, A3, A4 |
| **N7** | Menu-pack screens, default-menu extension, completeness migration (Q7) | A9, A10, A12 |
| **N8** | SD-card credential file (Q2) | — |
| **N9** | Hardware validation: polling rate, brownout under TX, HA end-to-end | A1–A4, R9.3.1 |

**N0 is not optional and must not be reordered.** Every slice after it assumes the answer to
§2.1 is favourable. If it is not, N1–N8 would be built on a premise the device disproves,
and the accuracy of the measurement is the reason this product exists.
