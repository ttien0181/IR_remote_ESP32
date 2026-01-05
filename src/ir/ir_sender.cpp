#include "ir_sender.h"
#include <IRremoteESP8266.h>
#include <IRsend.h>

namespace {
  IRsend* g_ir = nullptr;
}

void IRSender::begin(uint8_t gpioPin) {
  _pin = gpioPin;
  if (g_ir) { delete g_ir; g_ir = nullptr; }
  g_ir = new IRsend(_pin);
  g_ir->begin();
  Serial.print("[IR] Init pin=");
  Serial.println(_pin);
}

bool IRSender::sendSirc(uint32_t data, uint16_t bits) {
  if (!g_ir) {
    Serial.println("[IR] sendSirc: IR not ready");
    return false;
  }
  Serial.print("[IR] SIRC data=");
  Serial.print(data, HEX);
  Serial.print(" bits=");
  Serial.println(bits);
  g_ir->sendSony(data, bits);
  return true;
}

bool IRSender::sendNec(uint32_t data, uint16_t bits) {
  if (!g_ir) {
    Serial.println("[IR] sendNec: IR not ready");
    return false;
  }
  Serial.print("[IR] NEC data=");
  Serial.print(data, HEX);
  Serial.print(" bits=");
  Serial.println(bits);
  g_ir->sendNEC(data, bits);
  return true;
}

bool IRSender::sendSamsung(uint32_t data, uint16_t bits) {
  if (!g_ir) {
    Serial.println("[IR] sendSamsung: IR not ready");
    return false;
  }
  Serial.print("[IR] SAMSUNG data=");
  Serial.print(data, HEX);
  Serial.print(" bits=");
  Serial.println(bits);
  g_ir->sendSAMSUNG(data, bits);
  return true;
}

bool IRSender::sendRaw(const uint16_t* buf, uint16_t len, uint16_t khz) {
  if (!g_ir) {
    Serial.println("[IR] sendRaw: IR not ready");
    return false;
  }
  Serial.print("[IR] RAW len=");
  Serial.print(len);
  Serial.print(" khz=");
  Serial.println(khz);
  g_ir->sendRaw(buf, len, khz);
  return true;
}
