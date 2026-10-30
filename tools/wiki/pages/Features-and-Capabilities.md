# Features and Capabilities

Everything the project offers, on one page: what the device is for, how its meters can be configured,
how it communicates, and what the web app does. Each feature carries its status and the page that holds
the detail. This page is a map, so register addresses, topics and limits live on the pages it links to,
which are generated from the firmware or checked against it.

> ⚠️ **Nothing has run on hardware yet.** The firmware compiles and passes its host test suite, but every
> feature below has been checked only against the specifications and those tests, never on a board. The
> first bring-up is open item `G1` in `docs/active_work/open_decisions.md`, which is also where every ID on
> this page is explained.

**Status words used here:** *built* works in the firmware or the tools; *partly built* has a missing
piece, which is named; *planned* is specified and not built.

## 1. What the project is for

A water flow monitor for up to **eight pulse-output flow meters**, built on the M5Stack StampPLC, an
ESP32-S3 controller. It counts each meter's pulses, turns them into flow and volume, keeps the totals
across power cuts, and reports everything in three places:

- to a supervisory system, such as a programmable logic controller (PLC) or a SCADA (supervisory control
  and data acquisition) system, over **Modbus RTU** (Remote Terminal Unit, the serial form of Modbus) on
  the RS485 bus;
- to **Home Assistant**, or any other MQTT client, over WiFi;
- on its own **screen**, driven by three buttons, and on a status LED.

### The hardware

| | |
| --- | --- |
| Controller | M5Stack StampPLC (ESP32-S3) |
| Meter inputs | 8 opto-isolated digital inputs, IN1 to IN8, one meter on each |
| Meters | Pulse-output Hall-effect flow sensors, such as the YF-B10 (476 pulses per litre). Red wire to 5 V, black to ground, yellow to the input |
| Power | 6 to 36 V DC |
| Screen | 1.14 inch, 240 × 135 pixels, landscape, with three buttons: UP, DOWN and ENTER |
| Status light | One red, green and blue LED |
| Bus | RS485, for Modbus RTU |
| Radio | WiFi, off until someone enables it |
| Storage | Internal flash for settings and totals; a microSD slot for menu packs |

### What it measures

For each channel:

| Reading | Unit | After a power cut |
| --- | --- | --- |
| Instant flow | litres per minute | live, recomputed |
| Session volume | litres and cubic metres | starts again from zero |
| Lifetime volume | litres and cubic metres | **kept**, saved to flash about once a minute |
| Peak flow since the last reset | litres per minute | cleared, by design |
| Status | `--` not in service, `SET?` not calibrated, `OK` | — |

For the whole device: the total flow and session volume across every channel, the rate the pulse sampler
actually achieves, a per-channel flag for a meter pulsing faster than the sampler can count, a storage
fault code that says when the device has stopped keeping its own records, the uptime and the clock.

### Four rules that shape everything else

- **Pulse counting comes first.** The sampler has a processor core to itself, and nothing (the radio,
  the bus or the screen) may delay it. That is why WiFi is off until enabled.
- **Modbus is the source of truth.** Every reading and every setting can be read and written over RS485,
  and what a Modbus master writes survives a reboot.
- **`--` means not in service, never "not detected".** A pulse meter with no water flowing looks
  exactly like one whose wire has fallen off, so the device cannot sense whether a meter is there. A
  channel is in service because somebody said so.
- **A missing number is better than a wrong one.** The device refuses a calibration that would read
  flow on a dry pipe, and shows `SET?` rather than guessing.

### What the device does on its own

