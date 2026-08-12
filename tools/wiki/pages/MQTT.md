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
| `<prefix>/status` | — | **subscribed**, not published: Home Assistant's birth message |

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

## Home Assistant discovery

28 entities on a fully populated device: three per channel plus four device-wide.

| Entity | `object_id` | JSON key | Device class | Unit | State class |
| --- | --- | --- | --- | --- | --- |
| Flow | `s<n>_flow` | `flow` | `volume_flow_rate` | L/min | `measurement` |
| Session volume | `s<n>_session` | `session` | `water` | L | `total_increasing` |
| Lifetime volume | `s<n>_lifetime` | `total` | `water` | m³ | `total_increasing` |
| Board temperature | `board_temperature` | `tempC` | `temperature` | °C | `measurement` |
| Polling rate | `polling_rate` | `pollingRateKhz` | — | kHz | `measurement` |
| Under-sampling | `undersampling` | `undersampling` | — | — | `measurement` |
| WiFi RSSI | `wifi_rssi` | `rssi` | — | dBm | `measurement` |

The last four carry `entity_category: diagnostic`. `unique_id` is `<node_id>_<object_id>`. Lifetime
volume is the one to put on the Water dashboard — `water` + `total_increasing` + m³ is what the energy
and water panels look for.

Every entity carries `suggested_display_precision`, because without it Home Assistant infers zero
decimals and renders a litres-per-minute reading as a bare integer.

**Discovery is republished** on first connect, on every reconnect, when Home Assistant's birth message
arrives on `<prefix>/status`, and when the connected-sensor bitmap changes — that last one is how a
channel wired up after commissioning gets its entities without a reboot.

The device subscribes to `<prefix>/status` *before* announcing itself, so a birth message arriving in
the same instant is not missed. Its own availability topic also carries `online`, and that is
deliberately not treated as a birth — otherwise every availability publish would trigger a full
28-entity republish.

## What MQTT cannot do

**There are no command topics.** The requirement (§4.4.1) specifies them — Home Assistant acting on
the device rather than only observing it — and they are **not implemented**. The only subscription is
`<prefix>/status`, for the birth message. Resets are reachable over Modbus (`REG_MASTER_RESET_*`, the
per-channel `OFF_CMD_*` offsets) and from the panel, and nowhere else.

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
