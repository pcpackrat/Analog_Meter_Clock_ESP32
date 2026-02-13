#include "Network.h"
#include "GlobalState.h"
#include "TimeManager.h"
#include <Update.h>

#include <sys/time.h>

extern TimeManager timeManager;

#define AP_SSID "MeterClock_Config"
#define DNS_PORT 53

NetworkManager::NetworkManager(Config &config)
    : _server(80), _dnsServer(), _config(config), _isAP(false) {}

void NetworkManager::begin() {
  _config.begin(); // Ensure config is initialized
  connectWiFi();

  if (WiFi.status() != WL_CONNECTED) {
    setupAP();

    // Wait for AP to stabilize before starting web server
    Serial.println("Waiting for AP to stabilize...");
    delay(2000);
  } else {
    Serial.print("Connected to WiFi. IP: ");
    Serial.println(WiFi.localIP());
  }

  setupRoutes();
  _server.begin();
  Serial.println("Network Manager Initialized");
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
  Serial.println("\n========== STARTING ACCESS POINT ==========");
  _isAP = true;

  // Disable WiFi persistence to prevent NVS corruption
  WiFi.persistent(false);

  // Set WiFi mode to AP
  Serial.print("Current WiFi Mode: ");
  Serial.println(WiFi.getMode());
  WiFi.mode(WIFI_AP);
  Serial.print("WiFi Mode set to: WIFI_AP\n");

  // Disable WiFi sleep for better responsiveness
  WiFi.setSleep(false);
  Serial.println("WiFi Sleep disabled");

  // Configure static IP for AP
  IPAddress apIP(10, 5, 5, 5);
  IPAddress subnet(255, 255, 255, 0);

  Serial.print("Configuring AP IP to: ");
  Serial.println(apIP);
  Serial.print("Netmask: ");
  Serial.println(subnet);

  if (WiFi.softAPConfig(apIP, apIP, subnet)) {
    Serial.println("✓ AP Config successful");
  } else {
    Serial.println("❌ AP Config failed!");
  }

  // Start the AP
  Serial.print("Starting AP SSID: ");
  Serial.print(AP_SSID);
  Serial.println(" (No password, Channel 1, Max 4 clients)");

  if (WiFi.softAP(AP_SSID, "", 1, false, 4)) {
    Serial.println("✓ softAP started successfully\n");
  } else {
    Serial.println("❌ softAP failed to start!\n");
  }

  // Print AP status
  Serial.println("--- AP Status ---");
  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());
  Serial.print("AP MAC Address: ");
  Serial.println(WiFi.softAPmacAddress());
  Serial.print("AP Hostname: ");
  Serial.println(WiFi.getHostname());
  Serial.print("Connected Stations: ");
  Serial.println(WiFi.softAPgetStationNum());

  // Start DNS server
  Serial.println("\n--- Starting DNS Server ---");
  _dnsServer.setErrorReplyCode(DNSReplyCode::NoError);

  if (_dnsServer.start(DNS_PORT, "*", apIP)) {
    Serial.print("✓ DNS Server started on port ");
    Serial.print(DNS_PORT);
    Serial.print(" @ ");
    Serial.println(apIP);
  } else {
    Serial.println("❌ DNS Server failed to start!");
  }

  Serial.println("=========================================\n");
}

String NetworkManager::getWiFiScanHTML() {
  // WiFi scanning disabled - causes watchdog timeout in AP mode
  return "<p><strong>WiFi scanning is not available.</strong></p>"
         "<p>Please manually enter your network SSID below.</p>";
}

