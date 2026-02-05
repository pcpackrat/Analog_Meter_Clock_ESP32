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

  int getHour();
  int getMinute();
  int getSecond();

private:
  Config &_config;
  RTC_DS3231 _rtc;
  bool _rtcFound;
  unsigned long _lastRTCUpdate;

  void syncRTCToSystem();
  void syncSystemToRTC();
};
