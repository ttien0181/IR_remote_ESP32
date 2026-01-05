// #include <WiFi.h>
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

// // Callback nhận dữ liệu khi có message từ broker
// void callback(char* topic, byte* payload, unsigned int length) {
//   Serial.print("Message arrived [");
//   Serial.print(topic);
//   Serial.print("]: ");

//   for (unsigned int i = 0; i < length; i++) {
//     Serial.print((char)payload[i]);
//   }
//   Serial.println();
// }

// void setup_wifi() {
//   WiFi.begin(ssid, password);
//   Serial.print("Connecting to WiFi");
//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//     Serial.print(".");
//   }
//   Serial.println("\nWiFi connected");
// }

// void reconnect() {
//   while (!client.connected()) {
//     Serial.print("Attempting MQTT connection...");
//     if (client.connect("ESP32_Subscriber", mqtt_user, mqtt_password)) {
//       Serial.println("connected");
//       // Subscribe vào topic bạn muốn nhận
//       client.subscribe("demo/hello");
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

//   espClient.setInsecure();   // BẮT BUỘC nếu không dùng CA hợp lệ
//   client.setServer(mqtt_server, mqtt_port);
//   client.setCallback(callback);  // Gán hàm callback để nhận message
// }

// void loop() {
//   if (!client.connected()) {
//     reconnect();
//   }
//   client.loop();
// }
