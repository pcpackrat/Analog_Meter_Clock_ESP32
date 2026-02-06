#pragma once
#include "Config.h"
#include <Arduino.h>
#include <RTClib.h> // Ensure you have this lib
#include <time.h>

class TimeManager {
public:
  TimeManager(Config &config);
  void begin();
  void update(); // Call in loop
  void setOverrideUTC(bool enabled);
  void setUseNTP(bool enabled);

  int getHour();
  int getHour24();
  int getMinute();
  int getSecond();
  String getFormattedTime();
  bool isTimeSet();

private:
  Config &_config;
  RTC_DS3231 _rtc;
  bool _rtcFound;
  bool _isUTC; // Track current state
  unsigned long _lastRTCUpdate;

  void syncRTCToSystem();
  void syncSystemToRTC();
};
