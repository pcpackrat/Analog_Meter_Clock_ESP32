#include "Config.h"

Config::Config() {}

void Config::begin() {
  _prefs.begin("clock-cfg", false);

  // Load into RAM cache
  _ssid = _prefs.getString("ssid", "");
  _pass = _prefs.getString("pass", "");
  _tz = _prefs.getString("tz", "CST6CDT,M3.2.0,M11.1.0");
  _tz2 = _prefs.getString("tz2", "UTC0");
  _ntp = _prefs.getString("ntp", "pool.ntp.org");
  _is12h = _prefs.getBool("12h", true);
  _useNTP = _prefs.getBool("useNTP", true);
  _manualTime = _prefs.getULong64("manualTime", 0);
  _dayColor = _prefs.getUInt("dayColor", 0xFFFFFF);
  _nightColor = _prefs.getUInt("nightColor", 0xFF00FF);
  _dayBrightness = _prefs.getUChar("dayBright", 200);
  _nightBrightness = _prefs.getUChar("nightBright", 50);
  _nightStart = _prefs.getUChar("nightStart", 21);
  _nightStartMin = _prefs.getUChar("nightStartMin", 0);
  _nightEnd = _prefs.getUChar("nightEnd", 7);
  _nightEndMin = _prefs.getUChar("nightEndMin", 0);

  // Load Calibration (Default 0-1023)
  _calHMin = _prefs.getUShort("calHMin", 0);
  _calHMax = _prefs.getUShort("calHMax", 1023);
  _calMMin = _prefs.getUShort("calMMin", 0);
  _calMMax = _prefs.getUShort("calMMax", 1023);
  _calSMin = _prefs.getUShort("calSMin", 0);
  _calSMax = _prefs.getUShort("calSMax", 1023);
}

String Config::getSSID() { return _ssid; }
String Config::getWifiPass() { return _pass; }

void Config::saveWiFi(String ssid, String pass) {
  _ssid = ssid;
  _pass = pass;
  _prefs.putString("ssid", ssid);
  _prefs.putString("pass", pass);
}

String Config::getTimezone() { return _tz; }
void Config::saveTimezone(String tz) {
  _tz = tz;
  _prefs.putString("tz", tz);
}

String Config::getNTP() { return _ntp; }
void Config::saveNTP(String ntp) {
  _ntp = ntp;
  _prefs.putString("ntp", ntp);
}

bool Config::get12H() { return _is12h; }
void Config::save12H(bool is12h) {
  _is12h = is12h;
  _prefs.putBool("12h", is12h);
}

// Time Source
bool Config::getUseNTP() { return _useNTP; }
void Config::saveUseNTP(bool useNTP) {
  _useNTP = useNTP;
  _prefs.putBool("useNTP", useNTP);
}

time_t Config::getManualTime() { return _manualTime; }
void Config::saveManualTime(time_t timestamp) {
  _manualTime = timestamp;
  _prefs.putULong64("manualTime", timestamp);
}

// Secondary Timezone
String Config::getTimezone2() { return _tz2; }
void Config::saveTimezone2(String tz) {
  _tz2 = tz;
  _prefs.putString("tz2", tz);
}

// LED Day/Night Settings
uint32_t Config::getDayColor() { return _dayColor; }
void Config::saveDayColor(uint32_t color) {
  _dayColor = color;
  _prefs.putUInt("dayColor", color);
}

uint32_t Config::getNightColor() {
  // Serial.printf("Config::getNightColor returning 0x%06X\n", _nightColor);
  return _nightColor;
}
void Config::saveNightColor(uint32_t color) {
  _nightColor = color;
  _prefs.putUInt("nightColor", color);
}

uint8_t Config::getDayBrightness() { return _dayBrightness; }
void Config::saveDayBrightness(uint8_t brightness) {
  _dayBrightness = brightness;
  _prefs.putUChar("dayBright", brightness);
}

uint8_t Config::getNightBrightness() { return _nightBrightness; }
void Config::saveNightBrightness(uint8_t brightness) {
  _nightBrightness = brightness;
  _prefs.putUChar("nightBright", brightness);
}

uint8_t Config::getNightStart() { return _nightStart; }
void Config::saveNightStart(uint8_t hour) {
  _nightStart = hour;
  _prefs.putUChar("nightStart", hour);
}

uint8_t Config::getNightStartMinute() { return _nightStartMin; }
void Config::saveNightStartMinute(uint8_t min) {
  _nightStartMin = min;
  _prefs.putUChar("nightStartMin", min);
}

uint8_t Config::getNightEnd() { return _nightEnd; }
void Config::saveNightEnd(uint8_t hour) {
  _nightEnd = hour;
  _prefs.putUChar("nightEnd", hour);
}

uint8_t Config::getNightEndMinute() { return _nightEndMin; }
void Config::saveNightEndMinute(uint8_t min) {
  _nightEndMin = min;
  _prefs.putUChar("nightEndMin", min);
}

// Calibration Settings
uint16_t Config::getCalHMin() { return _calHMin; }
void Config::saveCalHMin(uint16_t val) {
  _calHMin = val;
  _prefs.putUShort("calHMin", val);
}

uint16_t Config::getCalHMax() { return _calHMax; }
void Config::saveCalHMax(uint16_t val) {
  _calHMax = val;
  _prefs.putUShort("calHMax", val);
}

uint16_t Config::getCalMMin() { return _calMMin; }
void Config::saveCalMMin(uint16_t val) {
  _calMMin = val;
  _prefs.putUShort("calMMin", val);
}

uint16_t Config::getCalMMax() { return _calMMax; }
void Config::saveCalMMax(uint16_t val) {
  _calMMax = val;
  _prefs.putUShort("calMMax", val);
}

uint16_t Config::getCalSMin() { return _calSMin; }
void Config::saveCalSMin(uint16_t val) {
  _calSMin = val;
  _prefs.putUShort("calSMin", val);
}

uint16_t Config::getCalSMax() { return _calSMax; }
void Config::saveCalSMax(uint16_t val) {
  _calSMax = val;
  _prefs.putUShort("calSMax", val);
}
