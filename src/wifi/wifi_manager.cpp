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
  if (cfg.wifiSsid.length() == 0 || cfg.controllerId.length() == 0) {
    Serial.println("[WIFI] Mode AP_CONFIG (missing ssid or controller_id)");
    return DeviceMode::AP_CONFIG;
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
  String cid = _htmlEscape(cfg.controllerId);
  String html;
  html += "<!doctype html><html><head><meta charset='utf-8'/>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'/>";
  html += "<title>ESP32 WiFi Config</title></head><body style='font-family:Arial'>";
  html += "<h3>ESP32 WiFi Config (AP mode)</h3>";
  // html += "<p>AP SSID: <b>ESP32_Config</b> | PASS: <b>12345678</b></p>";
  html += "<form method='POST' action='/save'>";
  html += "WiFi SSID:<br><input name='wifi_ssid' value='" + ssid + "' required><br><br>";
  html += "WiFi Password:<br><input name='wifi_pass' type='password' value=''><br><br>";
  html += "Controller ID:<br><input name='controller_id' value='" + cid + "' required><br><br>";
  html += "<button type='submit'>Save & Reboot</button>";
  html += "</form></body></html>";
  _server.send(200, "text/html", html);
}

void WiFiManager::_apSave(ConfigStore& store) {
  String ssid = _server.arg("wifi_ssid");
  String pass = _server.arg("wifi_pass");
  String cid = _server.arg("controller_id");
  ssid.trim(); pass.trim(); cid.trim();

  if (ssid.length() == 0) {
    _server.send(400, "text/plain", "Missing wifi_ssid");
    return;
  }
  if (cid.length() == 0) {
    _server.send(400, "text/plain", "Missing controller_id");
    return;
  }

  Serial.print("[PORTAL] Saving SSID=");
  Serial.print(ssid);
  Serial.print(" controller_id=");
  Serial.println(cid);

  store.saveWifi(ssid, pass);
  store.saveControllerId(cid);
  _server.send(200, "text/html", "<html><body><h3>Saved! Rebooting...</h3></body></html>");
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
