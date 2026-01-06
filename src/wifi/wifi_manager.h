#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include "../config/config_store.h"

enum class DeviceMode {
  AP_CONFIG,
  RUN
};

class WiFiManager {
public:
  void begin(uint8_t resetPin, uint32_t holdMs);

  // main state machine
  DeviceMode decideMode(const DeviceConfig& cfg) const;

  // AP config (save ssid/pass)
  void startApPortal(ConfigStore& store, DeviceConfig& cfg);



  // WiFi connect
  bool connectSta(const DeviceConfig& cfg, uint32_t timeoutMs);

  // loop for WebServer
  void loopWeb();

  // reset button hold -> clear config
  void handleResetHold(ConfigStore& store);

  bool isPortalRunning() const { return _portalRunning; }

private:
  uint8_t _resetPin = 255;
  uint32_t _holdMs = 5000;

  bool _pressed = false;
  uint32_t _pressStart = 0;

  bool _portalRunning = false;
  WebServer _server{80};

  // helpers
  String _htmlEscape(const String& s) const;
  void _stopPortal();

  // handlers AP
  void _apRoot(const DeviceConfig& cfg);
  void _apSave(ConfigStore& store);



  // store temp form values
  String _tmpSsid, _tmpPass;
};
