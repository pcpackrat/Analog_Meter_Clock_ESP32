#include "TimeManager.h"
#include <sys/time.h>

TimeManager::TimeManager(Config &config)
    : _config(config), _rtcFound(false), _lastRTCUpdate(0) {}

void TimeManager::begin() {
  // Setup NTP
  configTzTime(_config.getTimezone().c_str(), _config.getNTP().c_str());

  // Setup RTC
  if (_rtc.begin()) {
    _rtcFound = true;
    Serial.println("RTC Found");
    if (_rtc.lostPower()) {
      Serial.println("RTC lost power!");
    }

    // Attempt to set system time from RTC immediately on boot
    // This is useful if no WiFi
    syncSystemToRTC();

  } else {
    Serial.println("Couldn't find RTC");
  }
}

void TimeManager::update() {
  struct tm timeinfo;

  // Check if system time is valid (year > 2000)
  // If we have valid NTP time (system time), we should update RTC periodically
  if (getLocalTime(&timeinfo, 0)) {
    if (timeinfo.tm_year > (2020 - 1900)) {
      // We have valid time.
      // Update RTC every hour
      if (millis() - _lastRTCUpdate > 3600000) {
        syncRTCToSystem();
        _lastRTCUpdate = millis();
      }
    } else {
      // System time invalid (1970?), try fallback to RTC if we haven't already
      // (We did at boot, but maybe we should retry if network fails?)
    }
  }
}

void TimeManager::syncSystemToRTC() {
  if (!_rtcFound)
    return;

  DateTime now = _rtc.now();
  // basic validation
  if (now.year() > 2020) {
    struct timeval tv = {(time_t)now.unixtime(), 0};
    settimeofday(&tv, NULL);
    Serial.println("Synced system time from RTC");
  }
}

void TimeManager::syncRTCToSystem() {
  if (!_rtcFound)
    return;

  time_t now;
  time(&now);
  if (now > 100000) { // Valid timestamp
    _rtc.adjust(DateTime(now));
    Serial.println("Updated RTC from System (NTP)");
  }
}

int TimeManager::getHour() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    if (_config.get12H()) {
      int h = timeinfo.tm_hour % 12;
      if (h == 0)
        h = 12;
      return h;
    }
    return timeinfo.tm_hour;
  }
  return 0; // Invalid
}

int TimeManager::getMinute() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo))
    return timeinfo.tm_min;
  return 0;
}

int TimeManager::getSecond() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo))
    return timeinfo.tm_sec;
  return 0;
}
