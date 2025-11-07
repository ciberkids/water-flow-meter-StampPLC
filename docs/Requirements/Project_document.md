Of course. Here is a comprehensive project document in Markdown format that outlines the requirements, scope, architecture, and technical specifications for the M5Stack StampPLC Flow Monitoring System.

---

# Project Documentation: M5Stack StampPLC Multi-Channel Flow Monitoring System

**Version:** 1.0  
**Date:** October 24, 2025

## Executive Summary

This document outlines the design and functional requirements for a multi-channel flow monitoring system built on the M5Stack StampPLC microcontroller. The system is designed to read pulse data from up to eight Hall-effect flow meters, process this data into meaningful metrics (flow rate, volume), and expose all information and configuration options to a supervisory system (like a SCADA or PLC) via the Modbus RTU protocol over an RS485 interface.

The firmware leverages the dual-core architecture of the onboard ESP32-S3 to ensure high-reliability pulse counting, even while handling communication tasks. The system also features persistent storage for cumulative flow data, ensuring that totals are maintained across device reboots or power cycles.

---

## 1. System Overview

### 1.1. Core Objective

To create a robust and reliable firmware for the M5Stack StampPLC that can simultaneously monitor, calculate, and report the flow and volume of liquid passing through up to eight individual flow sensors.

### 1.2. Key Features

*   **Multi-Sensor Support:** Natively supports up to 8 digital input channels for flow monitoring.
*   **High-Reliability Polling:** Utilizes a dedicated microcontroller core for high-frequency polling to ensure no pulses are missed, even at maximum flow rates.
*   **Modbus RTU Slave:** Implements a standard Modbus RTU slave over RS485 for industrial communication and integration.
*   **Remote Configuration:** Allows a Modbus master to enable/disable sensors and configure the specific characteristics of each connected flow meter.
*   **Comprehensive Data Reporting:** Provides real-time data (instantaneous flow), resettable session data (volume since last reset), and persistent cumulative data (total lifetime volume).
*   **Persistent Memory:** Cumulative volume data is saved to non-volatile memory to survive power loss.
*   **System Diagnostics:** Reports its own core task performance (polling rate).
*   **Status Feedback:** A tri-colour LED communicates production pulses, configuration readiness, and live flow activity.

### 1.3. Target Hardware

*   **Controller:** M5Stack StampPLC (ESP32-S3 based)
*   **Sensors:** Pulse-based Hall-effect water flow sensors (e.g., YF-B10 series).

---

## 2. Hardware & Wiring

### 2.1. Controller

The M5Stack StampPLC provides all necessary hardware features:
*   8 opto-isolated digital inputs (IN1-IN8) for connecting the sensor signal lines.
*   A built-in RS485 interface for Modbus communication.
*   A wide-range DC power input (6-36V).
*   5V power output pins to power the sensors.
*   An RGB status LED (red/green/blue channels via PI4IOE5V6408 expander) for operational feedback.

### 2.2. Sensor Wiring

Each flow meter should be connected to the StampPLC as follows:

| Sensor Wire | Connection Point on StampPLC                                 | Purpose          |
| :---------- | :----------------------------------------------------------- | :--------------- |
| **Red**     | `5V` Terminal (e.g., from GPIO.EXT or Grove port)          | 5V Power Supply  |
| **Black**   | `GND` Terminal                                               | Ground           |
| **Yellow**  | `IN1` through `IN8` Digital Input Terminals                  | Pulse Signal Out |

---

## 3. Software Architecture

### 3.1. Development Environment

*   **Platform:** Arduino IDE or PlatformIO
*   **Core Libraries:**
    *   `M5StamPLC.h`: For hardware abstraction of the StampPLC.
    *   `eModbus.h`: For implementing the Modbus RTU slave protocol.
    *   `Preferences.h`: For accessing the ESP32's non-volatile storage (NVS).

### 3.2. Dual-Core Task Allocation

To guarantee system reliability, the firmware operates on a real-time operating system (FreeRTOS) and assigns specific tasks to each of the ESP32-S3's two cores.

#### 3.2.1. Core 0: Real-Time Polling Task

*   **Priority:** High
*   **Responsibility:** This core is exclusively dedicated to a single, high-speed task:
    1.  Continuously poll the state of the 8 digital input pins via the I/O expander.
    2.  Detect rising edges (LOW to HIGH transitions) on each active channel.
    3.  Atomically increment the pulse count for the corresponding sensor.