| Feature | Status | Detail |
| --- | --- | --- |
| Menus on the panel: a page per reading for all eight channels, configuration, diagnostics | built | [[Screen Navigation]], [[Screen Gallery]] |
| Status LED: red blinks once per 1, 10 or 100 litres, green when every channel in service is calibrated, blue while water is flowing, a colour cycle during boot, and a white blink that speeds up during a reset countdown | built | `docs/Requirements/feature addition/RGB_LED_Behavior.md` |
| Destructive actions need ENTER held on a confirm screen, from 1.5 s for a session reset to 30 s for a factory reset | built | [[Screen Navigation]] |
| Swap the whole menu from the SD card, chosen in the Select Menu | built: `npm run export:pack` builds a pack, and the device reads and selects it | [[SD Card]] |
| Report a failing flash store, over Modbus and MQTT | partly built: the warning on the panel is not drawn yet (`DF25`) | [[Modbus Registers]] |
| A real-time clock, kept in UTC (Coordinated Universal Time, with no time zone), set over Modbus or by network time | built; whether the clock chip keeps time through a power cut is unmeasured (`N-d2`) | [[Modbus Registers]] |

## 1a. Configuring the meters

Each of the eight channels is configured on its own, so any mix of meters can share one device.

### Putting a channel in service

A channel counts only while it is **in service**, an on/off setting per channel. **Switching a channel
on or off clears its calibration and its totals**, so it starts from nothing either way. A channel out of
service reads `--` everywhere.

### Two ways to calibrate

Meter datasheets state the pulse rate in one of two forms, and each channel takes either. Pick the form
with the channel's *Calibration* setting: `Pulses/L` or `Formula`.

| Form | What the datasheet says | What you enter |
| --- | --- | --- |
| **Pulses per litre** | One figure, for example `476 pulses/L` | Pulses per litre, 1 to 65,535, stored exactly |
| **Formula** | A straight line, `F = m × Q + c`: frequency F in hertz against flow Q in litres per minute | Multiplier `m`, a whole number from 1 to 32,767, and offset `c`, from 0 up to `m` × Q max |

Both forms also need **Q max**, the meter's nominal maximum flow in litres per minute. It must not be
zero: readings are capped at it, and the sampling check below uses it.

Pulses per litre exists because the formula's multiplier is a whole number. A meter rated 450 pulses
per litre needs a multiplier of 7.5, and rounding it to 7 or 8 puts a 6 % error on every reading.

**A negative offset is refused.** It would make a dry pipe read flow and add volume to the totals. If a
datasheet's formula has a negative intercept, such as `F = 6 × Q − 8`, enter the meter as pulses per
litre, or enter the offset as 0 and accept the small error at low flow.

Until a channel's calibration is complete and valid it reads `SET?` and accumulates nothing.

### The sampling check

The sampler reads the inputs at a fixed rate, designed as 3.3 kHz and not yet measured on a board
(`G1`). A calibration whose pulse rate at Q max is more than half the sampling rate would be under-counted,
so the device refuses it and raises that channel's under-sampling flag. A Modbus master that knows better
can insist by writing the same values twice. The same flag lights while running if a meter pulses faster
than the sampler can count.

### Resets

| Reset | Applies to | What it clears | Keeps |
| --- | --- | --- | --- |
| Peak flow | every channel | the peak | everything else |
| Session | one channel or all | session volume and peak | lifetime totals, calibration |
| Totals | one channel or all | session and lifetime volume | calibration |
| Calibration | one channel | its calibration, back to defaults | its measurements. Meant for swapping a broken meter for a different model |
| Configuration | one channel or all | calibration **and** measurements | nothing. Meant for taking a channel out of use |
| Factory reset | the device | every channel's calibration and totals, and the device settings, back to defaults | nothing |

Every reset except the factory reset is available over Modbus. The panel offers the peak, session,
totals and calibration resets behind confirm screens, and the factory reset is **only** on the panel,
behind a 30 s hold. Home Assistant offers the session and totals resets as buttons. Addresses are on
[[Modbus Registers]].

### Device-wide settings

