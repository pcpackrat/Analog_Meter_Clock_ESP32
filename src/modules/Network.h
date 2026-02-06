#pragma once
#include <Arduino.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>

#include "Config.h"

class NetworkManager {
public:
  NetworkManager(Config &config);
  void begin();
  void loop(); // Handle DNS requests

private:
  Config &_config;
  AsyncWebServer _server;
  DNSServer _dnsServer;
  bool _isAP;

  // WiFi scan caching to prevent blocking
  String _cachedScanHTML;
  unsigned long _lastScanTime;
  bool _scanInProgress;

  void setupAP();
  String getCommonStyle();
  void connectWiFi();
  void setupRoutes();
  void startAsyncScan();
  String getWiFiScanHTML();
  void sendWifiPage(AsyncWebServerRequest *request);
};
