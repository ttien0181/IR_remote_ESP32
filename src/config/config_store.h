#pragma once
#include <Arduino.h>

struct DeviceConfig {
  String wifiSsid;
  String wifiPass;
  String controllerId; // lấy từ HTTP login
};

class ConfigStore {
public:
  bool load(DeviceConfig& out);
  bool saveWifi(const String& ssid, const String& pass);
  bool saveControllerId(const String& controllerId);
  void clearAll();

private:
  const char* NS = "devcfg";
};
