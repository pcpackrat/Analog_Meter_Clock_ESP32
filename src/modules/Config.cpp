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
  _smoothSeconds = _prefs.getBool("smoothSec", false);
  _useNTP = _prefs.getBool("useNTP", true);
  _manualTime = _prefs.getULong64("manualTime", 0);
  _useSharedColors = _prefs.getBool("sharedColors", true);
  _dayColor = _prefs.getUInt("dayColor", 0xFFFFFF);
  _dayColorM = _prefs.getUInt("dayColorM", 0xFFFFFF);
  _dayColorS = _prefs.getUInt("dayColorS", 0xFFFFFF);
  _nightColor = _prefs.getUInt("nightColor", 0xFF00FF);
  _nightColorM = _prefs.getUInt("nightColorM", 0xFF00FF);
  _nightColorS = _prefs.getUInt("nightColorS", 0xFF00FF);
  _dayBrightness = _prefs.getUChar("dayBright", 200);
  _nightBrightness = _prefs.getUChar("nightBright", 50);
  _nightStart = _prefs.getUChar("nightStart", 21);
  _nightStartMin = _prefs.getUChar("nightStartMin", 0);
  _nightEnd = _prefs.getUChar("nightEnd", 7);
  _nightEndMin = _prefs.getUChar("nightEndMin", 0);
  _nightStart2 = _prefs.getUChar("nightStart2", 2);
  _nightStartMin2 = _prefs.getUChar("nightStartMin2", 0);
  _nightEnd2 = _prefs.getUChar("nightEnd2", 13);
  _nightEndMin2 = _prefs.getUChar("nightEndMin2", 0);

  // Load Calibration (Default 0-1023)
  _calHMin = _prefs.getUShort("calHMin", 0);
  _calHMax = _prefs.getUShort("calHMax", 1023);
  _calMMin = _prefs.getUShort("calMMin", 0);
  _calMMax = _prefs.getUShort("calMMax", 1023);
  _calSMin = _prefs.getUShort("calSMin", 0);
  _calSMax = _prefs.getUShort("calSMax", 1023);

  // Load Mid Calibration (Default 512 approx half)
  _calHMid = _prefs.getUShort("calHMid", 512);
  _calMMid = _prefs.getUShort("calMMid", 512);
  _calSMid = _prefs.getUShort("calSMid", 512);
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

bool Config::getSmoothSeconds() { return _smoothSeconds; }
void Config::saveSmoothSeconds(bool smooth) {
  _smoothSeconds = smooth;
  _prefs.putBool("smoothSec", smooth);
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
bool Config::getUseSharedColors() { return _useSharedColors; }
void Config::saveUseSharedColors(bool use) {
  _useSharedColors = use;
  _prefs.putBool("sharedColors", use);
}

uint32_t Config::getDayColor() { return _dayColor; }
void Config::saveDayColor(uint32_t color) {
  _dayColor = color;
  _prefs.putUInt("dayColor", color);
}

uint32_t Config::getDayColorMinute() { return _dayColorM; }
void Config::saveDayColorMinute(uint32_t color) {
  _dayColorM = color;
  _prefs.putUInt("dayColorM", color);
}

uint32_t Config::getDayColorSecond() { return _dayColorS; }
void Config::saveDayColorSecond(uint32_t color) {
  _dayColorS = color;
  _prefs.putUInt("dayColorS", color);
}

uint32_t Config::getNightColor() { return _nightColor; }
void Config::saveNightColor(uint32_t color) {
  _nightColor = color;
  _prefs.putUInt("nightColor", color);
}

uint32_t Config::getNightColorMinute() { return _nightColorM; }
void Config::saveNightColorMinute(uint32_t color) {
  _nightColorM = color;
  _prefs.putUInt("nightColorM", color);
}

uint32_t Config::getNightColorSecond() { return _nightColorS; }
void Config::saveNightColorSecond(uint32_t color) {
  _nightColorS = color;
  _prefs.putUInt("nightColorS", color);
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

uint8_t Config::getNightStart2() { return _nightStart2; }
void Config::saveNightStart2(uint8_t hour) {
  _nightStart2 = hour;
  _prefs.putUChar("nightStart2", hour);
}

uint8_t Config::getNightStartMinute2() { return _nightStartMin2; }
void Config::saveNightStartMinute2(uint8_t min) {
  _nightStartMin2 = min;
  _prefs.putUChar("nightStartMin2", min);
}

uint8_t Config::getNightEnd2() { return _nightEnd2; }
void Config::saveNightEnd2(uint8_t hour) {
  _nightEnd2 = hour;
  _prefs.putUChar("nightEnd2", hour);
}

uint8_t Config::getNightEndMinute2() { return _nightEndMin2; }
void Config::saveNightEndMinute2(uint8_t min) {
  _nightEndMin2 = min;
  _prefs.putUChar("nightEndMin2", min);
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

uint16_t Config::getCalHMid() { return _calHMid; }
void Config::saveCalHMid(uint16_t val) {
  _calHMid = val;
  _prefs.putUShort("calHMid", val);
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

uint16_t Config::getCalMMid() { return _calMMid; }
void Config::saveCalMMid(uint16_t val) {
  _calMMid = val;
  _prefs.putUShort("calMMid", val);
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

uint16_t Config::getCalSMid() { return _calSMid; }
void Config::saveCalSMid(uint16_t val) {
  _calSMid = val;
  _prefs.putUShort("calSMid", val);
}
