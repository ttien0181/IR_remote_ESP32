#include "mqtt_client.h"

namespace {
  MqttClient* g_self = nullptr;
}

void MqttClient::begin(WiFiClientSecure& net, const char* host, int port, const char* user, const char* pass) {
  _net = &net;
  _host = host;
  _port = port;
  _user = user;
  _pass = pass;

  Serial.print("[MQTT] Init host=");
  Serial.print(_host);
  Serial.print(":");
  Serial.println(_port);

  _client.setClient(net);
  _client.setServer(host, port);

  g_self = this;
  _client.setCallback(_staticCallback);
}

void MqttClient::setControllerId(const String& controllerId) {
  _controllerId = controllerId;
  _controllerId.trim();

  _topics.commands = "device/" + _controllerId + "/commands";
  _topics.ack      = "device/" + _controllerId + "/ack";
  _topics.data     = "device/" + _controllerId + "/data";
  _topics.status   = "device/" + _controllerId + "/status";

  Serial.print("[MQTT] Topics set for controller=");
  Serial.println(_controllerId);
}

void MqttClient::setLastWillEnabled(bool enabled) {
  _lwtEnabled = enabled;
}

bool MqttClient::connected()  {
  return _client.connected();
}

void MqttClient::loop() {
  _client.loop();
}

void MqttClient::onCommand(CommandHandler cb) {
  _cmdHandler = cb;
}

bool MqttClient::ensureConnected() {
  if (_controllerId.length() == 0) return false;
  if (_client.connected()) return true;

  while (!_client.connected()) {
    String cid = "ESP32-" + String((uint32_t)ESP.getEfuseMac(), HEX);

    Serial.print("[MQTT] Connecting cid=");
    Serial.println(cid);

    // LWT payload đơn giản
    const char* willPayload = "{\"status\":\"offline\"}";
    bool ok = false;

    if (_lwtEnabled) {
      ok = _client.connect(
        cid.c_str(),
        _user, _pass,
        _topics.status.c_str(), 0, false, willPayload
      );
    } else {
      ok = _client.connect(cid.c_str(), _user, _pass);
    }

    if (ok) {
      // online status (không retain)
      _client.publish(_topics.status.c_str(), "{\"status\":\"online\"}", false);

      // subscribe commands QoS0
      _client.subscribe(_topics.commands.c_str(), 0);
      Serial.println("[MQTT] Connected and subscribed");
      return true;
    }

    Serial.println("[MQTT] Connect failed, retrying...");
    delay(1500);
  }

  return false;
}

bool MqttClient::publishAck(const String& commandId, const String& ackAtIso) {
  if (!_client.connected()) {
    Serial.println("[MQTT] publishAck skipped (not connected)");
    return false;
  }

  StaticJsonDocument<256> d;
  d["command_id"] = commandId;
  d["status"] = "acked";
  d["ack_at"] = ackAtIso;

  char out[256];
  size_t n = serializeJson(d, out, sizeof(out));
  bool ok = _client.publish(
    _topics.ack.c_str(),
    (const uint8_t*)out,
    n,
    false
  );
  Serial.print("[MQTT] Ack publish command_id=");
  Serial.print(commandId);
  Serial.print(" result=");
  Serial.println(ok ? "OK" : "FAIL");
  return ok;
}

bool MqttClient::publishMetric(const String& metric, float value, const String& unit, const String& tsIso) {
  if (!_client.connected()) {
    Serial.println("[MQTT] publishMetric skipped (not connected)");
    return false;
  }

  StaticJsonDocument<256> d;
  d["metric"] = metric;
  d["value"] = value;
  d["unit"] = unit;
  d["ts"] = tsIso;

  char out[256];
  size_t n = serializeJson(d, out, sizeof(out));
  bool ok = _client.publish(
    _topics.data.c_str(),
    (const uint8_t*)out,
    n,
    false
  );
  Serial.print("[MQTT] Metric=");
  Serial.print(metric);
  Serial.print(" value=");
  Serial.print(value);
  Serial.print(" unit=");
  Serial.print(unit);
  Serial.print(" ts=");
  Serial.print(tsIso);
  Serial.print(" result=");
  Serial.println(ok ? "OK" : "FAIL");
  return ok;
}

void MqttClient::_staticCallback(char* topic, byte* payload, unsigned int length) {
  if (g_self) g_self->_callback(topic, payload, length);
}

void MqttClient::_callback(char* topic, byte* payload, unsigned int length) {
  // parse JSON from payload
  _doc.clear();
  DeserializationError err = deserializeJson(_doc, payload, length);
  if (err) {
    Serial.print("[MQTT] JSON parse error: ");
    Serial.println(err.c_str());
    return;
  }

  if (_cmdHandler) _cmdHandler(_doc);
}
