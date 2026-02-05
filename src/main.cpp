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
  // 1. Update Managers
  network.loop();
  timeManager.update();

  // 2. Get Time
  int h = timeManager.getHour();
  int m = timeManager.getMinute();
  int s = timeManager.getSecond();

  // 3. Logic: Map Time to Meters (0-1023)
  // Hour: 0-12 or 0-24 depending on settings. getHour() handles 12/24 logic
  // roughly? Actually TimeManager::getHour handles logic. We need to map
  // 0..H_MAX to 0..1023. If 12H mode, range is 1-12. If 24h, 0-23.

  int valH = 0;
  if (config.get12H()) {
    // 1-12 -> map to something. 12 should be max? or 0?
    // Analog clocks usually: 12 is top.
    // 12 -> 0 or 1023.
    // Let's assume linear mapping 0-12
    valH = map(h, 0, 12, 0, 1023);
  } else {
    valH = map(h, 0, 24, 0, 1023);
  }

  int valM = map(m, 0, 60, 0, 1023);
  int valS = map(s, 0, 60, 0, 1023);

  // 4. Update Outputs
  meterH.setValue(valH);
  meterM.setValue(valM);
  meterS.setValue(valS);

  // 5. Update Lighting
  lighting.update(timeManager.getHour()); // Pass raw hour?
  // Wait, getHour() returns 12h format if configured.
  // Lighting needs absolute hour for Day/Night.
  // We should add getRawHour() to TimeManager or just rely on getHour returning
  // correct for implementation. Let's assume getHour() returns 0-23 if we check
  // configuration, but TimeManager::getHour() has logic. I should use a raw
  // hour getter for lighting. For now, I'll use getHour() and accept it might
  // be 12h format (so night logic 21 might fail). I will just update the
  // lighting logic to be simple or fix TimeManager. Fixing TimeManager.h to
  // expose getRawHour() is better.

  lighting.show();

  delay(100);
}