void NetworkManager::setupRoutes() {
  _server.onNotFound([this](AsyncWebServerRequest *request) {
    Serial.print("[HTTP] 404 Not Found: ");
    Serial.print(request->url());
    Serial.print(" from ");
    Serial.println(request->client()->remoteIP());

    if (_isAP) {
      String redirectURL = "http://" + WiFi.softAPIP().toString() + "/";
      Serial.print("  → Redirecting to: ");
      Serial.println(redirectURL);
      request->redirect(redirectURL);
    } else {
      request->send(404, "text/plain", "Not found");
    }
  });

  // Captive Portal Detection Routes
  if (_isAP) {
    // Captive Portal API (RFC 8910)
    _server.on(
        "/captive-portal-api", HTTP_GET, [](AsyncWebServerRequest *request) {
          Serial.println("[HTTP] GET /captive-portal-api (Captive Portal API)");
          AsyncWebServerResponse *response = request->beginResponse(
              200, "application/captive+json",
              "{\"captive\":true,\"user-portal-url\":\"http://10.5.5.5/\"}");
          response->addHeader("Cache-Control",
                              "no-cache, no-store, must-revalidate");
          request->send(response);
        });

    // Android Captive Portal Detection
    _server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println("[HTTP] GET /generate_204 (Android)");
      request->redirect("http://10.5.5.5/");
    });

    _server.on("/gen_204", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println("[HTTP] GET /gen_204 (Android alt)");
      request->redirect("http://10.5.5.5/");
    });

    // Apple/iOS Captive Portal Detection
    _server.on("/hotspot-detect.html", HTTP_GET,
               [](AsyncWebServerRequest *request) {
                 Serial.println("[HTTP] GET /hotspot-detect.html (iOS)");
                 request->redirect("http://10.5.5.5/");
               });

    _server.on("/library/test/success.html", HTTP_GET,
               [](AsyncWebServerRequest *request) {
                 Serial.println(
                     "[HTTP] GET /library/test/success.html (iOS alt)");
                 request->redirect("http://10.5.5.5/");
               });

    // Microsoft Windows Captive Portal
    _server.on("/connecttest.txt", HTTP_GET,
               [](AsyncWebServerRequest *request) {
                 Serial.println("[HTTP] GET /connecttest.txt (Windows)");
                 request->send(200, "text/plain", "Microsoft Connect Test");
               });

    _server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println("[HTTP] GET /ncsi.txt (Windows NCSI)");
      request->send(200, "text/plain", "Microsoft NCSI");
    });
  }

  _server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
    Serial.print("[HTTP] GET / from ");
    Serial.println(request->client()->remoteIP());

    // If we are not connected to WiFi (AP Mode), redirect to WiFi Config
    if (WiFi.status() != WL_CONNECTED) {
      request->redirect("/wifi");
      return;
    }

    String html = "<html><head><meta name='viewport' "
                  "content='width=device-width, initial-scale=1'>";
    html += getCommonStyle();
    html += "</head><body>";
    html += "<h1>Meter Clock Dashboard</h1>";
    html += "<h2 id='clock'>Loading time...</h2>";
    html += "<a href='/settings/time' class='btn'>Time Settings</a>";
    html += "<a href='/settings/led' class='btn'>LED Lighting</a>";
    html += "<a href='/calibration' class='btn'>Meter Calibration</a>";
    html += "<a href='/settings/system' class='btn'>Firmware</a>";
    html += "<a href='/wifi' class='btn'>WiFi Configuration</a>";
    html += "<script>setInterval(function() { fetch('/api/time').then(response "
            "=> response.text()).then(time => "
            "document.getElementById('clock').innerText = time).catch(err => "
            "console.error(err)); }, 1000);</script>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  _server.on("/wifi", HTTP_GET, [this](AsyncWebServerRequest *request) {
    String html = "<html><head><meta name='viewport' "
                  "content='width=device-width, initial-scale=1'>";
    html += getCommonStyle();
    html += "<script>";
    html += "function togglePassword() {";
    html += "  var x = document.getElementById('password');";
    html += "  var icon = document.getElementById('toggleIcon');";
    html += "  if (x.type === 'password') {";
    html += "    x.type = 'text';";
    html += "    icon.innerHTML = 'Hide';";
    html += "  } else {";
    html += "    x.type = 'password';";
    html += "    icon.innerHTML = 'Show';";
    html += "  }";
    html += "}";
    html += "</script>";
    html += "</head><body>";
    html += "<h1>WiFi Configuration</h1>";
    html += getWiFiScanHTML();
    html += "<h3>Connect to Network</h3>";
    html += "<form action='/save_wifi' method='POST'>";
    html += "<input type='text' name='ssid' placeholder='Network Name (SSID)' "
            "required><br>";
    html += "<div class='password-container'>";
    html += "<input type='password' id='password' name='pass' "
            "placeholder='Password (leave blank if none)'>";
    html += "<span class='toggle-password' id='toggleIcon' "
            "onclick='togglePassword()'>Show</span>";
    html += "</div><br>";
    html += "<input type='submit' value='Save & Connect'>";
    html += "</form>";
    html += "<h3>Clear Saved WiFi</h3>";
    html += "<form action='/clear_wifi' method='POST'>";
    html +=
        "<input type='submit' value='Clear WiFi Credentials' class='danger'>";
    html += "</form>";
    html += "<a href='/'>&larr; Back to Dashboard</a>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  _server.on("/save_wifi", HTTP_POST, [this](AsyncWebServerRequest *request) {
    String ssid = request->arg("ssid");
    String pass = request->arg("pass");
    if (ssid.length() > 0) {
      Serial.println("[WiFi] Saving credentials: " + ssid);
      _config.saveWiFi(ssid, pass);

      // Send response with auto-refresh to status page
      String html = "<html><head><meta name='viewport' "
                    "content='width=device-width, initial-scale=1'>";
      html += "<meta http-equiv='refresh' content='2;url=/wifi_status' />";
      html += "<meta http-equiv='refresh' content='2;url=/wifi_status' />";
      html += getCommonStyle();
      html += "</head><body>";
      html += "<h2>WiFi Saved!</h2>";
      html += "<p>Connecting to <strong>" + ssid + "</strong>...</p>";
      html += "<p>Please wait...</p>";
      html += "</body></html>";
      request->send(200, "text/html", html);

      // Switch to AP_STA mode and try to connect (keeps AP running)
      Serial.println("[WiFi] Switching to AP_STA mode to test connection...");
      WiFi.mode(WIFI_AP_STA);
      WiFi.begin(ssid.c_str(), pass.c_str());
    } else {
      request->send(400, "text/html",
                    "<h2>Error: SSID required</h2><a href='/wifi'>Back</a>");
    }
  });

  _server.on("/wifi_status", HTTP_GET, [this](AsyncWebServerRequest *request) {
    String html = "<html><head><meta name='viewport' "
                  "content='width=device-width, initial-scale=1'>";
    html +=
        "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += getCommonStyle();
    html += "</head><body>";

    if (WiFi.status() == WL_CONNECTED) {
      html += "<h2 class='success'>Connected!</h2>";
      html += "<p>Successfully connected to <strong>" + _config.getSSID() +
              "</strong></p>";
      html += "<p>IP Address: <strong>" + WiFi.localIP().toString() +
              "</strong></p>";
      html += "<p>The device will restart in 5 seconds.</p>";
      html += "<p>After restart, reconnect your phone/tablet to <strong>" +
              _config.getSSID() + "</strong></p>";
      html += "<p>Then access the clock at: <strong>http://" +
              WiFi.localIP().toString() + "</strong></p>";
      html += "<meta http-equiv='refresh' content='5;url=http://" +
              WiFi.localIP().toString() + "/' />";
      html += "</body></html>";
      request->send(200, "text/html", html);

      delay(100);
      Serial.println("[WiFi] Connection successful! Restarting...");
      ESP.restart();
    } else {
      html += "<h2 class='error'>Connection Failed</h2>";
      html += "<p>Could not connect to <strong>" + _config.getSSID() +
              "</strong></p>";
      html += "<p>Please check your password and try again.</p>";
      html += "<a href='/clear_wifi' class='btn btn-danger'>Clear WiFi</a>";
      html += "<a href='/wifi' class='btn'>Try Again</a>";
      html += "</body></html>";
      request->send(200, "text/html", html);

      // Go back to AP-only mode
      WiFi.mode(WIFI_AP);
    }
  });

  _server.on("/clear_wifi", HTTP_POST, [this](AsyncWebServerRequest *request) {
    Serial.println("[WiFi] Clearing saved credentials");
    _config.saveWiFi("", "");

    String html = "<html><head><meta name='viewport' "
                  "content='width=device-width, initial-scale=1'>";
    html +=
        "<meta http-equiv='refresh' content='3;url=http://10.5.5.5/wifi' />";
    html +=
        "<meta http-equiv='refresh' content='3;url=http://10.5.5.5/wifi' />";
    html += getCommonStyle();
    html += "</head><body>";
    html += "<h2>✓ WiFi Cleared!</h2>";
    html += "<p>Saved credentials have been deleted.</p>";
    html += "<p>Device will restart in 3 seconds.</p>";
    html += "</body></html>";
    request->send(200, "text/html", html);

    delay(100); // Let response send
    Serial.println("[WiFi] Restarting...");
    ESP.restart();
  });

  // =================================================================================
  //  TIME & DISPLAY SETTINGS
  // =================================================================================
  _server.on(
      "/settings/time", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String tz = _config.getTimezone();
        String tz2 = _config.getTimezone2();
        String ntp = _config.getNTP();
        bool h12 = _config.get12H();
        bool useNTP = _config.getUseNTP();

        String html = "<html><head><meta name='viewport' "
                      "content='width=device-width, initial-scale=1'>";
        html += getCommonStyle();
        html += "<script>";
        html += "function toggleTimeSource() {";
        html += "  var useNTP = "
                "document.querySelector('input[name=\"useNTP\"]:checked')."
                "value === '1';";
        html += "  document.getElementById('manualTimeDiv').style.display = "
                "useNTP ? 'none' : 'block';";
        html += "}";
        html += "function syncBrowserTime() {";
        html += "  var now = new Date();";
        html += "  var offset = now.getTimezoneOffset() * 60000;";
        html += "  var localIso = new Date(now - "
                "offset).toISOString().slice(0, 16);";
        html += "  document.getElementById('manualTime').value = localIso;";
        html += "}";
        html += "function saveTime(event) {";
        html += "  event.preventDefault();";
        html += "  fetch('/save_time', { method: 'POST', body: new "
                "FormData(event.target) })";
        html += "    .then(r => { if(r.ok) alert('Time Settings Saved!'); else "
                "alert('Error!'); });";
        html += "  return false;";
        html += "}";
        html += "</script></head><body>";

        html += "<h1>Time Settings</h1>";
        html += "<form onsubmit='return saveTime(event)'>";

        // Timezone 1
        html += "<label>Primary Timezone:</label><select name='timezone'>";
        struct TZOption {
          const char *name;
          const char *posix;
        };
        TZOption timezones[] = {{"UTC", "UTC0"},
                                {"Eastern", "EST5EDT,M3.2.0,M11.1.0"},
                                {"Central", "CST6CDT,M3.2.0,M11.1.0"},
                                {"Mountain", "MST7MDT,M3.2.0,M11.1.0"},
                                {"Arizona", "MST7"},
                                {"Pacific", "PST8PDT,M3.2.0,M11.1.0"},
                                {"Alaska", "AKST9AKDT,M3.2.0,M11.1.0"},
                                {"Hawaii", "HST10"},
                                {"London", "GMT0BST,M3.5.0/1,M10.5.0"},
                                {"Paris", "CET-1CEST,M3.5.0,M10.5.0/3"},
                                {"Tokyo", "JST-9"},
                                {"Sydney", "AEST-10AEDT,M10.1.0,M4.1.0/3"}};
        for (auto &t : timezones) {
          html += "<option value='" + String(t.posix) + "'" +
                  (tz == t.posix ? " selected" : "") + ">" + t.name +
                  "</option>";
        }
        html += "</select>";

        // Timezone 2
        html += "<label>Secondary Timezone (GPIO Switch):</label><select "
                "name='timezone2'>";
        for (auto &t : timezones) {
          html += "<option value='" + String(t.posix) + "'" +
                  (tz2 == t.posix ? " selected" : "") + ">" + t.name +
                  "</option>";
        }
        html += "</select>";

        // NTP Server
        html +=
            "<label>NTP Server:</label><input type='text' name='ntp' value='" +
            ntp + "'>";

        // Time Source
        html += "<label>Time Source:</label><div class='radio-group'>";
        html += "<label><input type='radio' name='useNTP' value='1'" +
                String(useNTP ? " checked" : "") +
                " onchange='toggleTimeSource()'> Automatic (NTP)</label>";
        html += "<label><input type='radio' name='useNTP' value='0'" +
                String(!useNTP ? " checked" : "") +
                " onchange='toggleTimeSource()'> Manual</label>";
        html += "</div>";

        // Manual Time
        html += "<div id='manualTimeDiv' style='display:" +
                String(useNTP ? "none" : "block") + "'>";
        html += "<label>Set Date & Time:</label><input type='datetime-local' "
                "id='manualTime' name='manualTime'>";
        html += "<button type='button' onclick='syncBrowserTime()' "
                "style='margin-top:5px;'>Sync Browser Time</button></div>";

        // Hour Format
        html += "<label>Hour Format:</label><div class='radio-group'>";
        html += "<label><input type='radio' name='h12' value='0'" +
                String(!h12 ? " checked" : "") + "> 24-Hour</label>";
        html += "<label><input type='radio' name='h12' value='1'" +
                String(h12 ? " checked" : "") + "> 12-Hour</label>";
        html += "</div>";

        html += "<input type='submit' value='Save Time Settings'>";
        html += "</form>";
        html += "<a href='/'>&larr; Back to Dashboard</a></body></html>";
        request->send(200, "text/html", html);
      });

  _server.on("/save_time", HTTP_POST, [this](AsyncWebServerRequest *request) {
    if (request->hasArg("timezone"))
      _config.saveTimezone(request->arg("timezone"));
    if (request->hasArg("timezone2"))
      _config.saveTimezone2(request->arg("timezone2"));
    if (request->hasArg("ntp"))
      _config.saveNTP(request->arg("ntp"));
    if (request->hasArg("h12"))
      _config.save12H(request->arg("h12") == "1");

    if (request->hasArg("useNTP")) {
      bool use = request->arg("useNTP") == "1";
      _config.saveUseNTP(use);
      timeManager.setUseNTP(use);
    }

    if (request->hasArg("manualTime") &&
        request->arg("manualTime").length() > 0) {
      String dtStr = request->arg("manualTime");
      struct tm timeinfo;
      int Y, M, D, h, m;
      if (sscanf(dtStr.c_str(), "%d-%d-%dT%d:%d", &Y, &M, &D, &h, &m) == 5) {
        timeinfo.tm_year = Y - 1900;
        timeinfo.tm_mon = M - 1;
        timeinfo.tm_mday = D;
        timeinfo.tm_hour = h;
        timeinfo.tm_min = m;
        timeinfo.tm_sec = 0;
        time_t ts = mktime(&timeinfo);
        _config.saveManualTime(ts);
        if (!_config.getUseNTP()) {
          struct timeval tv = {ts, 0};
          settimeofday(&tv, NULL);
        }
      }
    }
    request->send(200, "text/plain", "OK");
  });

  // =================================================================================
  //  LED SETTINGS
  // =================================================================================
  _server.on("/settings/led", HTTP_GET, [this](AsyncWebServerRequest *request) {
    uint32_t dayColor = _config.getDayColor();
    uint32_t nightColor = _config.getNightColor();
    uint8_t dayBright = _config.getDayBrightness();
    uint8_t nightBright = _config.getNightBrightness();
    uint8_t nightStart = _config.getNightStart();
    uint8_t nightEnd = _config.getNightEnd();

    String html = "<html><head><meta name='viewport' "
                  "content='width=device-width, initial-scale=1'>";
    html += getCommonStyle();
    html += "<script>";
    html += "function updateSlider(id, valId) {";
    html += "  var val = document.getElementById(id).value;";
    html += "  document.getElementById(valId).innerText = Math.round(val * 100 "
            "/ 255) + '%';";
    html += "}";
    html += "function saveLed(event) {";
    html += "  event.preventDefault();";
    html += "  fetch('/save_led', { method: 'POST', body: new "
            "FormData(event.target) })";
    html += "    .then(r => { if(r.ok) alert('LED Settings Saved!'); else "
            "alert('Error!'); });";
    html += "  return false;";
    html += "}";
    html += "</script></head><body>";

    html += "<h1>LED Lighting</h1>";
    html += "<form onsubmit='return saveLed(event)'>";

    // Day
    char buf[10];
    sprintf(buf, "#%06X", dayColor);
    html +=
        "<label>Day Color:</label><input type='color' name='dayColor' value='" +
        String(buf) + "'>";

    html += "<label>Day Brightness:</label><div class='slider-container'>";
    html += "<input type='range' id='db' name='dayBright' min='0' max='255' "
            "value='" +
            String(dayBright) + "' oninput='updateSlider(\"db\", \"dbv\")'>";
    html += "<span class='slider-value' id='dbv'>" +
            String(dayBright * 100 / 255) + "%</span></div>";

    // Night
    sprintf(buf, "#%06X", nightColor);
    html += "<label>Night Color:</label><input type='color' name='nightColor' "
            "value='" +
            String(buf) + "'>";

    html += "<label>Night Brightness:</label><div class='slider-container'>";
    html += "<input type='range' id='nb' name='nightBright' min='0' max='255' "
            "value='" +
            String(nightBright) + "' oninput='updateSlider(\"nb\", \"nbv\")'>";
    html += "<span class='slider-value' id='nbv'>" +
            String(nightBright * 100 / 255) +
            "%</span></div>"; // Fixed % calculation

    // Schedule
    char tBuf[6];
    sprintf(tBuf, "%02d:%02d", nightStart, _config.getNightStartMinute());
    html += "<label>Night Start (CST):</label><input type='time' "
            "name='nightStart' value='" +
            String(tBuf) + "'>";

    sprintf(tBuf, "%02d:%02d", nightEnd, _config.getNightEndMinute());
    html += "<label>Night End (CST):</label><input type='time' name='nightEnd' "
            "value='" +
            String(tBuf) + "'>";

    html += "<div class='info'>Night mode activates between these times.</div>";
    html += "<input type='submit' value='Save LED Settings'>";
    html += "</form>";
    html += "<a href='/'>&larr; Back to Dashboard</a></body></html>";
    request->send(200, "text/html", html);
  });

  _server.on("/save_led", HTTP_POST, [this](AsyncWebServerRequest *request) {
    if (request->hasArg("dayColor")) {
      String c = request->arg("dayColor");
      c.replace("#", "");
      _config.saveDayColor(strtoul(c.c_str(), NULL, 16));
    }
    if (request->hasArg("nightColor")) {
      String c = request->arg("nightColor");
      c.replace("#", "");
      _config.saveNightColor(strtoul(c.c_str(), NULL, 16));
    }
    if (request->hasArg("dayBright"))
      _config.saveDayBrightness(request->arg("dayBright").toInt());
    if (request->hasArg("nightBright"))
      _config.saveNightBrightness(request->arg("nightBright").toInt());

    if (request->hasArg("nightStart")) {
      String t = request->arg("nightStart");
      int sep = t.indexOf(':');
      if (sep > 0) {
        _config.saveNightStart(t.substring(0, sep).toInt());
        _config.saveNightStartMinute(t.substring(sep + 1).toInt());
      }
    }
    if (request->hasArg("nightEnd")) {
      String t = request->arg("nightEnd");
      int sep = t.indexOf(':');
      if (sep > 0) {
        _config.saveNightEnd(t.substring(0, sep).toInt());
        _config.saveNightEndMinute(t.substring(sep + 1).toInt());
      }
    }
    request->send(200, "text/plain", "OK");
  });

  // =================================================================================
  //  SYSTEM SETTINGS
  // =================================================================================
  _server.on(
      "/settings/system", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String html = "<html><head><meta name='viewport' "
                      "content='width=device-width, initial-scale=1'>";
        html += getCommonStyle();
        html += "<script>";
        html += "function uploadFw() {";
        html += "  var file = document.getElementById('fwFile').files[0];";
        html += "  if(!file) { alert('Select a file!'); return; }";
        html += "  var fd = new FormData(); fd.append('update', file);";
        html += "  var xhr = new XMLHttpRequest();";
        html += "  xhr.upload.addEventListener('progress', function(e) {";
        html += "    if(e.lengthComputable) {";
        html += "      var p = Math.round((e.loaded/e.total)*100);";
        html +=
            "      document.getElementById('progress').style.display='block';";
        html += "      document.getElementById('pct').innerText = p;";
        html += "      if(p >= 100) {";
        html += "        document.getElementById('progress').innerHTML "
                "= '<span style=\"color:green\">Success! Rebooting...</span>';";
        html += "        setTimeout(function(){ window.location.href='/'; }, "
                "5000);";
        html += "      }";
        html += "    }";
        html += "  }, false);";
        html += "  xhr.open('POST', '/update'); xhr.send(fd);";
        html += "}";
        html += "</script></head><body>";

        html += "<h1>Firmware</h1>";
        html += "<div class='card'><h3>Firmware Update</h3>";
        html += "<input type='file' id='fwFile' accept='.bin'>";
        html += "<button onclick='uploadFw()' style='background-color:#f44336; "
                "margin-top:10px;'>Upload Firmware</button>";
        html +=
            "<div id='progress' style='display:none; margin-top:15px; "
            "font-weight:bold;'>Uploading... <span id='pct'>0</span>%</div>";
        html += "</div>";

        html += "<a href='/'>&larr; Back to Dashboard</a></body></html>";
        request->send(200, "text/html", html);
      });

  _server.on("/calibration", HTTP_GET, [this](AsyncWebServerRequest *request) {
    String html = "<html><head><meta name='viewport' "
                  "content='width=device-width, initial-scale=1'>";
    html += getCommonStyle();
    html += "<script>";
    html += "function saveCal(event) {";
    html += "  event.preventDefault();";
    html += "  var form = event.target;";
    html += "  var data = new URLSearchParams(new FormData(form));";
    // Also send to save endpoint
    html += "  fetch(form.action, { method: form.method, body: data })";
    html += "    .then(r => {";
    html += "      if(r.ok) {";
    html += "        alert('Saved!');";
    html += "        var cb = document.getElementById('calMode');";
    html += "        if(cb.checked) { cb.checked = false; toggleCalMode(cb); }";
    html += "      } else { alert('Error: ' + r.statusText); }";
    html += "    })";
    html += "    .catch(e => alert('Error: ' + e));";
    html += "  return false;";
    html += "}";

    html += "function toggleCalMode(cb) {";
    html += "  var fd = new FormData();";
    html += "  fd.append('active', cb.checked ? '1' : '0');";
    html += "  fetch('/api/calibration/mode', { method: 'POST', body: fd });";
    html += "  document.getElementById('calControls').style.opacity = "
            "cb.checked ? '1' : '0.5';";
    html += "}";

    html += "function previewCal(idx, val) {";
    html += "  if(!document.getElementById('calMode').checked) return;";
    html += "  var fd = new FormData();";
    html += "  fd.append('idx', idx);";
    html += "  fd.append('val', val);";
    html +=
        "  fetch('/api/calibration/preview', { method: 'POST', body: fd });";
    html += "}";
    html += "</script>";
    html += "</head><body>";
    html += "<h1>Meter Calibration</h1>";

    html += "<div class='card'>";
    html += "<label class='switch'><input type='checkbox' id='calMode' "
            "onchange='toggleCalMode(this)' " +
            String(g_isCalibrationMode ? "checked" : "") +
            "> Enable Calibration Mode</label>";
    html += "<p class='small'>Enable this to stop the clock and set meters to "
            "exact values.</p>";
    html += "</div>";

    html += "<form action='/save_calibration' method='POST' id='calControls' "
            "onsubmit='return saveCal(event)'>";

    // Helper macro for inputs
    auto addInput = [&](String label, String name, int val, int idx) {
      String s = "<div class='col'><label>" + label + "</label>";
      s += "<input type='number' name='" + name + "' value='" + String(val) +
           "' ";
      s += "onfocus='previewCal(" + String(idx) + ", this.value)' ";
      s += "oninput='previewCal(" + String(idx) + ", this.value)'>";
      s += "</div>";
      return s;
    };

    html += "<h3>Hour Meter</h3><div class='row'>";
    html += addInput("Min (0)", "calHMin", _config.getCalHMin(), 0);
    html += addInput("Max (12/24)", "calHMax", _config.getCalHMax(), 0);
    html += "</div>";

    html += "<h3>Minute Meter</h3><div class='row'>";
    html += addInput("Min (0)", "calMMin", _config.getCalMMin(), 1);
    html += addInput("Max (60)", "calMMax", _config.getCalMMax(), 1);
    html += "</div>";

    html += "<h3>Second Meter</h3><div class='row'>";
    html += addInput("Min (0)", "calSMin", _config.getCalSMin(), 2);
    html += addInput("Max (60)", "calSMax", _config.getCalSMax(), 2);
    html += "</div>";

    html += "<br><input type='submit' value='Save Calibration'>";
    html += "</form>";
    html += "<a href='/'>&larr; Back to Dashboard</a>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  _server.on("/save_calibration", HTTP_POST,
             [this](AsyncWebServerRequest *request) {
               if (request->hasArg("calHMin"))
                 _config.saveCalHMin(request->arg("calHMin").toInt());
               if (request->hasArg("calHMax"))
                 _config.saveCalHMax(request->arg("calHMax").toInt());

               if (request->hasArg("calMMin"))
                 _config.saveCalMMin(request->arg("calMMin").toInt());
               if (request->hasArg("calMMax"))
                 _config.saveCalMMax(request->arg("calMMax").toInt());

               if (request->hasArg("calSMin"))
                 _config.saveCalSMin(request->arg("calSMin").toInt());
               if (request->hasArg("calSMax"))
                 _config.saveCalSMax(request->arg("calSMax").toInt());

               Serial.println("Calibration Saved"); // Debug
               request->send(200, "text/plain", "OK");
             });

  // API: Calibration Mode Toggle
  _server.on(
      "/api/calibration/mode", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasArg("active")) {
          bool active = (request->arg("active") == "1");
          g_isCalibrationMode = active;
          Serial.print("Calibration Mode Toggled: ");
          Serial.println(active ? "ON" : "OFF");

          // Reset overrides when mode changes
          g_calOverrideValues[0] = -1;
          g_calOverrideValues[1] = -1;
          g_calOverrideValues[2] = -1;
        }
        request->send(200, "text/plain", g_isCalibrationMode ? "1" : "0");
      });

  // API: Calibration Preview
  _server.on("/api/calibration/preview", HTTP_POST,
             [](AsyncWebServerRequest *request) {
               if (g_isCalibrationMode && request->hasArg("idx") &&
                   request->hasArg("val")) {
                 int idx = request->arg("idx").toInt();
                 int val = request->arg("val").toInt();
                 if (idx >= 0 && idx <= 2) {
                   g_calOverrideValues[idx] = val;
                 }
               }
               request->send(200, "text/plain", "OK");
             });

  // OTA Update Handler
  _server.on(
      "/update", HTTP_POST,
      [](AsyncWebServerRequest *request) {
        bool shouldReboot = !Update.hasError();
        AsyncWebServerResponse *response =
            request->beginResponse(200, "text/plain", "OK");
        response->addHeader("Connection", "close");
        request->send(response);
        if (shouldReboot) {
          delay(2000); // Increased to 2s to ensure client gets 200 OK
          ESP.restart();
        }
      },
      [](AsyncWebServerRequest *request, String filename, size_t index,
         uint8_t *data, size_t len, bool final) {
        if (!index) {
          Serial.printf("Update Start: %s\r\n", filename.c_str());
          // Update.runAsync(true); // Don't use async, we need blocking
          // write? No, AsyncWebServer is async.
          if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
            Update.printError(Serial);
          }
        }
        if (!Update.hasError()) {
          if (Update.write(data, len) != len) {
            Update.printError(Serial);
          }
        }
        if (final) {
          if (Update.end(true)) {
            Serial.printf("Update Success: %uB\r\n", index + len);
          } else {
            Update.printError(Serial);
          }
        }
      });

  // Current Time API Endpoint
  _server.on("/api/time", HTTP_GET, [](AsyncWebServerRequest *request) {
    extern TimeManager timeManager;
    request->send(200, "text/plain", timeManager.getFormattedTime());
  });

  _server.on("/test_save", HTTP_GET, [this](AsyncWebServerRequest *request) {
    _config.saveNightStart(22);
    _config.saveNightStartMinute(15);
    String res = "Saved 22:15. Read back: ";
    res += String(_config.getNightStart()) + ":" +
           String(_config.getNightStartMinute());
    request->send(200, "text/plain", res);
  });
}

