#include "Network.h"

#define AP_SSID "MeterClock_Config"
#define DNS_PORT 53

NetworkManager::NetworkManager(Config &config)
    : _server(80), _dnsServer(), _config(config), _isAP(false) {}

void NetworkManager::begin() {
  _config.begin(); // Ensure config is initialized
  connectWiFi();

  if (WiFi.status() != WL_CONNECTED) {
    setupAP();
  } else {
    Serial.print("Connected to WiFi. IP: ");
    Serial.println(WiFi.localIP());
  }

  setupRoutes();
  _server.begin();
}

void NetworkManager::connectWiFi() {
  WiFi.mode(WIFI_STA);
  String ssid = _config.getSSID();
  String pass = _config.getWifiPass();

  if (ssid.length() > 0) {
    Serial.println("Connecting to saved WiFi: " + ssid);
    WiFi.begin(ssid.c_str(), pass.c_str());

    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 15) {
      delay(500);
      Serial.print(".");
      retries++;
    }
    Serial.println();
  } else {
    Serial.println("No saved WiFi credentials.");
  }
}

void NetworkManager::setupAP() {
  Serial.println("Starting Access Point...");
  _isAP = true;
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID);

  _dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  _dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
}

String NetworkManager::getWiFiScanHTML() {
  String list = "<h3>Available Networks</h3><ul>";
  int n = WiFi.scanNetworks();
  if (n == 0) {
    list += "<li>No networks found</li>";
  } else {
    for (int i = 0; i < n; ++i) {
      list += "<li>";
      list += WiFi.SSID(i);
      list += " (";
      list += WiFi.RSSI(i);
      list += " dBm)";
      list += " <a href='#' "
              "onclick=\"document.getElementsByName('ssid')[0].value='" +
              WiFi.SSID(i) + "'\">Select</a>";
      list += "</li>";
    }
  }
  list += "</ul>";
  return list;
}

void NetworkManager::setupRoutes() {
  _server.onNotFound([this](AsyncWebServerRequest *request) {
    if (_isAP) {
      request->redirect("http://" + WiFi.softAPIP().toString() + "/");
    } else {
      request->send(404, "text/plain", "Not found");
    }
  });

  _server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
    request->send(200, "text/html",
                  "<h1>Meter Clock Dashboard</h1><a href='/wifi'>Configure "
                  "WiFi</a><br><a href='/settings'>Settings</a>");
  });

  _server.on("/wifi", HTTP_GET, [this](AsyncWebServerRequest *request) {
    String html = "<html><head><meta name='viewport' "
                  "content='width=device-width, initial-scale=1'></head><body>";
    html += "<h1>WiFi Configuration</h1>";
    html += getWiFiScanHTML();
    html += "<h3>Manual Connect</h3>";
    html += "<form action='/save_wifi' method='POST'>";
    html += "SSID: <input type='text' name='ssid' placeholder='SSID' value='" +
            _config.getSSID() + "'><br>";
    html +=
        "Pass: <input type='password' name='pass' placeholder='Password'><br>";
    html += "<input type='submit' value='Save & Connect'>";
    html += "</form><br><a href='/'>Back</a>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  _server.on("/save_wifi", HTTP_POST, [this](AsyncWebServerRequest *request) {
    String ssid = request->arg("ssid");
    String pass = request->arg("pass");
    if (ssid.length() > 0) {
      Serial.println("Saving WiFi Creds: " + ssid);
      _config.saveWiFi(ssid, pass);
      WiFi.begin(ssid.c_str(), pass.c_str());
    }
    request->send(200, "text/html",
                  "Rebooting/Connecting... <a href='/'>Back</a>");
  });

  _server.on("/settings", HTTP_GET, [this](AsyncWebServerRequest *request) {
    String tz = _config.getTimezone();
    String ntp = _config.getNTP();
    bool h12 = _config.get12H();

    String html = "<html><head><meta name='viewport' "
                  "content='width=device-width, initial-scale=1'></head><body>";
    html += "<h1>Settings</h1>";
    html += "<form action='/save_settings' method='POST'>";
    html += "Timezone (POSIX): <input type='text' name='timezone' value='" +
            tz + "'><br>";
    html +=
        "NTP Server: <input type='text' name='ntp' value='" + ntp + "'><br>";
    html += "Format: <select name='h12'>";
    html +=
        "<option value='0'" + String(!h12 ? " selected" : "") + ">24H</option>";
    html +=
        "<option value='1'" + String(h12 ? " selected" : "") + ">12H</option>";
    html += "</select><br>";
    html += "<input type='submit' value='Save'>";
    html += "</form><br><a href='/'>Back</a>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  _server.on("/save_settings", HTTP_POST,
             [this](AsyncWebServerRequest *request) {
               if (request->hasArg("timezone"))
                 _config.saveTimezone(request->arg("timezone"));
               if (request->hasArg("ntp"))
                 _config.saveNTP(request->arg("ntp"));
               if (request->hasArg("h12"))
                 _config.save12H(request->arg("h12") == "1");

               request->send(200, "text/html",
                             "Settings Saved. <a href='/settings'>Back</a>");
             });
}

void NetworkManager::loop() {
  if (_isAP) {
    _dnsServer.processNextRequest();
  }
}
