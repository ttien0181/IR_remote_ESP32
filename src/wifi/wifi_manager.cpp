#include "wifi_manager.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

namespace {
  const char* AP_SSID = "ESP32_Config";
  const char* AP_PASS = "12345678";
}

void WiFiManager::begin(uint8_t resetPin, uint32_t holdMs) {
  _resetPin = resetPin;
  _holdMs = holdMs;
  pinMode(_resetPin, INPUT_PULLUP);
  Serial.print("[WIFI] Reset pin=");
  Serial.print(_resetPin);
  Serial.print(" holdMs=");
  Serial.println(_holdMs);
}

DeviceMode WiFiManager::decideMode(const DeviceConfig& cfg) const {
  if (cfg.wifiSsid.length() == 0) {
    Serial.println("[WIFI] Mode AP_CONFIG (no ssid)");
    return DeviceMode::AP_CONFIG;
  }
  if (cfg.controllerId.length() == 0) {
    Serial.println("[WIFI] Mode STA_LOGIN (no controller_id)");
    return DeviceMode::STA_LOGIN;
  }
  Serial.println("[WIFI] Mode RUN");
  return DeviceMode::RUN;
}

bool WiFiManager::connectSta(const DeviceConfig& cfg, uint32_t timeoutMs) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(cfg.wifiSsid.c_str(), cfg.wifiPass.c_str());

  Serial.print("[WIFI] Connecting to SSID ");
  Serial.print(cfg.wifiSsid);
  Serial.print(" timeoutMs=");
  Serial.println(timeoutMs);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeoutMs) {
    delay(250);
  }
  bool ok = WiFi.status() == WL_CONNECTED;
  Serial.print("[WIFI] Connect result=");
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok) {
    Serial.print("[WIFI] Status code=");
    Serial.println((int)WiFi.status());
  }
  return ok;
}

void WiFiManager::startApPortal(ConfigStore& store, DeviceConfig& cfg) {
  _portalRunning = true;

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);

  Serial.print("[PORTAL] AP started SSID=");
  Serial.print(AP_SSID);
  Serial.print(" pass=");
  Serial.println(AP_PASS);

  _server.on("/", HTTP_GET, [&]() { _apRoot(cfg); });
  _server.on("/save", HTTP_POST, [&]() { _apSave(store); });
  _server.begin();
}

void WiFiManager::startStaLoginPortal(ConfigStore& store, DeviceConfig& cfg, const String& loginUrl) {
  _portalRunning = true;

  // STA already connected outside
  Serial.print("[PORTAL] STA login portal, URL=");
  Serial.println(loginUrl);
  _server.on("/", HTTP_GET, [&]() { _staRoot(cfg); });
  _server.on("/login", HTTP_POST, [&]() { _staLogin(store, loginUrl); });
  _server.begin();
}

void WiFiManager::loopWeb() {
  if (_portalRunning) _server.handleClient();
}

String WiFiManager::_htmlEscape(const String& s) const {
  String out = s;
  out.replace("&", "&amp;");
  out.replace("<", "&lt;");
  out.replace(">", "&gt;");
  out.replace("\"", "&quot;");
  out.replace("'", "&#39;");
  return out;
}

void WiFiManager::_apRoot(const DeviceConfig& cfg) {
  String ssid = _htmlEscape(cfg.wifiSsid);
  String html;
  html += "<!doctype html><html><head><meta charset='utf-8'/>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'/>";
  html += "<title>ESP32 WiFi Config</title></head><body style='font-family:Arial'>";
  html += "<h3>ESP32 WiFi Config (AP mode)</h3>";
  html += "<p>AP SSID: <b>ESP32_Config</b> | PASS: <b>12345678</b></p>";
  html += "<form method='POST' action='/save'>";
  html += "WiFi SSID:<br><input name='wifi_ssid' value='" + ssid + "' required><br><br>";
  html += "WiFi Password:<br><input name='wifi_pass' type='password' value=''><br><br>";
  html += "<button type='submit'>Save & Reboot</button>";
  html += "</form></body></html>";
  _server.send(200, "text/html", html);
}