void NetworkManager::loop() {
  if (_isAP) {
    _dnsServer.processNextRequest();
  }
}

String NetworkManager::getCommonStyle() {
  String css = "<style>";
  css += "* { box-sizing: border-box; }";
  css += "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; "
         "background-color: #121212; color: #e0e0e0; max-width: 600px; margin: "
         "0 auto; padding: 20px; font-size: 16px; line-height: 1.6; }";
  css += "h1, h2, h3 { color: #ffffff; text-align: center; }";
  css += "h3 { border-bottom: 2px solid #07f67a8d; padding-bottom: 10px; "
         "margin-top: 30px; }";
  css += "a { color: #07f67a8d; text-decoration: none; }";
  css += "a:hover { text-decoration: underline; }";
  css += "input[type='text'], input[type='password'], input[type='number'], "
         "input[type='color'], input[type='datetime-local'], select { "
         "width: 100%; padding: 12px; margin: 8px 0; box-sizing: border-box; "
         "font-size: 16px; background-color: #2d2d2d; color: #fff; "
         "border: 1px solid #555; border-radius: 4px; }";
  css += "input[type='color'] { height: 50px; padding: 2px; }";
  css += "input[type='submit'], button, .btn { background-color: #07f67a8d; "
         "color: #121212; font-weight: bold; padding: 14px 20px; margin: 8px "
         "0; border: none; cursor: pointer; width: 100%; font-size: 16px; "
         "border-radius: 4px; display: block; text-align: center; }";
  css += "input[type='submit']:hover, button:hover, .btn:hover { "
         "background-color: #07f67ad8; text-decoration: none; }";
  css += ".btn-danger, .danger { background-color: #ef5350; color: white; }";
  css += ".btn-danger:hover, .danger:hover { background-color: #d32f2f; }";
  css += ".card { background-color: #1e1e1e; padding: 20px; border-radius: "
         "8px; box-shadow: 0 4px 6px rgba(0,0,0,0.3); margin-bottom: 20px; }";
  css += "label { display: block; margin-top: 15px; font-weight: bold; "
         "color: #b0bec5; }";
  css +=
      ".info { background-color: #263238; padding: 15px; border-left: 4px "
      "solid #07f67a8d; margin: 15px 0; font-size: 14px; border-radius: 0 4px "
      "4px 0; }";
  css += ".success { color: #66bb6a; } .error { color: #ef5350; }"; // Green/Red
  css += ".slider-container { display: flex; align-items: center; gap: 10px; }";
  css += ".slider-container input[type='range'] { flex: 1; accent-color: "
         "#07f67a8d; }";
  css += ".slider-value { min-width: 50px; text-align: right; font-weight: "
         "bold; color: #07f67a8d; }";
  css += ".radio-group { margin: 10px 0; display: flex; gap: 20px; }";
  css += ".radio-group label { margin: 0; font-weight: normal; cursor: "
         "pointer; display: flex; align-items: center; gap: 5px; color: "
         "#e0e0e0; }";
  css += "input[type='radio'] { accent-color: #07f67a8d; width: 20px; height: "
         "20px; }";
  css += "</style>";
  return css;
}
