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

private:
  Preferences _prefs;
};
