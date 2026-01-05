#pragma once
#include <Arduino.h>

namespace TimeUtils {
  void beginNtp();               // gọi sau khi WiFi connected
  bool isTimeValid();            // đã sync chưa
  String nowIso8601Utc();        // "2026-01-05T19:53:22.125Z"
}
