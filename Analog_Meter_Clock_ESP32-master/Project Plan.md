# Project Plan: ESP32 Analog Meter Clock

## 1. Project Overview
A three-meter analog clock (Hour, Minute, Second) powered by an ESP32. It features "Triple-Redundant" timekeeping (NTP, WWVB, and RTC backup), a captive portal configuration interface, and customizable WS2811 LED backlighting with automated Day/Night profiles.

---

## 2. Hardware Requirements
* **Microcontroller:** ESP32 (DevKit V1 or similar)
* **Meters:** 3x Analog DC Moving Coil Meters (e.g., 0-1mA or 0-50µA)
* **Time Module:** EverSet ES100-MOD (BPSK WWVB Receiver) **[Optional]**
* **RTC:** DS3231 I2C module (for offline drift protection)
* **LEDs:** WS2811 Integrated LEDs
* **Power:** 5V DC (Standard ESP32 Dev Board, same as Teensy Voter).
* **Support Components:** * 3x Precision Resistors (Calibrated for meter full-scale deflection)
    * 3x 0.1µF Capacitors (PWM smoothing for the needles)
    * 1x Logic Level Shifter (3.3V to 5V for WS2811 Data line)

---

## 3. System Architecture & Logic

### Time Synchronization Hierarchy
1. **Primary:** NTP via `time.h` (Fastest lock on boot).
2. **Secondary:** WWVB via EverSet ES100 (**Optional** atomic reference).
3. **Tertiary/Local:** DS3231 RTC (Maintains time during power loss/network outages).
4. **Timezone:** POSIX string for Selma, TX (handles DST automatically).

### Network & Captive Portal
* **Auto-Connect:** Tries saved WiFi; if fails, launches `MeterClock_Config` AP.
* **Captive Portal:** Intercepts DNS to trigger the "Sign-in to Network" popup on iOS/Android/Windows.
* **UI Features:** WiFi scan list (sorted by RSSI), open network support, and manual NTP/Timezone entry.

### Lighting & Display
* **Hour Meter:** Selectable 12h ($Hour \pmod{12}$) or 24h ($Hour$) scale.
* **Lighting Profiles:**
    * **Day Mode:** High intensity, user-defined RGB colors.
    * **Night Mode:** Low intensity, "warm" colors, user-defined Start/End times.

---

## 4. Software Modules

### A. Core Control (C++/Arduino)
* **Web Server:** `ESPAsyncWebServer` for real-time dashboard control.
* **Storage:** `LittleFS` for persistent `.json` config storage (SSID, Timezone, LED Colors).
* **PWM:** `ledcWrite` at 5kHz for silent, 10-bit resolution needle movement.

### B. LED & Transitions (`FastLED`)
* Implements a state machine to check `currentTime` against `nightModeStart`.
* Uses a "Step-Fade" to transition between Day/Night brightness smoothly.

---

## 5. Implementation Roadmap
1. **Phase 1 (Breadboard):** Test the ES100 and RTC on the I2C bus. Verify I2C address stability.
2. **Phase 2 (Networking):** Flash the Captive Portal code; verify mobile device redirect and SSID scanning.
3. **Phase 3 (Calibration):** Map PWM values (0-1023) to the physical meter faces and calibrate resistors.
4. **Phase 4 (Final Build):** Shield the WWVB antenna from the WS2811 noise; finalize the enclosure.

---

## 6. Project Notes (Antigravity Reference)
* **Goal:** A high-reliability horological instrument with a vintage-tech aesthetic.
* **System Admin Goal:** Ensure the web UI is responsive and provides diagnostic data (WiFi strength, NTP sync status, WWVB signal quality).