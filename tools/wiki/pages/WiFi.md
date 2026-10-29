# WiFi and provisioning

WiFi exists to carry MQTT. It is off until somebody turns it on, and it never turns itself on.

**Where the detail lives.** Specified in `docs/Requirements/feature addition/WiFi_MQTT_Connectivity.md`
§3 and §7, implemented in `src/net/wifi_manager.cpp` (the state machine) and
`src/net/portal_form.cpp` with `src/net/portal_server_arduino.h` (the configuration web page). This page
is the operator's and integrator's view.

## The radio is never switched on for you

Two rules, and everything below follows from them:

- **WiFi is enabled only by a setting the operator changed** — `kWifiEnabled`, register `500`. The
  panel shows the setting and has no editor for it. A device that has never been configured does not power the radio at all, so it costs the
  measurement nothing.
- **AP mode is never entered automatically, and never on a timer.** It is a consequence of enabling
  WiFi on a device that has no credentials.

The pulse counter is I²C-bound and shares its bus with everything else the board does. The radio
contends for CPU and for the RS485 driver's attention, so "always on, just in case" is not a neutral
default here — it is a measurement regression.

## States

Register `501` reports these. **The numbers are the wire contract** — append, never reorder.

| Value | State | Panel | Meaning |
| --- | --- | --- | --- |
| `0` | Disabled | `OFF` | Not enabled, or no credentials. Radio powered down. |
| `1` | Idle | `IDLE` | Enabled, radio down, nothing to do — unconfigured with the portal window closed. |
| `2` | Connecting | `CONN` | Associating and awaiting DHCP. |
| `3` | Connected | `OK` | Associated, with an address. Read it from register `503`. |
| `4` | Retrying | `RETRY` | Failed; waiting out the backoff. |
| `5` | AP portal | `AP` | Offering the provisioning access point. |
| `6` | Failed | `FAIL` | The backoff has saturated — most likely a wrong passphrase or an AP that is gone. |

What happens when WiFi is enabled depends on one thing:

```
enabled + credentials stored   ->  Connecting  ->  Connected
enabled + no credentials       ->  AP portal (10 minutes)  ->  Idle
```

`Idle` has exactly one automatic way out: credentials arriving from another surface — a Modbus write,
say. That is what makes fully remote setup work without anyone standing at the panel.

## Retrying, and why it is jittered

Exponential backoff from **1 s** to a **5 min** ceiling, with a **20 s** connect timeout of the
device's own. The timeout is not redundant: a station facing a captive or misbehaving AP can report
"connecting" indefinitely, and without a deadline the backoff ladder would never start.

The jitter is **subtractive** — the delay is `base - [0, base/8]`, never `base + something`. Additive
jitter on a step already clamped at the ceiling would either push past it or be clamped away exactly
where it is needed most: a whole site's meters, all rebooted by the same power cut, retrying in
lockstep forever.

Failures are distinguishable, because "it doesn't work" is not a diagnosis:

| Error | Means |
| --- | --- |
| `AuthFailed` | Wrong passphrase. |
| `ApNotFound` | The SSID is not in range. |
| `AssocTimeout` | The driver never resolved either way inside 20 s. |
| `LinkLost` | Was associated, then was not. A normal condition, not a fault. |
| `RadioFault` | The driver refused to start or connect — a local fault. |
| `ApStartFailed` | `softAP()` was refused. Also local. |

**A configuration that cannot associate is not rolled back.** The state machine reports the failure and
keeps retrying; it never edits settings. Deciding that a failed connection means the old credentials
were better is a decision for an operator with context, not for a retry loop.

## The provisioning access point

Raised when WiFi is enabled on a device with no stored credentials.

| | |
| --- | --- |
| SSID | `water_flow_meter_<6 digits>` — stable for the life of the board |
| Security | WPA2, a 12-character passphrase |
| Window | **10 minutes**, then the AP shuts down and the device goes Idle |
| Device address | Register `708`; the configuration page answers on it |

