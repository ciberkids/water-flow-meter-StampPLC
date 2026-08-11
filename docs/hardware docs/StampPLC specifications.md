# StampPLC Display & Interaction Specifications

**Source:** [M5Stack StamPLC documentation](https://docs.m5stack.com/en/core/StamPLC#specifications)
**Date retrieved:** 2025-02-15 · **Re-verified:** 2026-07-30

> §1.1 was added on re-verification, cross-checked against the vendor page,
> `StampPLC pin map.md`, and `M5StamPLC/src/pin_config.h` in the installed library
> (v1.2.0). All three agree. It exists because the firmware had the RS485 pins wrong
> and nothing in this folder recorded them explicitly.

---

## 1. Hardware Overview

| Specification | Parameter |
| --- | --- |
| Module Model | Stamp-S3A Control Module |
| SoC | ESP32-S3FN8@Xtensa LX7 dual-core, main frequency up to 240MHz |
| Flash | 8MB |
| Wi-Fi | 2.4 GHz Wi-Fi |
| Digital Input | 8-channel optocoupler isolated digital input, input voltage range: DC 5 ~ 36V |
| Digital Output | 4-channel relay output |
| Relay | AC 5A@250V / DC 5A@28V |
| DC Power Supply | Supports DC 6 ~ 36V @ 1A wide-voltage supply; DC power interface: DC5521 female 5.5 × 2.1mm (inner positive outer negative) |
| Expansion Port | GPIO.EXT port, 2 × HY2.0-4P ports |
| Communication Port | Onboard PWR-CAN and PWR-485 interfaces |
| PWR-CAN Port | XT30 (2+2) PW-M |
| PWR-485 Port | HT3.96-4P |
| Display | 1.14-inch color display (135×240 resolution), driver chip ST7789v2 |
| Interaction | 1 RESET/BOOT button, 3 user buttons, RGB status LED, buzzer |
| Data Storage | Built-in microSD slot |
| Sensor | LM75 temperature sensor, INA226 voltage/current sensor, RTC (RX8130CE) |
| IO Capacity | 2×8 expansion port max output: DC 4.76V@700mA; HY2.0-4P port load capacity test: DC 4.81V@700mA |
| Power Consumption | Standby current: (5V supply) 5V@21.60mA, (12V supply) 12V@15.22mA; Working current: (5V supply) 5V@93.89mA, (12V supply) 12V@47.84mA |
| Installation | DIN rail installation |
| Operating Temp | 0 ~ 40°C |
| Product Size | 72.0 × 80.0 × 33.4mm |
| Product Weight | 140.0g |
| Package Size | 102.0 × 94.0 × 37.0mm |
| Gross Weight | 163.7g |

### 1.1. Bus and Pin Assignments

Verified against the vendor page, `StampPLC pin map.md` and `M5StamPLC/src/pin_config.h`.

**I²C — internal bus, SCL = GPIO15, SDA = GPIO13, INT = GPIO14, RST = GPIO3**

| Address | Device | Role |
| --- | --- | --- |
| `0x43` | PI4IOE5V6408 | RGB LED (`P6/P5/P4`), user buttons `KEYA/B/C` |
| `0x59` | AW9523B | Digital inputs and relay outputs |
| `0x40` | INA226 | Bus voltage / current |
| `0x48` | LM75B | Temperature |
| `0x32` | RX8130CE | RTC |

> The **8 digital inputs are behind the AW9523B**, not the PI4IOE5V6408. `readPlcInput(ch)`
> performs one I²C read per channel, which is the polling-rate constraint recorded as
> decision **G1**.

**RS485 (PWR-485)**

| Signal | GPIO | Note |
| --- | --- | --- |
| `RS485_TX` | **G0** | Shared with the BOOT pin; this is the vendor's own wiring. |
| `RS485_RX` | **G39** | |
| `RS485_DIR` | **G46** | Direction / driver-enable. |

The library drives these on `UART_NUM_1` in `UART_MODE_RS485_HALF_DUPLEX`. Our firmware
uses Serial2 with the same pins — the GPIO matrix allows any UART on any pin, and staying
off UART1 keeps us clear of the library's optional built-in Modbus slave, which calls
`Serial1.end()`.

**SPI — the SD card and the LCD share one bus**

| Signal | GPIO | Used by |
| --- | --- | --- |
| MOSI | 8 | LCD **and** SD |
| MISO | 9 | LCD **and** SD |
| SCLK | 7 | LCD **and** SD |
| LCD CS | 12 | display |
| SD CS | 10 | card (`SD.begin(10, SPI, 4000000)`) |

> ⚠️ **Card reads contend with the display.** Any feature that touches the SD card must not
> do so during rendering — see `Requirements/feature addition/Loadable_UI_Menu_Packs.md` §4.5.

**Other**

| Signal | GPIO |
| --- | --- |
| CAN_TX / CAN_RX | G42 / G43 |
| Buzzer | G44 |
| Grove (red) SDA | 2 |
| Grove (blue) SDA | 5 |

**`M5StamPLC::Config_t` defaults** — all three optional subsystems are **off** unless
`M5StamPLC.config()` is called before `begin()`:

| Field | Default | Consequence |
| --- | --- | --- |
| `enableModbusSlave` | `false` | Keep it false: it serves the library's own register map, not ours. |
| `enableCan` | `false` | Unused by this project. |
| `enableSdCard` | `false` | **Must be enabled** for loadable menu packs. |

---

## 2. Button Interface

| Control | Electrical Mapping | Notes |
| --- | --- | --- |
| User Buttons A/B/C | PI4IOE5V6408 expander (I²C: SCL = GPIO15, SDA = GPIO13, INT = GPIO14, RST = GPIO3) | Key lines: `KEYA`, `KEYB`, `KEYC`. |
| RGB LED | PI4IOE5V6408 `P6/P5/P4` | Shared expander. |
| Additional combo | Up + Down long-press (30 s) | Reserved for factory reset workflow. |

`RESET/BOOT` remains on the Stamp-S3 module (GPIO0 / GPIO46) and should be exposed in debug menus only.

---

## 3. RGB LED Behaviour

- **Red (P6):** Pulses when cumulative volume crosses the configured threshold (1/10/100 L). Pulse period defaults to 500 ms and is adjustable via global holding register 32.
- **Green (P5):** Solid ON when all enabled sensors report `isReady == true`; OFF otherwise.
- **Blue (P4):** Blinks at 2 Hz whenever aggregate instantaneous flow > 0 L/min.
- Configuration registers:  
  - `Reg 31` — Red volume step (1, 10, 100).  
  - `Reg 32` — Red pulse period (ms), clamped 100–2000 ms.
- Behaviour is mirrored in the on-device UI; see `Requirements/feature addition/RGB_LED_Behavior.md` for details.

---

## 4. Timing Recommendations

- **Display idle timeout:** 120 s with backlight off (restore within 50 ms on user input).
- **Countdown overlays:** centred text box occupying 100 × 80 px, translucent background (`#00000088`).
- **Long-press detection:** 500 ms threshold; treat >1.5 s as “long hold” for ENTER/combos; factory reset requires continuous hold for 30 s.

---

## 5. Asset Pipeline

- Place propeller SVG frames under `graphics/svg/ui/`.
- Use stroke widths ≥1 px for visibility on RGB565 panels.
- Ensure the SVG viewBox is `0 0 48 48` with centred graphics.
- Export any additional icons (e.g., warning triangles) following the same 48 px bounding box for consistency.

---

## 6. References

- [StamPLC pin map](StampPLC%20pin%20map.md)
- [Display UI requirements](Requirements/feature%20addition/Display_UI_Requirements.md)
- [RGB LED behaviour](Requirements/feature%20addition/RGB_LED_Behavior.md)
