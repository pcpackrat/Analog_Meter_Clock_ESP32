#include "modules/Config.h"
#include "modules/Lighting.h"
#include "modules/Meter.h"
#include "modules/Network.h"
#include "modules/TimeManager.h"
#include <Arduino.h>

// Hardware Pin Configuration
#define PIN_METER_H 25
#define PIN_METER_M 26
#define PIN_METER_S 27
#define PIN_TZ_SWITCH 4 // GPIO4 for UTC Switch (Active Low)
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
    int h = timeManager.getHour();
    int m = timeManager.getMinute();
    int s = timeManager.getSecond();

    // Logic: Map Time to Meters (with Calibration)
    int valH = 0;
    if (config.get12H()) {
      valH =
          map(h, 0, 12, config.getCalHMin(), config.getCalHMax()); // 12H Mode
    } else {
      valH =
          map(h, 0, 24, config.getCalHMin(), config.getCalHMax()); // 24H Mode
    }

    int valM = map(m, 0, 60, config.getCalMMin(), config.getCalMMax());
    int valS = map(s, 0, 60, config.getCalSMin(), config.getCalSMax());

    // Update Outputs
    meterH.setValue(valH);
    meterM.setValue(valM);
    meterS.setValue(valS);

    // Verify Connection State
    bool isConnected = (WiFi.status() == WL_CONNECTED);
    bool isTimeSet = timeManager.isTimeSet();
    bool showConnectionError = (!isConnected && !isTimeSet);

    // Update Lighting
    lighting.update(timeManager.getHour24(), m, config, showConnectionError);
    lighting.show();
  }
}
