#pragma once
#include <Arduino.h>

class Dht11Sensor {
public:
  void begin(uint8_t gpioPin);
  bool read(float& outTempC, float& outHumid);

private:
  uint8_t _pin = 255;
};
