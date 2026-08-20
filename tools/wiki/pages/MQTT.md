# MQTT and Home Assistant

The device publishes telemetry to a broker and announces itself to Home Assistant. It does not accept
commands over MQTT — see [What MQTT cannot do](#what-mqtt-cannot-do) before you plan around it.

**Where the detail lives.** The behaviour is specified in
`docs/Requirements/feature addition/WiFi_MQTT_Connectivity.md` §4, and implemented in
`src/net/mqtt_publisher.cpp` (topics, cadence, queue) and `src/net/ha_discovery.cpp`
(discovery payloads). Both headers carry the reasoning for every constant named here. This page is the
integrator's view: what appears on the wire and what you can rely on.

## Identity

Everything is derived from the last three bytes of the WiFi MAC, so it is stable across reboots and
unique per board. Read the MAC from network register `505`.

| | Shape | Example |
| --- | --- | --- |
| Client id | `wfm-<mac-suffix>` | `wfm-a1b2c3` |
| Default base topic | `watermeter/<mac-suffix>` | `watermeter/a1b2c3` |
| Home Assistant node id | `wfm_<mac-suffix>` | `wfm_a1b2c3` |

A generated client id would have been simpler and wrong: two devices sharing one kick each other off
the broker in a loop, and from the device end that looks like an unreliable network rather than a
collision.

The base topic is configurable (network register `634`, the web portal, or the panel). It is **refused
rather than repaired** if it contains `+`, `#`, an interior `//`, or a leading or trailing `/` —
silently stripping a wildcard would publish to a topic the operator did not type and cannot find.

## Topics

`<base>` is the base topic; `<prefix>` is the discovery prefix, default `homeassistant`.

| Topic | Retained | Payload |
| --- | --- | --- |
| `<base>/status` | yes | `online` / `offline` — the availability topic and the broker's will message |
| `<base>/sensor/<1..8>/state` | no | one JSON object per channel |
| `<base>/total/state` | no | the aggregate across all active channels |
| `<base>/diagnostics/state` | no | device health |
| `<prefix>/sensor/<node_id>/<object_id>/config` | yes | one Home Assistant discovery message per entity |
| `<prefix>/button/<node_id>/<object_id>/config` | yes | one discovery message per command button |
| `<prefix>/status` | — | **subscribed**, not published: Home Assistant's birth message |
| `<base>/cmd/reset-session` | **never** | **subscribed** — payload `RESET` |
| `<base>/cmd/reset-totals` | **never** | **subscribed** — payload `RESET` |
| `<base>/cmd/republish` | **never** | **subscribed** — any payload |

One payload per channel rather than one per value — eight publishes a cycle instead of forty. Radio
airtime competes with the pulse counter for the same I²C bus, so this is a measurement decision, not a
tidiness one. Home Assistant reaches each field with a `value_template`.

### Payloads

```jsonc
// <base>/sensor/3/state
{"flow":12.340,"session":1234.56,"total":1.234567,"max":18.900,"pulses":123456}
//  L/min, 3dp      L, 2dp         m³, 6dp         L/min, 3dp   count

// <base>/total/state
{"flow":45.670,"session":9876.54,"total":12.345678,"sensors":8}
//                                                  active channels

// <base>/diagnostics/state
{"pollingRateKhz":3.310,"baselineKhz":3.400,"undersampling":0,"tempC":41.2,"uptimeS":86400,"rssi":-57}
```

Two things to note. **Flow is always L/min on the wire**, whatever the panel is set to show — the
display unit (`REG_DISPLAY_FLOW_UNIT`, register `33`) is a screen preference and never rescales a
payload. And a non-finite value is emitted as JSON `null`, not `nan`: `nan` is not valid JSON, so Home
Assistant would drop the whole message and the entity would stop updating with nothing logged.

`pollingRateKhz` travels with `baselineKhz` deliberately. The live rate alone cannot show a
regression, and a sampling regression that is only visible in a lab is the failure the whole
measurement budget is written to prevent. `undersampling` is a bitmap: bit *n* set means channel *n+1*
is pulsing faster than the sampler can count, so its readings are low.

## Cadence

- **Publish period** — configurable 1..3600 s, default 10 (register `563`).
- **Change detection** — a state topic is only published when its serialised payload differs from the
  last one sent on that topic. Comparing the rendered text rather than the fields means "unchanged"
  is exactly what a subscriber would see, so a value that moves by less than its printed precision
  correctly counts as no change.
- **Heartbeat** — every 60 s a full set is published regardless, bypassing both the change detection
  and the rate limit. A consequence worth naming: with the period set above 60, the heartbeat becomes
  the effective cadence, because it is a bound on staleness rather than a preference.
- **On reconnect** — a full set is republished, because a reconnect is exactly when a subscriber's
  picture is emptiest and change detection would otherwise suppress every unchanged value.

## QoS is decided by the message, not by the setting

`config.mqtt.qos` — bit 1 of register `564` — applies to **telemetry only**.

| Class | QoS | Why |
| --- | --- | --- |
| Telemetry | as configured (default 0) | A lost reading is superseded within one publish period, so best-effort is a legitimate choice. |
| Availability | 1, always | A dropped `online` leaves Home Assistant showing values from a device that is gone. |
| Discovery | 1, always | A dropped config message is an entity that never appears at all. |

Neither of the last two is self-correcting, so neither inherits a best-effort setting chosen for
readings. The configured QoS is not consulted on those paths even as a floor it could raise — a floor
reads as harmless and still leaves their QoS a function of a setting that speaks only for telemetry.

## When the queue fills

Sixteen slots. If a message arrives with no room, the **oldest telemetry** is evicted, whatever class
the newcomer is. If the queue holds no telemetry to evict — all availability and discovery — the
incoming message is **rejected** instead, because evicting a discovery message loses an entity
permanently while dropping a reading loses one sample.

Overlong topics and payloads are refused, never truncated: a truncated topic publishes *successfully*
to the wrong place, so the broker accepts it and nothing reports a fault. The publisher counts drops,
rejections and failures, and remembers the last failed topic — all of it because the natural failure
mode here is silence.

## Commands

Home Assistant can act on the device, not only observe it. Three topics, discovered as `button`
entities so they appear as buttons rather than as switches implying a state to be in:

| Topic | Payload | What it does |
| --- | --- | --- |
| `<base>/cmd/reset-session` | `RESET` | the session volume and peak of every in-use channel — register `22`'s command |
| `<base>/cmd/reset-totals` | `RESET` | that, and the lifetime totals — register `21`'s command |
| `<base>/cmd/republish` | anything | re-sends every discovery message |

**A reset takes the exact payload `RESET`.** Anything else — including an empty payload — is ignored
and reported. This is the ASCII form of the `0x5AA5` magic the destructive registers use: without it,
any stray publish to the topic zeroes a customer's totals.

**A reset is rate-limited to one per minute per kind, and that is the primary guard.** The owner's
rule: *a reset failing remotely is not a breaking thing; the device entering a reset loop is.* So every
ambiguous case swallows the command. The limit is measured on uptime, never on the wall clock — an NTP
sync can move the clock backwards and would silently re-arm it — and the epoch of the last accepted
reset is *also* persisted, because the loop that matters most is the one that reboots and starts
`millis()` again from zero. With no trusted time the persisted half cannot fire and the uptime half
carries it alone, which still covers every loop that does not reboot.

**A retained command message is discarded unconditionally**, before the payload is even looked at. A
retained `RESET` would otherwise wipe the totals on every reconnect, forever. It is a fault to clear at
the broker, so it is reported as such rather than silently rate-limited.

**A refusal is visible.** The outcome of the last command — `idle`, `accepted`, `rate-limited`,
`retained-ignored`, `bad-payload` or `unknown-command` — appears as `lastCmd` in the diagnostics
payload, in register `565`, and on the panel's *MQTT publish* page. A button that silently does
nothing is worse than one that says why. A topic under `cmd/` that names no command is reported as
`unknown-command` rather than dropped, so a typo in an automation is visible to whoever made it.

**A command is acknowledged by the telemetry it causes**, not by a reply topic: an accepted command
publishes everything immediately instead of waiting for the next period.

Commands are accepted only while MQTT is enabled and connected, and are **never queued** — one that
arrives during a disconnect is lost, which is correct: a reset asked for two hours ago is not one still
wanted. And a reset goes through the same holding-register write a Modbus master performs, so a reset
from Home Assistant, from the panel's confirm screen and from the bus are one implementation.

> **The security position, stated plainly.** This makes the broker a control path into a metering
> device: anyone able to publish to it can zero the totals. The magic payload stops accidents, not
> intent. That is an accepted trade for a device on a home network, and it is why nothing that changes
> *configuration* is reachable over MQTT — a reset is recoverable by re-reading the meter, a repointed
> broker or a changed calibration is not.

## Home Assistant discovery

31 entities on a fully populated device: three per channel, four device-wide, and three buttons.

| Entity | `object_id` | JSON key | Device class | Unit | State class |
| --- | --- | --- | --- | --- | --- |
| Flow | `s<n>_flow` | `flow` | `volume_flow_rate` | L/min | `measurement` |
| Session volume | `s<n>_session` | `session` | `water` | L | `total_increasing` |
| Lifetime volume | `s<n>_lifetime` | `total` | `water` | m³ | `total_increasing` |
| Board temperature | `board_temperature` | `tempC` | `temperature` | °C | `measurement` |
| Polling rate | `polling_rate` | `pollingRateKhz` | — | kHz | `measurement` |
| Under-sampling | `undersampling` | `undersampling` | — | — | `measurement` |
| WiFi RSSI | `wifi_rssi` | `rssi` | — | dBm | `measurement` |
| Reset session | `cmd_reset_session` | — (button) | — | — | — |
| Reset lifetime totals | `cmd_reset_totals` | — (button) | — | — | — |
| Republish discovery | `cmd_republish` | — (button) | — | — | — |

The three buttons carry `command_topic` and `payload_press` in place of a state topic, and none of the
reading keys — Home Assistant rejects a payload carrying keys the component does not define, and a
rejected payload is an entity that never appears with nothing logged. They are published under
`<prefix>/button/`, not `<prefix>/sensor/`, and they exist regardless of the connected-sensor bitmap: a
device with nothing wired up can still be told to republish.

The four diagnostics carry `entity_category: diagnostic`; the buttons deliberately do not, or they
would be folded away in the device page's diagnostics section rather than being available to a
dashboard. `unique_id` is `<node_id>_<object_id>`. Lifetime
volume is the one to put on the Water dashboard — `water` + `total_increasing` + m³ is what the energy
and water panels look for.

Every entity carries `suggested_display_precision`, because without it Home Assistant infers zero
decimals and renders a litres-per-minute reading as a bare integer.

**Discovery is republished** on first connect, on every reconnect, when Home Assistant's birth message
arrives on `<prefix>/status`, when `<base>/cmd/republish` is pressed, and when the connected-sensor
bitmap changes — that last one is how a
channel wired up after commissioning gets its entities without a reboot.

The device subscribes to `<prefix>/status` and to `<base>/cmd/#` *before* announcing itself, so a birth message arriving in
the same instant is not missed. Its own availability topic also carries `online`, and that is
deliberately not treated as a birth — otherwise every availability publish would trigger a full
31-entity republish.

## What MQTT cannot do

**Nothing that changes configuration.** The three commands above reset measurements and re-send
discovery; there is no MQTT route to a calibration, a broker address, a topic or a link setting. That
is a deliberate boundary, not a gap — see the security note under [Commands](#commands).

TLS is out of scope. Bit 2 of the flags register briefly carried a toggle for it and was retired
rather than reused, because a toggle that does nothing implies protection that is not there.

## Configuring it

Three routes, all writing the same settings:

1. **The web portal** — join the provisioning AP, see [WiFi](WiFi).
2. **Modbus** — the network register block, addresses `560`–`672`. See
   [Modbus registers](Modbus-Registers); note that writes stage and register `730` commits.
3. **The panel** — Configuration → Modbus / Display for local settings. The panel is a **reader** of
   WiFi and MQTT configuration, not an editor: there is no on-device text entry, so the SSID, broker
   host and topics can only be set over the portal or over Modbus.
