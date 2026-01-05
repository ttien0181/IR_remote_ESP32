// #include <WiFi.h>
// #include <WiFiClientSecure.h>
// #include <PubSubClient.h>
// #include <ArduinoJson.h>

// #include <IRremoteESP8266.h>
// #include <IRsend.h>

// //  WIFI 
// const char* ssid = "Huster";
// const char* password = "husterk67";

// //  MQTT 
// const char* mqtt_server = "c55d02d9fd4d4b4f812d0e68dc8b3ef6.s1.eu.hivemq.cloud";
// const int mqtt_port = 8883;
// const char* mqtt_user = "ttien0181";
// const char* mqtt_password = "Ttien0181";

// const char* SUB_TOPIC = "device/694d792451e6b027e8193520/commands";

// // JSON
// #define MAX_SIZE_RAW 1000
// #define MAX_SIZE_PAYLOAD 12000
// DynamicJsonDocument doc(MAX_SIZE_PAYLOAD); // doc lưu json payload
// static uint16_t rawBuf[MAX_SIZE_RAW]; // Mỗi số là thời gian (microsecond) mà IR LED bật hoặc tắt trong khoảng đó
// // ví dụ 190, 256, 356, ... là bật 190 micro s, tắt 256 micro s, bật 356 micro s, ...

// //  IR 
// #define IR_LED_PIN 17
// IRsend irsend(IR_LED_PIN);

// //  CLIENT 
// WiFiClientSecure espClient;
// PubSubClient client(espClient);


// // setup WIFI
// void setup_wifi() {
//   Serial.print("Connecting WiFi");
//   WiFi.begin(ssid, password);

//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//     Serial.print(".");
//   }

//   Serial.println("\nWiFi connected");
//   Serial.println(WiFi.localIP());
// }


// // MQTT CALLBACK
// // byte* payload: con trỏ trỏ tới buffer byte[] do thư viện MQTT cấp phát
// // unsigned int length: số byte hợp lệ
// void mqttCallback(char* topic, byte* payload, unsigned int length) {
//   Serial.println("\n📩 MQTT message received");
//   Serial.println(topic);

//   doc.clear();
//   DeserializationError error = deserializeJson(doc, payload, length); // parse từ payload vào doc
//   if (error) { // nếu có lỗi
//     Serial.print("ERROR: JSON parse failed: ");
//     Serial.println(error.c_str());
//     return;
//   }

//   const char* protocol = doc["protocol"]; // lấy protocol
//   if (!protocol) {
//     Serial.println("ERROR: protocol missing");
//     return;
//   }

//   char protocolUpperCase[20]; // khai báo mảng char để lưu protocol IN HOA
//   strncpy(protocolUpperCase, protocol, sizeof(protocolUpperCase)); // copy vào mảng char đó, tối đa 20 ký tự
//   protocolUpperCase[sizeof(protocolUpperCase) - 1] = '\0'; // đặt ký tự cuỗi mảng thành ký tự kế thúc chuỗi


//   // chuyển thành in hoa
//   for (int i = 0; protocolUpperCase[i]; i++) {
//     protocolUpperCase[i] = toupper((unsigned char)protocolUpperCase[i]);
//   }


//   // strupr(protocolUpperCase); // chuyển thành in hoa

//   int frequency = doc["frequency"] | 38; // lấy tần số

//   if (strcmp(protocolUpperCase, "RAW") == 0) { // nếu là RAW
//     JsonArray rawArray = doc["raw_data"]; // lấy trường "raw_data"

//     if (rawArray.isNull() || rawArray.size() == 0) {
//       Serial.println("ERROR: raw_data missing/empty");
//       return;
//     }

//     int rawLen = rawArray.size();
//     if (rawLen > (int)(sizeof(rawBuf) / sizeof(rawBuf[0]))) { // lỗi khi lenght rawBuff > lenght rawBuf
//       Serial.printf("ERROR: RAW too long: %d (max %d)\n",
//                     rawLen, (int)(sizeof(rawBuf) / sizeof(rawBuf[0])));
//       return;
//     }

//     // copy từ rawArray sang rawBuf
//     for (int i = 0; i < rawLen; i++) { 
//       int v = rawArray[i].as<int>(); // chuyển thành int
//       if (v <= 0 || v > 65535) { //  vì là uint16_t 
//         Serial.printf("ERROR: Invalid RAW[%d]=%d\n", i, v);
//         return;
//       }
//       rawBuf[i] = (uint16_t)v;
//     }

//     Serial.printf("LOADING: Sending RAW (%d) @ %dkHz\n", rawLen, frequency);
//     irsend.sendRaw(rawBuf, rawLen, frequency); // send raw
//     Serial.println("SUCCESS: RAW sent");
//     return;
//   }

//   // nếu ko là RAW, lấy trường "data" và "bits"
//   int bits = doc["bits"] | 0; 
//   const char* data = doc["data"];
//   if (!data || bits <= 0) {
//     Serial.println("ERROR: No data/bits for parsed protocol");
//     return;
//   }

//   unsigned long dataValue = strtoul(data, NULL, 16);
//   Serial.printf("➡ Protocol: %s | Bits: %d | Data: 0x%lX\n",
//                 protocolUpperCase, bits, dataValue);

//   if (strcmp(protocolUpperCase, "SIRC") == 0) {
//     irsend.sendSony(dataValue, bits);
//   } else if (strcmp(protocolUpperCase, "NEC") == 0) {
//     irsend.sendNEC(dataValue, bits);
//   } else if (strcmp(protocolUpperCase, "SAMSUNG") == 0) {
//     irsend.sendSAMSUNG(dataValue, bits);
//   } else {
//     Serial.println("ERROR: Unsupported protocol");
//   }
// }

// // MQTT RECONNECT
// void reconnect() {
//   while (!client.connected()) {
//     Serial.print("Connecting MQTT... ");

//     if (client.connect("ESP32_IR", mqtt_user, mqtt_password)) {
//       Serial.println("connected");
//       client.subscribe(SUB_TOPIC);
//       Serial.print("Subscribed: ");
//       Serial.println(SUB_TOPIC);
//     } else {
//       Serial.print("failed, rc=");
//       Serial.println(client.state());
//       delay(2000);
//     }
//   }
// }


// // SETUP
// void setup() {
//   Serial.begin(9600);

//   setup_wifi();

//   // HiveMQ Cloud TLS
//   espClient.setInsecure();

//   client.setServer(mqtt_server, mqtt_port);
//   client.setCallback(mqttCallback);

//   irsend.begin();

//   Serial.println("🚀 ESP32 ready");
// }

// // LOOP
// void loop() {
//   if (!client.connected()) {
//     reconnect();
//   }
//   client.loop();
// }


