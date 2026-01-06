#include "time_utils.h"
#include <time.h>

namespace {
  const long GMT_OFFSET_SEC = 0;
  const int  DAYLIGHT_OFFSET_SEC = 0;

  bool timeValidImpl() {
    time_t now = time(nullptr);
    // mốc sanity: 2024-01-01
    return (now > 1704067200);
  }
}

namespace TimeUtils {

void beginNtp() {
  // NTP servers
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, "pool.ntp.org", "time.google.com", "time.cloudflare.com");
  Serial.println("[TIME] NTP config requested");
}

bool isTimeValid() {
  return timeValidImpl();
}

String nowIso8601Utc() {
  // Nếu chưa valid, vẫn trả chuỗi theo millis (đỡ null.
  if (!timeValidImpl()) {
    return String("1970-01-01T00:00:00.000Z");
  }

  struct timeval tv;
  gettimeofday(&tv, nullptr);

  time_t nowSec = tv.tv_sec;
  struct tm tmUtc;
  gmtime_r(&nowSec, &tmUtc);

  char buf[32];
  int ms = (int)(tv.tv_usec / 1000);

  // YYYY-MM-DDTHH:MM:SS.mmmZ
  snprintf(buf, sizeof(buf),
           "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
           tmUtc.tm_year + 1900, tmUtc.tm_mon + 1, tmUtc.tm_mday,
           tmUtc.tm_hour, tmUtc.tm_min, tmUtc.tm_sec, ms);

  return String(buf);
}

} // namespace
