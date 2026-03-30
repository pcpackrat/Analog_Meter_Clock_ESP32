#pragma once
#include <Arduino.h>
#include <Preferences.h>

class Config {
public:
  Config();
  void begin();

  // WiFi
  String getSSID();
  String getWifiPass();
  void saveWiFi(String ssid, String pass);

  // Settings
  String getTimezone();
  void saveTimezone(String tz);

  String getNTP();
  void saveNTP(String ntp);

  bool get12H();
  void save12H(bool is12h);

  bool getDisableDST();
  void saveDisableDST(bool disable);

  bool getSmoothSeconds();
  void saveSmoothSeconds(bool smooth);

  // Time Source
  bool getUseNTP(); // true = NTP, false = Manual
  void saveUseNTP(bool useNTP);

  time_t getManualTime(); // Unix timestamp
  void saveManualTime(time_t timestamp);

  // Secondary Timezone
  String getTimezone2();
  void saveTimezone2(String tz);

  bool getDisableDST2();
  void saveDisableDST2(bool disable);

  // LED Day/Night Settings
  bool getUseSharedColors();
  void saveUseSharedColors(bool use);

  uint32_t getDayColor(); // RGB as uint32_t (0xRRGGBB)
  void saveDayColor(uint32_t color);
  uint32_t getDayColorMinute();
  void saveDayColorMinute(uint32_t color);
  uint32_t getDayColorSecond();
  void saveDayColorSecond(uint32_t color);

  uint32_t getNightColor();
  void saveNightColor(uint32_t color);
  uint32_t getNightColorMinute();
  void saveNightColorMinute(uint32_t color);
  uint32_t getNightColorSecond();
  void saveNightColorSecond(uint32_t color);

  uint8_t getDayBrightness(); // 0-255
  void saveDayBrightness(uint8_t brightness);

  uint8_t getNightBrightness();
  void saveNightBrightness(uint8_t brightness);

  uint8_t getNightStart(); // Hour 0-23
  void saveNightStart(uint8_t hour);
  uint8_t getNightStartMinute();
  void saveNightStartMinute(uint8_t min);

  uint8_t getNightEnd(); // Hour 0-23
  void saveNightEnd(uint8_t hour);
  uint8_t getNightEndMinute();
  void saveNightEndMinute(uint8_t min);

  uint8_t getNightStart2();
  void saveNightStart2(uint8_t hour);
  uint8_t getNightStartMinute2();
  void saveNightStartMinute2(uint8_t min);

  uint8_t getNightEnd2();
  void saveNightEnd2(uint8_t hour);
  uint8_t getNightEndMinute2();
  void saveNightEndMinute2(uint8_t min);

  // Calibration Settings
  uint16_t getCalHMin();
  void saveCalHMin(uint16_t val);
  uint16_t getCalHMax();
  void saveCalHMax(uint16_t val);

  uint16_t getCalHMid();
  void saveCalHMid(uint16_t val);

  uint16_t getCalMMin();
  void saveCalMMin(uint16_t val);
  uint16_t getCalMMax();
  void saveCalMMax(uint16_t val);

  uint16_t getCalMMid();
  void saveCalMMid(uint16_t val);

  uint16_t getCalSMin();
  void saveCalSMin(uint16_t val);
  uint16_t getCalSMax();
  void saveCalSMax(uint16_t val);

  uint16_t getCalSMid();
  void saveCalSMid(uint16_t val);

private:
  Preferences _prefs;

  // RAM Cache
  String _ssid;
  String _pass;
  String _tz;
  String _tz2;
  String _ntp;
  bool _is12h;
  bool _disableDST;
  bool _disableDST2;
  bool _smoothSeconds;
  bool _useNTP;
  time_t _manualTime;
  bool _useSharedColors;
  uint32_t _dayColor;
  uint32_t _dayColorM;
  uint32_t _dayColorS;
  uint32_t _nightColor;
  uint32_t _nightColorM;
  uint32_t _nightColorS;
  uint8_t _dayBrightness;
  uint8_t _nightBrightness;
  uint8_t _nightStart;
  uint8_t _nightStartMin;
  uint8_t _nightEnd;
  uint8_t _nightEndMin;
  uint8_t _nightStart2;
  uint8_t _nightStartMin2;
  uint8_t _nightEnd2;
  uint8_t _nightEndMin2;

  // Calibration Cache
  uint16_t _calHMin;
  uint16_t _calHMax;
  uint16_t _calMMin;
  uint16_t _calMMax;
  uint16_t _calSMin;
  uint16_t _calSMax;
  uint16_t _calHMid;
  uint16_t _calMMid;
  uint16_t _calSMid;
};
