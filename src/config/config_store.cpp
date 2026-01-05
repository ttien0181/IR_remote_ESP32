#include "config_store.h"
#include <Preferences.h>

bool ConfigStore::load(DeviceConfig& out) {
  Preferences prefs;
  if (!prefs.begin(NS, true)) return false;

  out.wifiSsid = prefs.getString("ssid", "");
  out.wifiPass = prefs.getString("pass", "");
  out.controllerId = prefs.getString("cid", "");

  prefs.end();

  out.wifiSsid.trim();
  out.wifiPass.trim();
  out.controllerId.trim();
  return true;
}

bool ConfigStore::saveWifi(const String& ssid, const String& pass) {
  Preferences prefs;
  if (!prefs.begin(NS, false)) return false;
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();
  return true;
}

bool ConfigStore::saveControllerId(const String& controllerId) {
  Preferences prefs;
  if (!prefs.begin(NS, false)) return false;
  prefs.putString("cid", controllerId);
  prefs.end();
  return true;
}

void ConfigStore::clearAll() {
  Preferences prefs;
  if (!prefs.begin(NS, false)) return;
  prefs.clear();
  prefs.end();
}
