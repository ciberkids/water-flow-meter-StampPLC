# PinMap

## PWR-485 & PWR-CAN

ESP32-S3FN8 | G42 | G43 | G0 | G39 | G46
--- | --- | --- | --- | --- | ---
PWR-CAN | CAN_TX | CAN_RX |  |  | 
PWR-485 |  |  | RS485_TX/BOOT | RS485_RX | RS485_DIR


## RGB & User Button A/B/C

- Controlled via PI4IOE5V6408

ESP32-S3FN8 | G15 | G13 | G14 | G3
--- | --- | --- | --- | ---
PI4IOE5V6408(0x43) | SCL | SDA | INT | RST

PI4IOE5V6408 | P6 | P5 | P4 | P2 | P1 | P0
--- | --- | --- | --- | --- | --- | ---
RGB | R | G | B |  |  | 
Button |  |  |  | KEYA | KEYB | KEYC


## microSD Card Slot

ESP32-S3FN8 | G9 | G10 | G7 | G8
--- | --- | --- | --- | ---
microSD | MISO | CS | SCK | MOSI


## Sensor

ESP32-S3FN8 | G15 | G13 | G14
--- | --- | --- | ---
INA226AIDGSR (0x40) | SCL | SDA | INT
LM75BDP (0x48) | SCL | SDA | INT
RX8130CE (0x32) | SCL | SDA | INT


## LCD

ESP32-S3FN8 | G8 | G7 | G6 | G12 | G3
--- | --- | --- | --- | --- | ---
LCD | MOSI | SCK | RS | CS | RST

PI4IOE5V6408 | P7
--- | ---
LCD | LCD_BL


## Buzzer

ESP32-S3FN8 | G44
--- | ---
Buzzer | BUZZER_PWM


## Relay & Optocoupler

- Controlled via AW9523B (0x59)

ESP32-S3FN8 | G15 | G13 | G14 | G3
--- | --- | --- | --- | ---
AW9523B(0x59) | SCL | SDA | INT | RST

- Relay Control

AW9523B | P0_0 | P0_1 | P0_2 | P0_3
--- | --- | --- | --- | ---
Relay | RLY_DRV1 | RLY_DRV2 | RLY_DRV3 | RLY_DRV4

- Optocoupler Output

AW9523B | P0_4 | P0_5 | P0_6 | P0_7 | P1_4 | P1_5 | P1_6 | P1_7
--- | --- | --- | --- | --- | --- | --- | --- | ---
EL3H4 | SYS_IN1 | SYS_IN2 | SYS_IN3 | SYS_IN4 | SYS_IN5 | SYS_IN6 | SYS_IN7 | SYS_IN8

- Optocoupler Input

EL3H4 | Function Description
--- | ---
EXCOM_IN1 | External Input Signal 1
EXCOM_IN2 | External Input Signal 2
EXCOM_IN3 | External Input Signal 3
EXCOM_IN4 | External Input Signal 4
EXCOM_IN5 | External Input Signal 5
EXCOM_IN6 | External Input Signal 6
EXCOM_IN7 | External Input Signal 7
EXCOM_IN8 | External Input Signal 8
EXCOM_COM | Common Terminal


## GPIO.Ext

ESP32-S3FN8 | GPIO.Ext
--- | ---
G40 | Custom
G41 | Custom
G8 | MOSI
G11 | CS
G9 | MISO
G7 | SCK
G3 | PHY RST
G14 | INT
G15 | SCL
G13 | SDA


## HY2.0-4P

- PORT.A

HY2.0-4P | Black | Red | Yellow | White
--- | --- | --- | --- | ---
PORT.A | GND | 5V | G2 | G1

- PORT.C

HY2.0-4P | Black | Red | Yellow | White
--- | --- | --- | --- | ---
PORT.C | GND | 5V | G5 | G4