The AP passphrase is **readable in clear** — on the panel and over Modbus (register `692`) — while the
WiFi passphrase the operator gave the device never reads back at all. That asymmetry is deliberate: the
AP password describes an access point the device is broadcasting, which anyone in radio range can
already see, and a remote operator needs it to direct somebody standing at the panel. The other is a
secret the device was trusted with.

Every DNS name resolves to the device while the AP is up, and any unknown path redirects to `/`, which
is what makes a phone pop the portal without anyone typing an address. Both stop the moment the AP does
— hijacking name resolution for a device that is merely on a network would be a different thing
entirely.

### Reopening the window

There is **no direct way to reopen it.** `WifiManager::requestApPortal()` exists and is tested, but
nothing in the shipped firmware calls it: there is no portal-enable register (the code comments
reference a `NET_PORTAL_ENABLED` that was never added) and no menu action.

What works: **toggle `kWifiEnabled` (register `500`) off and back on.** That re-runs the enable path,
which raises the AP again if the device still has no credentials. Note the condition — on a device that
already has credentials this reconnects instead, and does not open a portal.

## The configuration page

HTTP on the AP address. Basic auth, defaulting to **`admin` / `admin`**, and the login form is the
first thing on the page for that reason.

Locked out? Write `0x5AA5` to register `710` and the login returns to `admin`/`admin`. That command
acts immediately rather than staging, because a recovery action that needs a second write is one
somebody gets half-way through. It grants a Modbus master nothing new: register `736` already lets it
set the password outright — the command exists because "reset to a known default" is a different
intention from "set to this string" and deserves to be expressible.

**The page is served only while the access point is up**, and stops with it: when the window closes,
when WiFi is turned off, and when a submitted form gives the device credentials to join with. It is
never served on the network the device joins as a station, because the same server answers every DNS
name with the device's own address. The rule is `src/net/portal_lifecycle.h`.

> **Fixed 2026-10-29, not yet seen working on a board (`DF28`).** Until then the firmware built this
> page and never started it, so the access point came up and nothing answered on it. Behind that were
> three older defects that would have stopped a submission saving anything, and they are fixed too: the
> form's body was never read, a successful save never reached the WiFi state machine, and every
> submission reported errors for read-only rows. The rules are host-tested, including a simulated browser
> submitting the page and a check that the firmware calls the rule correctly. What no host test can show
> is a phone getting the page and the device then joining your network, so if that does not happen,
> provision over **Modbus** (below) and report it.

## Configuring it over Modbus

The fully remote path, and the one that needs nobody at the device. Write the credentials, commit, and the device
associates without any AP at all:

```
write kWifiSsid   (510, 16 registers, 2 chars each, high byte first)
write kWifiPsk    (526, 32 registers)          -> staged; reads back as zeros
write kWifiEnabled(500) = 1                    -> staged
write kApply      (730) = 0x5AA5               -> committed together
read  kRevision   (731)                        -> incremented, so the write took
read  kWifiState  (501)                        -> 2 connecting, then 3 connected
read  kLastError  (732)                        -> if the apply was refused
```

Two of those three reads returned 0 no matter what happened, until 2026-08-20. `kWifiState` was
never written at all — the whole live half of the block was zeroed on every republish — and
`kLastError` was written on a refusal and erased within a second. So this sequence, the one route a
device with no portal window has, could not distinguish a successful association from a refused
apply. It works now, and the registers refresh once a second; if you are reading this against
firmware older than that, only `kRevision` means anything.

Staging matters here for the same reason it does on the serial link: these fields have to change
together or not at all. A committed SSID with an uncommitted passphrase is a device that cannot
associate and has forgotten how it used to.

## What the panel shows, and does not

The panel is a **reader** of WiFi and MQTT configuration. It shows the state, SSID, address, signal
strength, the broker's state, and the AP details while a portal is open — and it cannot edit any of
it, because there is no on-device text entry and an SSID is text. Every configurable network field is
set over the portal or over Modbus.

What the panel does configure is its own behaviour and the Modbus link: Configuration → Modbus for the
serial parameters, Configuration → Display for the LED and the flow unit shown on screen.