*   **Outcome:** This separation ensures that pulse counting is never delayed or interrupted by other application logic, such as Modbus communication delays.

#### 3.2.2. Core 1: Application & Communication Logic Task

*   **Priority:** Normal
*   **Responsibilities:** This core handles all other system functions:
    1.  **Modbus Slave Task:** Manages the RS485 communication, listening for requests from a master and responding with data. Handles all read and write operations on the register map.
    2.  **Data Processing:** Periodically (e.g., once per second), it reads the pulse counts generated by Core 0.
    3.  **Calculations:** Converts pulse counts into frequency (Hz), then calculates instantaneous flow (L/s), and integrates this over time to update session and cumulative volumes.
    4.  **Data Persistence:** Periodically saves the cumulative volume data for each active sensor to non-volatile storage.
    5.  **Register Updates:** Keeps the Modbus register map updated with the latest calculated values.
    6.  **Register Map Management:** Maintains an in-memory holding register table served through eModbus worker callbacks, encoding floats and doubles as IEEE-754 big-endian word pairs/quads for Modbus transport.

---

## 4. Modbus RTU Register Map

The system functions as a Modbus slave with a defined map of Holding Registers.

*   **Slave ID:** Configurable (Default: 1)
*   **Baud Rate:** Configurable (Default: 9600, 8N1)

### 4.1. Global Registers

| Address | Register Name                        | Type        | R/W | Description                                                                                               |
| :------ | :----------------------------------- | :---------- | :-- | :-------------------------------------------------------------------------------------------------------- |
| 0-1     | Polling Rate (kHz)                 | `float`     | R   | The measured execution speed of the pulse polling loop on Core 0.                                           |
| 10      | Connected Sensors Bitmap           | `uint16_t`  | R/W | A bitmask to enable/disable sensors. E.g., writing `3` (00000011b) enables sensors 1 and 2. Resets a sensor on activation and mirrors the live enable state back to masters. |
| 20      | Master Reset All Configs           | `uint16_t`  | W   | Write `1` to reset everything (cumulative, session, configs) for ALL active sensors.                    |
| 21      | Master Reset All Measured Values | `uint16_t`  | W   | Write `1` to reset cumulative and session values for ALL active sensors.                                |
| 22      | Master Reset All Session Values    | `uint16_t`  | W   | Write `1` to reset only the session (non-persistent) values for ALL active sensors.                     |
| 30      | Undersampling Flags                | `uint16_t`  | R   | Bitfield where bit *n* = 1 indicates sensor *n* exceeded the Nyquist limit during its last validation cycle. Resets automatically when the sensor passes validation. |
| 31      | LED Red Volume Step                | `uint16_t`  | R/W | Defines the totalized volume (liters) per red LED pulse. Supported values: 1, 10, 100. Defaults to 1. |
| 32      | LED Red Pulse Period (ms)          | `uint16_t`  | R/W | Sets the full on/off period for each red pulse (clamped 100–2000 ms, default 500 ms). |

### 4.2. Per-Sensor Register Block

The following block of 23 registers is repeated for each of the 8 sensors.
*   **Sensor 1 Base Address:** 100
*   **Sensor 2 Base Address:** 140
*   **Sensor `n` Base Address:** `100 + (n-1) * 40`

