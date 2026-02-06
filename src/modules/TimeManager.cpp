#include "TimeManager.h"
#include "esp_sntp.h"
#include <WiFi.h>
#include <sys/time.h>
#include <time.h>

void timeAvailable(struct timeval *t) {
  Serial.println("Got time adjustment from NTP!");
}

TimeManager::TimeManager(Config &config)
    : _config(config), _rtcFound(false), _isUTC(false), _lastRTCUpdate(0) {}

void TimeManager::setUseNTP(bool enabled) {
  // Only act if state changes or if force re-init is needed (simplified here)
  if (enabled) {
    String server = _config.getNTP();
    if (server.length() == 0) {
      server = "time.google.com"; // Fallback
    }
    Serial.printf("Enabling NTP with server: '%s'\r\n", server.c_str());

    // Check if we can resolve the hostname
    IPAddress ntpIP;
    if (WiFi.hostByName(server.c_str(), ntpIP)) {
      Serial.print("DNS Lookup Success. NTP IP: ");
      Serial.println(ntpIP);
    } else {
      Serial.println("DNS Lookup FAILED for NTP server!");
    }

    // Log the exact TZ string being used - CRITICAL for debugging
    String tz = _config.getTimezone();
    if (tz.isEmpty()) {
      tz = "CST6CDT,M3.2.0,M11.1.0"; // Fallback to Chicago
    }
    Serial.printf("Configuring Time with TZ: '%s' and Server: '%s'\r\n",
                  tz.c_str(), server.c_str());

    if (sntp_enabled()) {
      sntp_stop();
    }
    sntp_setoperatingmode(SNTP_OPMODE_POLL);

    // Manual Init Sequence to ensure reliable start
    sntp_setservername(0, "time.google.com");
    if (server != "time.google.com") {
      sntp_setservername(1, server.c_str());
    }
    sntp_set_time_sync_notification_cb(timeAvailable);
    sntp_init();

    // Manually apply timezone
    setenv("TZ", tz.c_str(), 1);
    tzset();

    Serial.println("NTP Initialized via Manual Sequence (Google Primary). "
                   "Waiting for sync...\r\n");
  } else {
    if (sntp_enabled()) {
      Serial.println("Disabling NTP...\r\n");
      sntp_stop();
    }
  }
}

void TimeManager::setOverrideUTC(bool enabled) {
  if (enabled != _isUTC) {
    _isUTC = enabled;
    if (_isUTC) {
      Serial.println("Switching to UTC\r\n");
      configTzTime("UTC0", _config.getNTP().c_str());
    } else {
      Serial.println("Switching to Local Time\r\n");
      configTzTime(_config.getTimezone().c_str(), _config.getNTP().c_str());
    }
  }
}

void TimeManager::begin() {
  sntp_set_time_sync_notification_cb(timeAvailable);
  // Setup NTP if enabled
  if (_config.getUseNTP()) {
    setUseNTP(true);
  } else {
    Serial.println("NTP Disabled by config\r\n");
  }

  // Setup RTC
  if (_rtc.begin()) {
    _rtcFound = true;
    Serial.println("RTC Found\r\n");
    if (_rtc.lostPower()) {
      Serial.println("RTC lost power!\r\n");
    }

    // Attempt to set system time from RTC immediately on boot
    // This is useful if no WiFi
    syncSystemToRTC();

  } else {
    Serial.println("Couldn't find RTC\r\n");
  }
}

void TimeManager::update() {
  struct tm timeinfo;

  // Debug logging removed

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
    Serial.println("Synced system time from RTC\r\n");
  }
}

void TimeManager::syncRTCToSystem() {
  if (!_rtcFound)
    return;

  time_t now;
  time(&now);
  if (now > 100000) { // Valid timestamp
    _rtc.adjust(DateTime(now));
    Serial.println("Updated RTC from System (NTP)\r\n");
  }
}

int TimeManager::getHour() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 0)) {
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

int TimeManager::getHour24() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 0)) {
    // Serial.printf("TimeManager::getHour24: %d\n", timeinfo.tm_hour);
    return timeinfo.tm_hour;
  }
  return 0;
}

int TimeManager::getMinute() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 0))
    return timeinfo.tm_min;
  return 0;
}

int TimeManager::getSecond() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 0))
    return timeinfo.tm_sec;
  return 0;
}

String TimeManager::getFormattedTime() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 0)) {
    char timeStr[64];
    // Format: HH:MM:SS Month-Day-Year Timezone
    // %B = Full Month Name, %d = Day, %Y = Year, %Z = Timezone
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S %B-%d-%Y %Z", &timeinfo);
    return String(timeStr);
  }
  return "Time Not Set";
}

bool TimeManager::isTimeSet() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 0)) {
    return (timeinfo.tm_year > (2020 - 1900));
  }
  return false;
}
