#pragma once
#include <Arduino.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>


class NetworkManager {
public:
  NetworkManager();
  void begin();
  void loop(); // Handle DNS requests

private:
  AsyncWebServer _server;
  DNSServer _dnsServer;
  bool _isAP;

  void setupAP();
  void connectWiFi();
  void setupRoutes();
};