| Offset | Address (Sensor 1) | Register Name                 | Type       | R/W | Description                                                                                                                              |
| :----- | :----------------- | :---------------------------- | :--------- | :-- | :--------------------------------------------------------------------------------------------------------------------------------------- |
| **0**  | 100                | Status Flags                  | `uint16_t` | R   | Bit 0: `inUse` (is the sensor enabled via bitmap?). Bit 1: `isReady` (is it configured and providing valid data?).                         |
| **1-2**| 101                | Instant Flow (L/s)            | `float`    | R   | Instantaneous flow rate in Liters per Second.                                                                                            |
| **3-6**| 103                | Cumulative Liters             | `double`   | R   | Total volume measured since the last full reset. This value is **persistent** across reboots.                                          |
| **7-10**| 107                | Cumulative Cubic Meters       | `double`   | R   | Derived from Cumulative Liters (`Liters / 1000`).                                                                                        |
| **11-12**| 111                | Session Liters                | `float`    | R   | Total volume measured since the last session reset. This value is **not persistent**.                                                  |
| **13-14**| 113                | Session Cubic Meters          | `float`    | R   | Derived from Session Liters.                                                                                                             |
| **15-16**| 115                | Max Flow Since Reset (L/s)    | `float`    | R   | The peak instantaneous flow rate recorded during the current session.                                                                    |
| **17** | 117                | **CMD: Reset Session**          | `uint16_t` | W   | Write `1` to reset Session Liters, Session Cubic Meters, and Max Flow to 0.                                                              |
| **18** | 118                | **CMD: Reset All Measured**     | `uint16_t` | W   | Write `1` to reset all session values AND the persistent Cumulative Liters/Meters to 0.                                                  |
| **19** | 119                | **CMD: Reset Config**           | `uint16_t` | W   | Write `1` to reset all measured values AND the sensor's characteristics below. Sets the `isReady` flag to `false`.                   |
| **20** | 120                | **CONFIG: Q (Max Flow)**        | `uint16_t` | R/W | The nominal max flow of the sensor in L/min (e.g., 50). Used for clamping values.                                                         |
| **21** | 121                | **CONFIG: F (Multiplier)**      | `int16_t`  | R/W | The `F` value in the formula `Frequency = F * Q(L/min) + Adjust`.                                                                        |
| **22** | 122                | **CONFIG: Adjust**              | `int16_t`  | R/W | The `Adjust` value in the formula.                                                                                                       |

> **Encoding Note:** Floats and doubles are transported as IEEE-754 big-endian values, with the most significant 16-bit word placed at the lowest register address within each multi-register field.

---

## 5. Functional Requirements

### 5.1. Sensor Configuration Workflow

1.  A Modbus master writes to the **Connected Sensors Bitmap** (reg 10) to enable the desired sensor channels. When a sensor is first enabled, all its values and configuration are cleared, and its `isReady` flag is set to `false`.
2.  The master then writes the specific characteristics of the flow meter to the **CONFIG** registers for that sensor block (Q, F, Adjust).
3.  Once the configuration is valid (non-zero `Q`, non-zero `F`, and `|Adjust|` bounded to `Q * |F| * 10`), the firmware automatically sets the sensor's `isReady` flag (in the Status Flags register) to `true`.
4.  Only when `isReady` is `true` will the system perform flow and volume calculations for that sensor. Otherwise, the register map reports 0 for all measurement fields while underlying counters remain frozen.
5.  If the proposed configuration would violate the sampling limit (`pollingRate_kHz * 1000 < 2 * (Q * F + Adjust)`), the firmware rejects the change, raises bit *n* in **Undersampling Flags** (reg 30), and keeps the previous values unless the master explicitly overrides the warning.

> Attempting to write configuration registers for a sensor that is not enabled in the bitmap is rejected with an illegal data address response.
>
> When bit *n* in **Undersampling Flags** is set, the associated sensor continues reporting measurements but the master is advised to adjust configuration or polling rate; the bit clears automatically once the validation passes.

### 5.2. Flow Calculation Formula

The firmware uses the following configurable formula to determine flow rate from the measured pulse frequency:

> `Flow Rate (L/min) = (Frequency_Hz - Adjust) / F_Multiplier`

This allows the system to be adapted to a wide variety of flow sensors with different pulse characteristics.

### 5.3. RGB Status LED Behaviour

1.  Red channel pulses whenever the aggregated session volume crosses the threshold defined in **LED Red Volume Step** (reg 31). The pulse period is taken from **LED Red Pulse Period** (reg 32) and applied with a 40 % duty cycle.
2.  Green channel remains ON when every enabled sensor reports `isReady == true`; it turns OFF immediately if any sensor becomes invalid or is disabled.
3.  Blue channel blinks at 2 Hz while the sum of instantaneous flows across ready sensors is greater than 0 L/s; it turns OFF after 500 ms of inactivity.
4.  Factory reset (UP+DOWN 30 s) restores registers 31/32 to defaults and turns all LED channels off during the countdown.

---

## 6. Project Scope

### 6.1. In Scope

*   Complete firmware source code for the M5Stack StampPLC.
*   Implementation of all features described in this document.
*   This documentation, including the detailed Modbus register map.

### 6.2. Out of Scope

*   Development of a Modbus master client or HMI.
*   Physical enclosure design or hardware assembly.
*   Support for protocols other than Modbus RTU.
*   User interface on the StampPLC's built-in screen.
