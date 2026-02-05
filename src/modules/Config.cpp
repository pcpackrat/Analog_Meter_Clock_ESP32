#include "Config.h"

Config::Config() {}

void Config::begin() { _prefs.begin("clock-cfg", false); }

String Config::getSSID() { return _prefs.getString("ssid", ""); }

String Config::getWifiPass() { return _prefs.getString("pass", ""); }

void Config::saveWiFi(String ssid, String pass) {
  _prefs.putString("ssid", ssid);
  _prefs.putString("pass", pass);
}

String Config::getTimezone() {
  // Default to CST6CDT per user context (or similar default)
  return _prefs.getString("tz", "CST6CDT,M3.2.0,M11.1.0");
}

void Config::saveTimezone(String tz) { _prefs.putString("tz", tz); }

String Config::getNTP() { return _prefs.getString("ntp", "pool.ntp.org"); }

void Config::saveNTP(String ntp) { _prefs.putString("ntp", ntp); }

bool Config::get12H() { return _prefs.getBool("12h", true); }

void Config::save12H(bool is12h) { _prefs.putBool("12h", is12h); }
