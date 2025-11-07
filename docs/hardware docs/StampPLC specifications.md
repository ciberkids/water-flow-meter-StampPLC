# StampPLC Display & Interaction Specifications

**Source:** [M5Stack StamPLC documentation](https://docs.m5stack.com/en/core/StamPLC#specifications)  
**Date retrieved:** 2025-02-15

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
- **Blue (P4):** Blinks at 2 Hz whenever aggregate instantaneous flow > 0 L/s.
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
