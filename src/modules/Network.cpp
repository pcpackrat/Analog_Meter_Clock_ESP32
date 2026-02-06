#include "Network.h"
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
    html += "<a href='/wifi' class='btn'>WiFi Configuration</a>";
    html += "<a href='/settings' class='btn'>Settings</a>";
    html += "<a href='/calibration' class='btn'>Meter Calibration</a>";
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

  _server.on("/settings", HTTP_GET, [this](AsyncWebServerRequest *request) {
    String tz = _config.getTimezone();
    String tz2 = _config.getTimezone2();
    String ntp = _config.getNTP();
    bool h12 = _config.get12H();

    // LED settings
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
    html += "function updateSlider(sliderId, valueId) {";
    html += "  document.getElementById(valueId).textContent = "
            "document.getElementById(sliderId).value + '%';";
    html += "}";
    html += "function toggleTimeSource() {";
    html += "  var useNTP = "
            "document.querySelector('input[name=\"useNTP\"]:checked').value "
            "=== '1';";
    html += "  document.getElementById('manualTimeDiv').style.display = useNTP "
            "? 'none' : 'block';";
    html += "}";
    html += "function showToast(message) {";
    html += "  var toast = document.createElement('div');";
    html += "  toast.style.position = 'fixed';";
    html += "  toast.style.bottom = '20px';";
    html += "  toast.style.left = '50%';";
    html += "  toast.style.transform = 'translateX(-50%)';";
    html += "  toast.style.backgroundColor = '#333';";
    html += "  toast.style.color = 'white';";
    html += "  toast.style.padding = '12px 24px';";
    html += "  toast.style.borderRadius = '4px';";
    html += "  toast.style.zIndex = '1000';";
    html += "  toast.textContent = message;";
    html += "  document.body.appendChild(toast);";
    html +=
        "  setTimeout(function(){ document.body.removeChild(toast); }, 3000);";
    html += "}";
    html += "function saveSettings(event) {";
    html += "  event.preventDefault();";
    html += "  var form = event.target;";
    html += "  var data = new FormData(form);";
    html += "  fetch(form.action, {";
    html += "    method: form.method,";
    html += "    body: data";
    html += "  }).then(response => {";
    html += "    if (response.ok) {";
    html += "      showToast('Settings Saved Successfully!');";
    html += "    } else {";
    html += "      showToast('Error Saving Settings');";
    html += "    }";
    html += "  }).catch(error => {";
    html += "    showToast('Connection Error');";
    html += "  });";
    html += "  return false;";
    html += "}";
    html += "</script>";
    html += "</head><body>";
    html += "<h1>Settings</h1>";
    html += "<form action='/save_settings' method='POST' onsubmit='return "
            "saveSettings(event)'>";

    // Time & Display Section
    // Time & Display Section
    html += "<div class='card'>";
    html += "<h3>Time & Display</h3>";

    // Primary Timezone Dropdown
    html += "<label for='timezone'>Primary Timezone:</label>";
    html += "<select name='timezone' id='timezone'>";

    struct TZOption {
      const char *name;
      const char *posix;
    };
    TZOption timezones[] = {
        {"UTC (Coordinated Universal Time)", "UTC0"},
        {"Eastern Time (New York)", "EST5EDT,M3.2.0,M11.1.0"},
        {"Central Time (Chicago)", "CST6CDT,M3.2.0,M11.1.0"},
        {"Mountain Time (Denver)", "MST7MDT,M3.2.0,M11.1.0"},
        {"Arizona (No Daylight Saving)", "MST7"},
        {"Pacific Time (Los Angeles)", "PST8PDT,M3.2.0,M11.1.0"},
        {"Alaska Time (Anchorage)", "AKST9AKDT,M3.2.0,M11.1.0"},
        {"Hawaii Time (Honolulu)", "HST10"},
        {"GMT (London)", "GMT0BST,M3.5.0/1,M10.5.0"},
        {"Central European (Paris)", "CET-1CEST,M3.5.0,M10.5.0/3"},
        {"Japan (Tokyo)", "JST-9"},
        {"Australia Eastern (Sydney)", "AEST-10AEDT,M10.1.0,M4.1.0/3"}};

    for (int i = 0; i < 12; i++) {
      html += "<option value='" + String(timezones[i].posix) + "'";
      if (tz == String(timezones[i].posix))
        html += " selected";
      html += ">" + String(timezones[i].name) + "</option>";
    }
    html += "</select>";

    // Secondary Timezone Dropdown
    html += "<label for='timezone2'>Secondary Timezone:</label>";
    html += "<select name='timezone2' id='timezone2'>";
    for (int i = 0; i < 12; i++) {
      html += "<option value='" + String(timezones[i].posix) + "'";
      if (tz2 == String(timezones[i].posix))
        html += " selected";
      html += ">" + String(timezones[i].name) + "</option>";
    }
    html += "</select>";
    html += "<div class='info'>Secondary timezone can be displayed on a second "
            "meter or used for reference. GPIO switch: OPEN=primary, "
            "CLOSED=secondary.</div>";

    // NTP Server
    html += "<label for='ntp'>NTP Server:</label>";
    html += "<input type='text' name='ntp' id='ntp' value='" + ntp +
            "' placeholder='pool.ntp.org'>";

    // Time Source Selection
    bool useNTP = _config.getUseNTP();
    html += "<label>Time Source:</label>";
    html += "<div class='radio-group'>";
    html += "<label><input type='radio' name='useNTP' value='1'" +
            String(useNTP ? " checked" : "") +
            " onchange='toggleTimeSource()'> Use NTP (Automatic)</label>";
    html += "<label><input type='radio' name='useNTP' value='0'" +
            String(!useNTP ? " checked" : "") +
            " onchange='toggleTimeSource()'> Manual Time</label>";
    html += "</div>";

    // Manual Time Input (hidden by default if NTP is selected)
    html += "<div id='manualTimeDiv' style='display:" +
            String(useNTP ? "none" : "block") + "'>";
    html += "<label for='manualTime'>Set Date & Time:</label>";
    html += "<input type='datetime-local' name='manualTime' id='manualTime'>";
    html += "<button type='button' onclick='syncBrowserTime()' "
            "style='background-color: #2196F3; margin-top: 5px;'>Sync with "
            "Browser Time</button>";
    html +=
        "<div class='info'>Enter the current date and time. This will be used "
        "instead of NTP synchronization.</div>";
    html += "</div>";

    // Hour Format
    html += "<label>Hour Format:</label>";
    html += "<div class='radio-group'>";
    html += "<label><input type='radio' name='h12' value='0'" +
            String(!h12 ? " checked" : "") + "> 24-Hour</label>";
    html += "<label><input type='radio' name='h12' value='1'" +
            String(h12 ? " checked" : "") + "> 12-Hour</label>";
    html += "</div>";

    // LED Settings Section
    // LED Settings Section
    html += "</div><div class='card'>";
    html += "<h3>LED Lighting</h3>";

    // Day Mode
    html += "<label for='dayColor'>Day Color:</label>";
    char dayColorHex[8];
    sprintf(dayColorHex, "#%06X", dayColor);
    html += "<input type='color' name='dayColor' id='dayColor' value='" +
            String(dayColorHex) + "'>";

    html += "<label for='dayBright'>Day Brightness:</label>";
    html += "<div class='slider-container'>";
    html += "<input type='range' name='dayBright' id='dayBright' min='0' "
            "max='255' value='" +
            String(dayBright) +
            "' oninput='updateSlider(\"dayBright\", \"dayBrightVal\")'>";
    html += "<span class='slider-value' id='dayBrightVal'>" +
            String(dayBright * 100 / 255) + "%</span>";
    html += "</div>";

    // Night Mode
    html += "<label for='nightColor'>Night Color:</label>";
    char nightColorHex[8];
    sprintf(nightColorHex, "#%06X", nightColor);
    html += "<input type='color' name='nightColor' id='nightColor' value='" +
            String(nightColorHex) + "'>";

    html += "<label for='nightBright'>Night Brightness:</label>";
    html += "<div class='slider-container'>";
    html += "<input type='range' name='nightBright' id='nightBright' min='0' "
            "max='255' value='" +
            String(nightBright) +
            "' oninput='updateSlider(\"nightBright\", \"nightBrightVal\")'>";
    html += "<span class='slider-value' id='nightBrightVal'>" +
            String(nightBright * 100 / 255) + "%</span>";
    html += "</div>";

    // Night Mode Times
    html += "<label for='nightStart'>Night Mode Start:</label>";
    char nsBuf[6];
    sprintf(nsBuf, "%02d:%02d", nightStart, _config.getNightStartMinute());
    html += "<input type='time' name='nightStart' id='nightStart' value='" +
            String(nsBuf) + "'>";

    html += "<label for='nightEnd'>Night Mode End:</label>";
    char neBuf[6];
    sprintf(neBuf, "%02d:%02d", nightEnd, _config.getNightEndMinute());
    html += "<input type='time' name='nightEnd' id='nightEnd' value='" +
            String(neBuf) + "'>";

    html += "<div class='info'>Night mode automatically switches LED color and "
            "brightness between the specified hours (24-hour format).</div>";

    html += "<input type='submit' value='Save Settings'>";
    html += "</form>";

    // Firmware Update (AJAX)
    // Firmware Update (AJAX)
    html += "</div><div class='card'>";
    html += "<h3 id='fwHeader'>Firmware Update</h3>";
    html += "<div id='fwContainer'>";
    html += "<form id='fwForm'>";
    html += "<input type='file' name='update' id='fwFile' accept='.bin'>";
    html +=
        "<button type='button' onclick='uploadFw()' style='background-color: "
        "#f44336; margin-top: 10px;'>Upload Firmware</button>";
    html += "</form>";
    html += "</div></div>";       // Close fwContainer and card
    html += "<div class='card'>"; // For progress
    html += "<div id='progress' style='display:none; margin-top:20px; "
            "font-weight:bold;'>Uploading... <span id='pct'>0</span>%</div>";
    html += "</div>";

    html += "<script>";
    html += "function uploadFw() {";
    html += "  var file = document.getElementById('fwFile').files[0];";
    html += "  if(!file) { showToast('Select a file!'); return; }";
    html += "  var fd = new FormData();";
    html += "  fd.append('update', file);";
    html += "  var xhr = new XMLHttpRequest();";
    html += "  xhr.upload.addEventListener('progress', function(e) {";
    html += "    if(e.lengthComputable) {";
    html += "      var p = Math.round((e.loaded/e.total)*100);";
    html += "      document.getElementById('progress').style.display='block';";
    html += "      document.getElementById('pct').innerText = p;";
    html +=
        "      document.getElementById('fwContainer').style.display='none';";
    html += "      if(p >= 100) {";
    html += "        document.getElementById('progress').innerHTML = '<span "
            "style=\"color:green\">Success! Rebooting...</span>';";
    html +=
        "        setTimeout(function(){ window.location.href='/'; }, 5000);";
    html += "      }";
    html += "    }";
    html += "  }, false);";
    // Ignore onload, we trust the 100% progress
    html += "  xhr.onload = function() {";
    html += "    if(this.status != 200) {";
    html += "      console.log('Upload finished but status not 200 (likely "
            "rebooted)');";
    html += "    }";
    html += "  };";
    html += "  xhr.open('POST', '/update');";
    html += "  xhr.send(fd);";
    html += "}";
    html += "</script>";

    html += "<a href='/'>&larr; Back to Dashboard</a>";
    html += "</body></html>";
    request->send(200, "text/html", html);
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

  _server.on(
      "/save_settings", HTTP_POST, [this](AsyncWebServerRequest *request) {
        // Save time & display settings
        if (request->hasArg("timezone"))
          _config.saveTimezone(request->arg("timezone"));
        if (request->hasArg("timezone2"))
          _config.saveTimezone2(request->arg("timezone2"));
        if (request->hasArg("ntp"))
          _config.saveNTP(request->arg("ntp"));
        if (request->hasArg("h12"))
          _config.save12H(request->arg("h12") == "1");

        // Save time source selection
        if (request->hasArg("useNTP")) {
          bool use = request->arg("useNTP") == "1";
          _config.saveUseNTP(use);
          timeManager.setUseNTP(use);
        }

        // Save manual time if provided
        if (request->hasArg("manualTime") &&
            request->arg("manualTime").length() > 0) {
          Serial.println("Manual Time Arg found: " +
                         request->arg("manualTime"));
          String dtStr = request->arg("manualTime");
          struct tm timeinfo;
          int year, month, day, hour, minute;
          if (sscanf(dtStr.c_str(), "%d-%d-%dT%d:%d", &year, &month, &day,
                     &hour, &minute) == 5) {
            timeinfo.tm_year = year - 1900;
            timeinfo.tm_mon = month - 1;
            timeinfo.tm_mday = day;
            timeinfo.tm_hour = hour;
            timeinfo.tm_min = minute;
            timeinfo.tm_sec = 0;
            time_t timestamp = mktime(&timeinfo);
            _config.saveManualTime(timestamp);

            // Apply immediately if using Manual Time
            if (!_config.getUseNTP()) {
              struct timeval tv = {timestamp, 0};
              settimeofday(&tv, NULL);
              Serial.println("Manual time applied immediately");
            }
          }
        }

        // Save LED settings
        if (request->hasArg("dayColor")) {
          String colorStr = request->arg("dayColor");
          colorStr.replace("#", "");
          uint32_t color = strtoul(colorStr.c_str(), NULL, 16);
          _config.saveDayColor(color);
        }

        if (request->hasArg("nightColor")) {
          String colorStr = request->arg("nightColor");
          colorStr.replace("#", "");
          uint32_t color = strtoul(colorStr.c_str(), NULL, 16);
          // Serial.printf("Saving Night Color: 0x%06X (from %s)\r\n", color,
          // request->arg("nightColor").c_str());
          _config.saveNightColor(color);
        }

        if (request->hasArg("dayBright"))
          _config.saveDayBrightness(request->arg("dayBright").toInt());

        if (request->hasArg("nightBright"))
          _config.saveNightBrightness(request->arg("nightBright").toInt());

        if (request->hasArg("nightStart")) {
          String tStr = request->arg("nightStart");
          int sep = tStr.indexOf(':');
          if (sep > 0) {
            int h = tStr.substring(0, sep).toInt();
            int m = tStr.substring(sep + 1).toInt();
            Serial.printf("Saving Night Start manually parsed: %d:%d\n", h, m);
            _config.saveNightStart(h);
            _config.saveNightStartMinute(m);
          } else {
            Serial.println("Failed to parse nightStart (no colon): " + tStr);
          }
        }

        if (request->hasArg("nightEnd")) {
          String tStr = request->arg("nightEnd");
          int sep = tStr.indexOf(':');
          if (sep > 0) {
            int h = tStr.substring(0, sep).toInt();
            int m = tStr.substring(sep + 1).toInt();
            Serial.printf("Saving Night End manually parsed: %d:%d\n", h, m);
            _config.saveNightEnd(h);
            _config.saveNightEndMinute(m);
          } else {
            Serial.println("Failed to parse nightEnd (no colon): " + tStr);
          }
        }

        request->send(200, "text/plain", "OK");
      });

  _server.on("/calibration", HTTP_GET, [this](AsyncWebServerRequest *request) {
    String html = "<html><head><meta name='viewport' "
                  "content='width=device-width, initial-scale=1'>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; max-width: 600px; margin: "
            "0 auto; padding: 20px; font-size: 16px; }";
    html += "h1 { text-align: center; color: #333; }";
    html += "h3 { color: #666; margin-top: 20px; border-bottom: 2px solid "
            "#2196F3; padding-bottom: 5px; }";
    html += "label { display: block; margin-top: 10px; font-weight: bold; }";
    html += "input[type='number'] { width: 100%; padding: 8px; margin: 5px 0; "
            "box-sizing: border-box; }";
    html += "input[type='submit'] { background-color: #4CAF50; color: white; "
            "padding: 14px 20px; margin: 20px 0; border: none; cursor: "
            "pointer; width: 100%; font-size: 16px; border-radius: 4px; }";
    html += "input[type='submit']:hover { background-color: #45a049; }";
    html += "a { display: block; text-align: center; margin-top: 20px; color: "
            "#2196F3; text-decoration: none; }";
    html += ".row { display: flex; gap: 10px; }";
    html += ".col { flex: 1; }";
    html += "</style>";
    html += "<script>";
    html += "function saveCal(event) {";
    html += "  event.preventDefault();";
    html += "  var form = event.target;";
    html += "  var data = new FormData(form);";
    html += "  fetch(form.action, { method: form.method, body: data })";
    html +=
        "    .then(r => { if(r.ok) alert('Saved!'); else alert('Error'); })";
    html += "    .catch(e => alert('Error'));";
    html += "  return false;";
    html += "}";
    html += "</script>";
    html += "</head><body>";
    html += "<h1>Meter Calibration</h1>";
    html += "<form action='/save_calibration' method='POST' onsubmit='return "
            "saveCal(event)'>";

    html += "<h3>Hour Meter</h3>";
    html += "<div class='row'>";
    html += "<div class='col'><label>Min (0-1023)</label><input type='number' "
            "name='calHMin' value='" +
            String(_config.getCalHMin()) + "'></div>";
    html += "<div class='col'><label>Max (0-1023)</label><input type='number' "
            "name='calHMax' value='" +
            String(_config.getCalHMax()) + "'></div>";
    html += "</div>";

    html += "<h3>Minute Meter</h3>";
    html += "<div class='row'>";
    html += "<div class='col'><label>Min (0-1023)</label><input type='number' "
            "name='calMMin' value='" +
            String(_config.getCalMMin()) + "'></div>";
    html += "<div class='col'><label>Max (0-1023)</label><input type='number' "
            "name='calMMax' value='" +
            String(_config.getCalMMax()) + "'></div>";
    html += "</div>";

    html += "<h3>Second Meter</h3>";
    html += "<div class='row'>";
    html += "<div class='col'><label>Min (0-1023)</label><input type='number' "
            "name='calSMin' value='" +
            String(_config.getCalSMin()) + "'></div>";
    html += "<div class='col'><label>Max (0-1023)</label><input type='number' "
            "name='calSMax' value='" +
            String(_config.getCalSMax()) + "'></div>";
    html += "</div>";

    html += "<input type='submit' value='Save Calibration'>";
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
}

void NetworkManager::loop() {
  if (_isAP) {
    _dnsServer.processNextRequest();
  }
}

String NetworkManager::getCommonStyle() {
  String css = "<style>";
  css += "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; "
         "background-color: #121212; color: #e0e0e0; max-width: 600px; margin: "
         "0 auto; padding: 20px; font-size: 16px; line-height: 1.6; }";
  css += "h1, h2, h3 { color: #ffffff; text-align: center; }";
  css += "h3 { border-bottom: 2px solid #29B6F6; padding-bottom: 10px; "
         "margin-top: 30px; }";
  css += "a { color: #29B6F6; text-decoration: none; }";
  css += "a:hover { text-decoration: underline; }";
  css += "input[type='text'], input[type='password'], input[type='number'], "
         "input[type='color'], input[type='datetime-local'], select { "
         "width: 100%; padding: 12px; margin: 8px 0; box-sizing: border-box; "
         "font-size: 16px; background-color: #2d2d2d; color: #fff; "
         "border: 1px solid #555; border-radius: 4px; }";
  css += "input[type='color'] { height: 50px; padding: 2px; }";
  css += "input[type='submit'], button, .btn { background-color: #29B6F6; "
         "color: #121212; font-weight: bold; padding: 14px 20px; margin: 8px "
         "0; border: none; cursor: pointer; width: 100%; font-size: 16px; "
         "border-radius: 4px; display: block; text-align: center; }";
  css += "input[type='submit']:hover, button:hover, .btn:hover { "
         "background-color: #039BE5; text-decoration: none; }";
  css += ".btn-danger, .danger { background-color: #ef5350; color: white; }";
  css += ".btn-danger:hover, .danger:hover { background-color: #d32f2f; }";
  css += ".card { background-color: #1e1e1e; padding: 20px; border-radius: "
         "8px; box-shadow: 0 4px 6px rgba(0,0,0,0.3); margin-bottom: 20px; }";
  css += "label { display: block; margin-top: 15px; font-weight: bold; "
         "color: #b0bec5; }";
  css += ".info { background-color: #263238; padding: 15px; border-left: 4px "
         "solid #29B6F6; margin: 15px 0; font-size: 14px; border-radius: 0 4px "
         "4px 0; }";
  css += ".success { color: #66bb6a; } .error { color: #ef5350; }"; // Green/Red
  css += ".slider-container { display: flex; align-items: center; gap: 10px; }";
  css += ".slider-container input[type='range'] { flex: 1; accent-color: "
         "#29B6F6; }";
  css += ".slider-value { min-width: 50px; text-align: right; font-weight: "
         "bold; color: #29B6F6; }";
  css += ".radio-group { margin: 10px 0; display: flex; gap: 20px; }";
  css += ".radio-group label { margin: 0; font-weight: normal; cursor: "
         "pointer; display: flex; align-items: center; gap: 5px; color: "
         "#e0e0e0; }";
  css += "input[type='radio'] { accent-color: #29B6F6; width: 20px; height: "
         "20px; }";
  css += "</style>";
  return css;
}