| Setting | Choices | Default |
| --- | --- | --- |
| Flow unit on the screen | L/min, L/s, m³/h. **The screen only**: every register and every MQTT message stays in litres per minute | L/min |
| Red LED volume step | 1, 10 or 100 litres per blink | 1 |
| Red LED blink period | 100 to 2,000 ms | 500 ms |
| Modbus address | 1 to 247 | 1 |
| Modbus baud rate | 1,200 to 115,200 | 9,600 |
| Parity and stop bits | none, even or odd; 1 or 2 | none, 1 |

Calibration and every setting above can be changed at the panel or over Modbus. The WiFi and MQTT
settings are shown on the panel but cannot be changed there (most of them are text, and the panel has no
way to type text), so set them over Modbus or on the configuration web page. [[Communications]] has the full picture of what each route can do.

### Meters in cascade: in progress, not usable yet

Meters can be installed in **cascade**: a main meter with others downstream of it, for example a main
meter on the supply and sub-meters on the branches it feeds. The device is designed to handle this
(open item `N-e`), and about a third of it is built. **Today no screen or register can set a cascade**,
so every channel is treated as a separate pipe.

**What it will do.** Each channel gets one setting, its *parent*: the meter directly upstream of it, or
none. From that:

- **The total stops counting water twice.** The device total becomes the sum of the top-level meters
  only. Without this, a main meter plus two sub-meters reads roughly twice the water delivered.
- **The difference becomes evidence.** A parent should read the sum of its children. Water that entered
  a branch and that no child measured is an unmetered outlet or a leak. The device compares the
  difference over time against a baseline taken when the installation is commissioned, and warns when it
  moves away from it. A fixed percentage would not work: one unmetered garden tap can exceed 10 % of the
  water used, so a fixed limit would be either always on or blind to a small overnight leak.
- **A missing number rather than a wrong one.** If a top-level meter is not calibrated, its branch's
  total is reported as unknown rather than as a smaller figure.
- **Taking a meter out of service keeps the shape.** Its children attach to the nearest in-service
  meter above them, never to the top.
- **Any shape, no loops.** Up to all eight meters in one chain. A setting that would create a loop, or
  name a meter as its own parent, is refused and changes nothing.

| Piece | Status |
| --- | --- |
| The rules: parents, loop detection, and keeping the arrangement in flash. An arrangement found damaged at boot falls back to every meter independent | built (T1) |
| The total that counts each litre once, computed beside today's plain sum | built, internal only (T2). Nothing publishes it yet |
| The comparison against the commissioning baseline, and the warning | not built (T3) |
| Setting the parents over Modbus, and reading the results | not built (T4). This is the first piece that makes the feature usable |
| The parent setting and the warning on the panel | not built (T5) |
| The results in the MQTT diagnostics, read-only | not built (T6) |

Four questions in the specification are still open. MQTT will never be able to set a parent, by
decision, and the configuration web page is left out of the first version.

**Until it lands:** every per-channel reading is correct, including each channel's lifetime volume in
Home Assistant. Only the device's total is wrong on a cascaded installation, because it counts the
downstream water twice; add up the top-level channels yourself. The full design is in
`docs/Requirements/feature addition/Sensor_Cascade_Topology.md`.

### Not configurable yet

- **Settings from a file on the SD card** (`N8b`, planned). See [[SD Card]].

## 2. Communication

| Route | What it gives you | Status | Detail |
| --- | --- | --- | --- |
| **Modbus RTU over RS485** | Every reading and every setting, including the network settings. Configuration changes are staged, committed together, and confirmed. Needs no network | built | [[Modbus Registers]], [[Communications]] |
| **MQTT** | Telemetry for each channel, the total and the device's health; Home Assistant discovery, which announces every entity so none needs YAML, and hands every config to the MQTT client with any number of channels (fixed on 2026-10-30, `DF30`); three Home Assistant buttons: reset session, reset totals, republish discovery | built; one check waits for hardware (`N-c`), and what the MQTT client does after the hand-over is not yet measured (`DF31`) | [[MQTT]] |
| **WiFi** | Carries MQTT and network time. Off until enabled, joins your network with automatic retries, and offers a provisioning access point when it has no credentials | built | [[WiFi]] |
| **Network time** (NTP, the Network Time Protocol) | Sets the clock once WiFi is connected, and again every six hours | built | — |
| **Configuration web page** | A login-protected form for the WiFi and MQTT settings and the clock, served on the provisioning access point while it is up | built; fixed on 2026-10-29 after it had never been started (`DF28`) | [[WiFi]] |

