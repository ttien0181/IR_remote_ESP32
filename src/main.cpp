#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "config/config_store.h"
#include "wifi/wifi_manager.h"
#include "mqtt/mqtt_client.h"
#include "ir/ir_sender.h"
#include "sensor/dht11.h"
#include "utils/time_utils.h"

#include <ArduinoJson.h>

//  Pins 
static const uint8_t PIN_IR   = 21;
static const uint8_t PIN_DHT  = 4;
static const uint8_t PIN_RESET = 0;     // bạn chốt reset pin 0
static const uint32_t RESET_HOLD_MS = 5000;

//  MQTT server 
static const char* MQTT_HOST = "c55d02d9fd4d4b4f812d0e68dc8b3ef6.s1.eu.hivemq.cloud";
static const int   MQTT_PORT = 8883;
static const char* MQTT_USER = "ttien0181";
static const char* MQTT_PASS = "Ttien0181";

//  HTTP login endpoint 
static const char* LOGIN_URL = "http://localhost:5000/api/device/login";

//  Globals 
ConfigStore g_store;
DeviceConfig g_cfg;

WiFiManager g_wifiMgr;

WiFiClientSecure g_tls;
MqttClient g_mqtt;

IRSender g_ir;
Dht11Sensor g_dht;

// RAW buffer for commands
static uint16_t g_rawBuf[1200];

// 10s phát 1 lần 
static uint32_t g_lastSensorMs = 0;
static const uint32_t SENSOR_INTERVAL_MS = 10000;

//  parse protocol uppercase without String
static void toUpperCopy(const char* src, char* dst, size_t dstSize) {
  if (!src || dstSize == 0) return;
  strncpy(dst, src, dstSize - 1);
  dst[dstSize - 1] = '\0';
  for (size_t i = 0; dst[i]; i++) dst[i] = (char)toupper((unsigned char)dst[i]);
}

//  Command handler (MQTT -> IR -> ACK)
static void handleCommand(const JsonDocument& doc) {
  const char* protocol = doc["protocol"];
  if (!protocol) return;

  char proto[20];
  toUpperCopy(protocol, proto, sizeof(proto));

  String commandId = doc["command_id"] | "";
  commandId.trim();

  Serial.print("[CMD] Received protocol=");
  Serial.print(proto);
  Serial.print(" command_id=");
  Serial.println(commandId);

  if (strcmp(proto, "RAW") == 0) {
    int frequency = doc["frequency"] | 38; // kHz
    JsonArrayConst  arr = doc["raw_data"];
    if (arr.isNull() || arr.size() == 0) return;

    int len = (int)arr.size();
    if (len > (int)(sizeof(g_rawBuf) / sizeof(g_rawBuf[0]))) return;

    for (int i = 0; i < len; i++) {
      int v = arr[i].as<int>();
      if (v <= 0 || v > 65535) return;
      g_rawBuf[i] = (uint16_t)v;
    }

    Serial.print("[IR] Send RAW len=");
    Serial.print(len);
    Serial.print(" freq=");
    Serial.println(frequency);
    g_ir.sendRaw(g_rawBuf, (uint16_t)len, (uint16_t)frequency);

  } else {
    int bits = doc["bits"] | 0;
    const char* dataStr = doc["data"];
    if (!dataStr || bits <= 0) return;

    unsigned long data = strtoul(dataStr, nullptr, 16);

    Serial.print("[IR] Send ");
    Serial.print(proto);
    Serial.print(" data=0x");
    Serial.print(dataStr);
    Serial.print(" bits=");
    Serial.println(bits);

    if (strcmp(proto, "SIRC") == 0) {
      g_ir.sendSirc((uint32_t)data, (uint16_t)bits);
    } else if (strcmp(proto, "NEC") == 0) {
      g_ir.sendNec((uint32_t)data, (uint16_t)bits);
    } else if (strcmp(proto, "SAMSUNG") == 0) {
      g_ir.sendSamsung((uint32_t)data, (uint16_t)bits);
    } else {
      // unsupported => still ACK? tuỳ bạn; hiện tại không ack nếu không hỗ trợ
      Serial.println("[IR] Unsupported protocol");
      return;
    }
  }

  // ACK after sending
  if (commandId.length() > 0) {
    String nowIso = TimeUtils::nowIso8601Utc();
    g_mqtt.publishAck(commandId, nowIso);
    Serial.print("[CMD] Acked command_id=");
    Serial.println(commandId);
  }
}

void setup() {
  Serial.begin(9600);
  delay(200);

  Serial.println("[BOOT] Setup start");

  // load config
  g_store.load(g_cfg);
  Serial.print("[CFG] wifi_ssid=");
  Serial.print(g_cfg.wifiSsid);
  Serial.print(" controller_id=");
  Serial.println(g_cfg.controllerId);

  // init reset
  g_wifiMgr.begin(PIN_RESET, RESET_HOLD_MS);

  // init IR + sensor
  g_ir.begin(PIN_IR);
  g_dht.begin(PIN_DHT);

  Serial.println("[INIT] IR and DHT initialized");

  // decide mode
  DeviceMode mode = g_wifiMgr.decideMode(g_cfg);

  if (mode == DeviceMode::AP_CONFIG) {
    Serial.println("[MODE] AP_CONFIG");
    g_wifiMgr.startApPortal(g_store, g_cfg);
    return;
  }

  // Need STA
  Serial.println("[WIFI] connecting...");
  if (!g_wifiMgr.connectSta(g_cfg, 20000)) {
    Serial.println("[WIFI] failed -> AP_CONFIG");
    g_wifiMgr.startApPortal(g_store, g_cfg);
    return;
  }
  Serial.print("[WIFI] IP: ");
  Serial.println(WiFi.localIP());

  TimeUtils::beginNtp();
  Serial.println("[NTP] Requesting time sync");

  // RUN mode
  Serial.println("[MODE] RUN");

  // TLS insecure (như bạn đang dùng); sau này thay CA pinning nếu muốn
  g_tls.setInsecure();

  g_mqtt.begin(g_tls, MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASS);
  g_mqtt.setLastWillEnabled(true);
  g_mqtt.setControllerId(g_cfg.controllerId);
  g_mqtt.onCommand(handleCommand);

  g_mqtt.ensureConnected();
  Serial.println("[MQTT] Connected and subscribed");
}

static void publishSensorsIfDue() {
  uint32_t now = millis();
  if (now - g_lastSensorMs < SENSOR_INTERVAL_MS) return;
  g_lastSensorMs = now;

  float t = 0, h = 0;
  if (!g_dht.read(t, h)) {
    Serial.println("[DHT11] Read failed");
    return;
  }

  String ts = TimeUtils::nowIso8601Utc();

  // temp
  g_mqtt.publishMetric("temp", t, "C", ts);
  // humid
  g_mqtt.publishMetric("humid", h, "%", ts);
  Serial.print("[DHT11] temp=");
  Serial.print(t);
  Serial.print("C humid=");
  Serial.print(h);
  Serial.print("% ts=");
  Serial.println(ts);
}

void loop() {
  // reset hold always works
  g_wifiMgr.handleResetHold(g_store);

  // portal mode?
  if (g_wifiMgr.isPortalRunning()) {
    g_wifiMgr.loopWeb();
    delay(2);
    return;
  }

  // RUN mode
  if (!g_mqtt.connected()) {
    g_mqtt.ensureConnected();
  }
  g_mqtt.loop();

  publishSensorsIfDue();
}
