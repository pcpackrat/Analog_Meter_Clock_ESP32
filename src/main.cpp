#include "modules/Config.h"
#include "modules/GlobalState.h"
#include "modules/Lighting.h"
#include "modules/Meter.h"
#include "modules/Network.h"
#include "modules/TimeManager.h"
#include <Arduino.h>

// Global State Definitions
bool g_isCalibrationMode = false;
int g_calOverrideValues[3] = {-1, -1, -1};

// Hardware Pin Configuration
#define PIN_METER_H 25
#define PIN_METER_M 26
#define PIN_METER_S 27
#define PIN_TZ_SWITCH 4 // GPIO4 for UTC Switch (Active Low)
#define PIN_TZ_GND 19   // GPIO19 as Software Ground for Switch
// Lighting Pin is defined in Lighting.h (Default 13)

Config config;
NetworkManager network(config);
TimeManager timeManager(config);

Meter meterH(PIN_METER_H, 0);
Meter meterM(PIN_METER_M, 1);
Meter meterS(PIN_METER_S, 2);

Lighting lighting;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Analog Meter Clock...");

  pinMode(PIN_TZ_SWITCH, INPUT_PULLUP);
  pinMode(PIN_TZ_GND, OUTPUT);
  digitalWrite(PIN_TZ_GND, LOW); // Acts as a Ground pin

  // Initialize Modules

  // 1. Config (Load Settings)
  config.begin();

  // 2. Hardware
  meterH.begin();
  meterM.begin();
  meterS.begin();
  lighting.begin();

  // 3. Network
  network.begin(); // connects to WiFi or AP

  // 4. Time
  timeManager.begin(); // Syncs NTP/RTC
}

// Piecewise linear mapping
float mapPiecewise(float x, float in_min, float in_mid, float in_max, float out_min,
                  float out_mid, float out_max) {
  if (x <= in_mid) {
    return (x - in_min) * (out_mid - out_min) / (in_mid - in_min) + out_min;
  } else {
    return (x - in_mid) * (out_max - out_mid) / (in_max - in_mid) + out_mid;
  }
}

void loop() {
  // 1. Update Network (Run this as often as possible for DNS)
  network.loop();
  timeManager.update();

  // Check UTC Switch (Active Low)
  timeManager.setOverrideUTC(digitalRead(PIN_TZ_SWITCH) == LOW);

  // 2. Logic & UI Updates (Throttled to avoid starving Network/Interrupts)
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 50) { // 20Hz update rate
    lastUpdate = millis();

    // Get Time
    float h = timeManager.getHour();
    float m = timeManager.getMinute();
    float s = config.getSmoothSeconds() ? timeManager.getExactSecond() : timeManager.getSecond();

    // Logic: Map Time to Meters (with Calibration)
    float valH = 0;
    float valM = 0;
    float valS = 0;

    if (g_isCalibrationMode) {
      // Direct Calibration Override
      if (millis() % 1000 == 0)
        Serial.println("In Calibration Mode Loop"); // Debug
      valH = (g_calOverrideValues[0] != -1) ? g_calOverrideValues[0] : 0;
      valM = (g_calOverrideValues[1] != -1) ? g_calOverrideValues[1] : 0;
      valS = (g_calOverrideValues[2] != -1) ? g_calOverrideValues[2] : 0;
    } else {
      // Standard Time Mode with Piecewise Linear Mapping
      if (config.get12H()) {
        valH = mapPiecewise(h, 0, 6, 12, config.getCalHMin(),
                            config.getCalHMid(), config.getCalHMax());
      } else {
        valH = mapPiecewise(h, 0, 12, 24, config.getCalHMin(),
                            config.getCalHMid(), config.getCalHMax());
      }
      valM = mapPiecewise(m, 0, 30, 60, config.getCalMMin(),
                          config.getCalMMid(), config.getCalMMax());
      valS = mapPiecewise(s, 0, 30, 60, config.getCalSMin(),
                          config.getCalSMid(), config.getCalSMax());
    }

    // Update Outputs (Target)
    meterH.setTarget(valH);
    meterM.setTarget(valM);
    meterS.setTarget(valS);

    // Verify Connection State
    bool isConnected = (WiFi.status() == WL_CONNECTED);
    bool isTimeSet = timeManager.isTimeSet();
    bool showConnectionError = (!isConnected && !isTimeSet);

    // Update Lighting
    lighting.update(timeManager.getHour24(), m, config, showConnectionError);
    lighting.show();
  }

  // Fast loop for smooth ramping
  meterH.update();
  meterM.update();
  meterS.update();
} // End loop