void WiFiManager::_apSave(ConfigStore& store) {
  String ssid = _server.arg("wifi_ssid");
  String pass = _server.arg("wifi_pass");
  ssid.trim(); pass.trim();

  if (ssid.length() == 0) {
    _server.send(400, "text/plain", "Missing wifi_ssid");
    return;
  }

  Serial.print("[PORTAL] Saving SSID=");
  Serial.println(ssid);

  store.saveWifi(ssid, pass);
  _server.send(200, "text/html", "<html><body><h3>Saved! Rebooting...</h3></body></html>");
  delay(800);
  ESP.restart();
}

void WiFiManager::_staRoot(const DeviceConfig& cfg) {
  String html;
  html += "<!doctype html><html><head><meta charset='utf-8'/>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'/>";
  html += "<title>ESP32 Controller Login</title></head><body style='font-family:Arial'>";
  html += "<h3>Controller Login (STA mode)</h3>";
  html += "<p>WiFi connected. Please login to get controller_id.</p>";
  html += "<form method='POST' action='/login'>";
  html += "Username:<br><input name='username' required><br><br>";
  html += "Password:<br><input name='password' type='password' required><br><br>";
  html += "<button type='submit'>Login</button>";
  html += "</form>";
  html += "<p><small>No way to change WiFi except holding RESET button 5s.</small></p>";
  html += "</body></html>";
  _server.send(200, "text/html", html);
}

void WiFiManager::_staLogin(ConfigStore& store, const String& loginUrl) {
  String username = _server.arg("username");
  String password = _server.arg("password");
  username.trim(); password.trim();

  if (username.length() == 0 || password.length() == 0) {
    _server.send(400, "text/plain", "Missing username/password");
    return;
  }

  // HTTP POST -> expect controller_id
  Serial.print("[PORTAL] Login attempt user=");
  Serial.println(username);
  HTTPClient http;
  http.begin(loginUrl); // NOTE: change to https + cert pinning later if needed
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<256> req;
  req["username"] = username;
  req["password"] = password;
  req["device_id"] = String((uint32_t)ESP.getEfuseMac(), HEX);

  String body;
  serializeJson(req, body);

  int code = http.POST(body);
  String resp = http.getString();
  http.end();

  Serial.print("[PORTAL] Login HTTP code=");
  Serial.println(code);

  if (code < 200 || code >= 300) {
    _server.send(401, "text/plain", "Login failed. HTTP " + String(code) + "\n" + resp);
    return;
  }

  DynamicJsonDocument resDoc(1024);
  DeserializationError err = deserializeJson(resDoc, resp);
  if (err) {
    _server.send(500, "text/plain", "Invalid JSON response");
    return;
  }

  // accept controller_id at root or data.controller_id
  String cid = resDoc["controller_id"] | "";
  if (cid.length() == 0) {
    cid = resDoc["data"]["controller_id"] | "";
  }
  cid.trim();

  if (cid.length() == 0) {
    _server.send(500, "text/plain", "controller_id missing in response");
    return;
  }

  Serial.print("[PORTAL] Received controller_id=");
  Serial.println(cid);

  store.saveControllerId(cid);

  _server.send(200, "text/html", "<html><body><h3>Login OK! Saved controller_id. Rebooting...</h3></body></html>");
  delay(800);
  ESP.restart();
}

void WiFiManager::handleResetHold(ConfigStore& store) {
  bool pressedNow = (digitalRead(_resetPin) == LOW);

  if (pressedNow && !_pressed) {
    _pressed = true;
    _pressStart = millis();
  } else if (!pressedNow && _pressed) {
    _pressed = false;
  } else if (pressedNow && _pressed) {
    if (millis() - _pressStart >= _holdMs) {
      Serial.println("[WIFI] Reset hold detected -> clear config");
      store.clearAll();
      delay(300);
      ESP.restart();
    }
  }
}
