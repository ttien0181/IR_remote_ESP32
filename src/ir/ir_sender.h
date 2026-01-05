#pragma once
#include <Arduino.h>

class IRSender {
public:
  void begin(uint8_t gpioPin);

  // parsed protocols
  bool sendSirc(uint32_t data, uint16_t bits);
  bool sendNec(uint32_t data, uint16_t bits);
  bool sendSamsung(uint32_t data, uint16_t bits);

  // raw
  bool sendRaw(const uint16_t* buf, uint16_t len, uint16_t khz);

private:
  uint8_t _pin = 255;
};
