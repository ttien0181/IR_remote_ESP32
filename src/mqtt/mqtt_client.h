#pragma once
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

struct MqttTopics {
  String commands;
  String ack;
  String data;
  String status;
};

typedef void (*CommandHandler)(const JsonDocument& doc);

class MqttClient {
public:
  void begin(WiFiClientSecure& net, const char* host, int port, const char* user, const char* pass);

  void setControllerId(const String& controllerId);
  void setLastWillEnabled(bool enabled);

  void loop();
  bool ensureConnected();
  bool connected();

  // publish
  bool publishAck(const String& commandId, const String& ackAtIso);
  bool publishMetric(const String& metric, float value, const String& unit, const String& tsIso);

  void onCommand(CommandHandler cb);

private:
  static void _staticCallback(char* topic, byte* payload, unsigned int length);
  void _callback(char* topic, byte* payload, unsigned int length);

  PubSubClient _client;
  WiFiClientSecure* _net = nullptr;

  const char* _host = nullptr;
  int _port = 8883;
  const char* _user = nullptr;
  const char* _pass = nullptr;

  bool _lwtEnabled = true;

  String _controllerId;
  MqttTopics _topics;

  CommandHandler _cmdHandler = nullptr;

  // parse buffer
  DynamicJsonDocument _doc{12000};
};
