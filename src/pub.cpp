// // #include <WiFi.h>
// #include <WiFiClientSecure.h>
// #include <PubSubClient.h>

// const char* ssid = "Huster";
// const char* password = "husterk67";

// const char* mqtt_server = "c55d02d9fd4d4b4f812d0e68dc8b3ef6.s1.eu.hivemq.cloud";
// const int mqtt_port = 8883;
// const char* mqtt_user = "ttien0181";
// const char* mqtt_password = "Ttien0181";

// WiFiClientSecure espClient;
// PubSubClient client(espClient);

// void setup_wifi() {
//   WiFi.begin(ssid, password);
//   Serial.print("Connecting");
//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//     Serial.print(".");
//   }
//   Serial.println("\nWiFi connected");
// }

// void reconnect() {
//   while (!client.connected()) {
//     Serial.print("Attempting MQTT connection...");
//     if (client.connect("ESP32_Publisher", mqtt_user, mqtt_password)) {
//       Serial.println("connected");
//     } else {
//       Serial.print("failed, rc=");
//       Serial.println(client.state());
//       delay(2000);
//     }
//   }
// }

// void setup() {
//   Serial.begin(9600);
//   setup_wifi();
//   espClient.setInsecure();   // BẮT BUỘC cho HiveMQ Cloud nếu không dùng CA
//   client.setServer(mqtt_server, mqtt_port);
// }

// void loop() {
//   if (!client.connected()) reconnect();
//   client.loop();

//   client.publish("demo/hello", "Hello MQTT!");
//   Serial.println("Published: Hello MQTT!");
//   delay(3000);
// }
