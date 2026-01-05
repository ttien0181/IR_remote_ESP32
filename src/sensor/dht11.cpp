#include "dht11.h"
#include <DHT.h>

namespace {
  DHT* g_dht = nullptr;
}

void Dht11Sensor::begin(uint8_t gpioPin) {
  _pin = gpioPin;
  if (g_dht) { delete g_dht; g_dht = nullptr; }
  g_dht = new DHT(_pin, DHT11);
  g_dht->begin();
  Serial.print("[DHT11] Init pin=");
  Serial.println(_pin);
}

bool Dht11Sensor::read(float& outTempC, float& outHumid) {
  if (!g_dht) return false;

  float h = g_dht->readHumidity();
  float t = g_dht->readTemperature(); // Celsius

  if (isnan(h) || isnan(t)) {
    Serial.println("[DHT11] Invalid reading");
    return false;
  }

  outTempC = t;
  outHumid = h;
  Serial.print("[DHT11] Read temp=");
  Serial.print(outTempC);
  Serial.print("C humid=");
  Serial.println(outHumid);
  return true;
}
