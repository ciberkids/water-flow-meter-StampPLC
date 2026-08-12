# Communications

Three ways to talk to the device, with different jobs. Pick by what you are doing.

| | Modbus RTU over RS485 | MQTT | WiFi |
| --- | --- | --- | --- |
| **For** | Control and integration | Observation and Home Assistant | Carrying MQTT, and provisioning |
| **Read telemetry** | yes | yes | — |
| **Write configuration** | yes, all of it | no | — |
| **Command a reset** | yes | **no** | — |
| **Works without a network** | yes | no | — |
| **Reference** | [Modbus registers](Modbus-Registers) | [MQTT](MQTT) | [WiFi](WiFi) |

**Modbus is the complete surface.** Every value the device measures and every setting it holds is
reachable over RS485 — including the WiFi and MQTT configuration, so a device can be provisioned
entirely remotely without anyone joining its access point.

**MQTT is read-only in practice.** Command topics are specified and not implemented; the only
subscription is Home Assistant's birth message. Anything that changes the device happens over Modbus
or at the panel.

**The panel is a reader of network configuration.** There is no on-device text entry, so an SSID or a
broker host cannot be typed at the device. It configures its own display behaviour and the Modbus
serial parameters, and reports everything else.

## Units, in one place

Flow is **litres per minute everywhere on every wire** — Modbus registers, MQTT payloads, Home
Assistant. Volume is published in both litres and cubic metres.

The panel has its own flow-unit setting (L/min, L/s, m³/h) and it changes **only what the screen
shows**. Register `33` reports what the panel is currently displaying, so a support call describing a
number that does not match the wire can be explained — but no integrator should ever rescale because
of it.

There is no fixed-point scaling anywhere. A flow register holds a real number, and multi-register
values are high word first.

## The apply protocol, common to both register blocks

Configuration writes **stage**; a magic value commits them.

```
1. write the value registers      -> staged, not in force
2. write 0x5AA5 to the apply reg  -> validated, then committed together
3. read the revision register     -> incremented, so the write is confirmed
```

`REG_LINK_APPLY` (44) commits the serial parameters; `kApply` (730) commits the network block. Staging
is what makes it safe to reconfigure a link over that same link — a slave id that applied before the
baud rate would strand the master mid-conversation.

Two things act immediately instead, both deliberately: the per-channel reset commands, and the
portal-login reset. A recovery action that needs a second write is one an operator gets half-way
through.

## Identity

Everything is derived from the last three bytes of the WiFi MAC, readable at register `505`:

- MQTT client id `wfm-<mac-suffix>`
- default base topic `watermeter/<mac-suffix>`
- Home Assistant node id `wfm_<mac-suffix>`
- provisioning AP `water_flow_meter_<6 digits>`

Derived rather than generated, so it survives a reboot and cannot collide with the meter next to it.

## Secrets

The WiFi passphrase, the MQTT password and the portal password are **write-only**: they read back as
zeros over Modbus and are never rendered on the panel or in the web page.

The provisioning AP's own passphrase is the deliberate exception — it reads in clear, because it
describes an access point the device is broadcasting to anyone in radio range, and a remote operator
needs it to direct somebody standing at the panel.

TLS is out of scope. A flags bit briefly carried a toggle for it and was retired rather than reused,
because a toggle that does nothing implies protection that is not there.

## Where the specifications live

The wiki is orientation. The contracts are versioned with the code:

| | |
| --- | --- |
| Modbus register map, serial link | `docs/Requirements/Project_document.md` |
| WiFi, MQTT, Home Assistant, the network register block | `docs/Requirements/feature addition/WiFi_MQTT_Connectivity.md` |
| What the panel shows for each of them | `docs/Requirements/feature addition/Display_Per_Screen_Spec.md` |
