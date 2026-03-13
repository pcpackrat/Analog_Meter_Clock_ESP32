# ESP32 Analog Meter Clock - Wiring Guide

This document lists all the hardware connections between the ESP32 and the various components (Analog Meters, RTC, LEDs, and Switches).

## Complete Pinout Table

| ESP32 Pin (GPIO) | Board Label | Connected To | Description / Notes |
| :--- | :--- | :--- | :--- |
| **GPIO 5** | **D5** | **RTC SDA** | I2C Data line for DS3231. *(Note: Strapping pin, temporarily disconnect if firmware fails to flash).* |
| **GPIO 18** | **D18** | **RTC SCL** | I2C Clock line for DS3231. |
| **GPIO 4** | **D4** | **TZ Switch (Pole 1)** | Active Low switch to toggle UTC mode. |
| **GPIO 19** | **D19** | **TZ Switch (Pole 2)** | Configured in software to act as **Ground (GND)** for the TZ switch. |
| **GPIO 25** | **D25** | **Hour Meter** | PWM output for the Hour analog meter. |
| **GPIO 26** | **D26** | **Minute Meter** | PWM output for the Minute analog meter. |
| **GPIO 27** | **D27** | **Second Meter** | PWM output for the Second analog meter. |
| **GPIO 13** | **D13** | **WS2811 Data** | Data line for the WS2811 LED backlighting. |
| **GPIO 0** | **D0** | **External BOOT Button** | Connect to a momentary push button going to **GND**. Hold this down while pressing EN/RESET to enter flashing mode. |
| **EN** | **EN/RST** | **External RESET Button** | Connect to a momentary push button going to **GND**. Press to restart the ESP32. |

## Power Connections

### DS3231 RTC Module
| RTC Pin | Destination |
| :--- | :--- |
| **VCC** | **3.3V** (ESP32) |
| **GND** | **GND** (ESP32) |
| **SDA** | **D5** (ESP32) |
| **SCL** | **D18** (ESP32) |

### WS2811 LEDs
*   **5V / VCC:** Connect to a standard 5V supply (matching the ESP32 input).
*   **GND:** Connect to a common ground with the ESP32.
*   **DATA:** Connect to **D13** (GPIO 13). *(Note: A 3.3V to 5V logic level shifter is recommended for stability).*

### Analog Meters
*   **Positive Terminal (+):** Connect to respective GPIO (D25, D26, D27) via calibration resistors.
*   **Negative Terminal (-):** Connect to common ground (**GND**) and add 0.1µF smoothing capacitors across the terminals if necessary.