**Two ways to put a device on a network:** join its provisioning access point with a phone and fill in
the configuration page, or write the settings over Modbus, which needs nobody at the device. The WiFi
page gives both, including the exact Modbus write sequence.

What holds across every route:

- **One unit per quantity.** Flow is litres per minute and volume is litres and cubic metres, on every
  register and in every message. The screen's flow unit never changes what goes over a wire.
- **No fixed-point scaling.** Readings are real numbers, and values spanning several registers put the
  high word first.
- **Staged configuration.** Over Modbus a change is written, then committed by a magic value, then
  confirmed by a revision counter, so the bus settings can be changed safely over the bus itself.
- **One identity.** The MQTT client id, the topics, the Home Assistant device and the access point name
  are all derived from the WiFi hardware address, so they survive a reboot and never collide with the
  next device.
- **Secrets are write-only.** The WiFi passphrase and the MQTT password never read back.
- **MQTT never changes configuration.** It can reset measurements and nothing else, deliberately: a
  reset can be recovered by re-reading the meter, a changed calibration or broker cannot.
- **No encryption.** TLS (Transport Layer Security) is out of scope; the device is meant for a trusted
  network.

## 3. The web app

The web app is the **screen designer** in `web/mockup/`. It is a tool for designing the device's menus,
and it never connects to a meter: it is neither a monitoring dashboard nor the device's own configuration
page from section 2.

The panel's screens are not written by hand in the firmware. They are designed in this app, saved as
data, and translated into C++ tables that the firmware draws.

| Feature | Status |
| --- | --- |
| Every screen shown at the panel's exact size, 240 × 135 pixels | built |
| The three buttons simulated on the keyboard (ArrowUp, ArrowDown, Enter), with the device's short-press, long-press and hold timings, so navigation, editors and countdowns can be walked through | built |
| **Simulation** tab: the screen, the simulated device values, a trace of what each button press did, and layout diagnostics | built |
| **Design** tab: element toolbox, colour and font theme, the screen tree, button bindings, and a live JSON editor | built |
| **Import & Export** tab: load, check and download a design, and export it to the firmware | built |
| **Help & Documentation** tab: the design file's schema and a summary of the loaded design | built |
| Export to the firmware: translates the design into C++, backs up the previous files, and compiles the firmware to prove the result builds | built |
| Ten checks before an export, which refuse a screen, action or value the firmware does not have, an element the firmware cannot draw, or code that does not compile | built |
| The default menu generated from the firmware's own list of settings, refusing any menu that leaves a setting unreachable | built |
| A picture of every screen for the [[Screen Gallery]], captured by continuous integration (CI) on every run | built |
| A menu pack (`.uipack`) for the SD card, built by `npm run export:pack` through the same checks as an export, plus the device's size and file name limits read from the firmware | partly built: command line only, with no download button in the Import & Export tab, and a design of your own is not yet checked for an editor per setting. See [[SD Card]] |

**Running it:** `npm ci` and then `npm run dev` in `web/mockup`, then open `http://localhost:5173/`.
The repository's `docker-compose.yml` runs the same thing in a container: `docker compose up` from the
repository root, on the same port.

The rules a design must follow are on [[UI Dataset Contract]], and the tool's own guide is
`web/mockup/README.md`.

*This page is published from `tools/wiki/pages/Features-and-Capabilities.md`. Edit that file, not this
page.*
