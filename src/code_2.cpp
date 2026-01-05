// /*
//   ESP32 IR MQTT Controller + AP Config Portal
//   - Subscribe: /{userid}/ir_commands/#
//   - JSON fields used: protocol, data, bits, rawData, frequency
//   - protocol: "raw" or "Sony" or "Samsung" or "NEC"
//   - raw: sendRaw(rawData[], freq)
//   - others: send<Protocol>(data, bits)

//   AP Config:
//     SSID: ESP32_IR_Config
//     PASS: 12345678
//     Web:  http://192.168.4.1/
//     Form fields: wifi_ssid, wifi_pass, userid

//   Reset:
//     Hold BOOT button (GPIO0) ~5 seconds => clear config & reboot to AP mode
// */

// #include <WiFi.h>
// #include <WebServer.h>
// #include <Preferences.h>

// #include <WiFiClientSecure.h>
// #include <PubSubClient.h>
// #include <ArduinoJson.h>

// #include <IRremoteESP8266.h>
// #include <IRsend.h>

// // ================== IR config ==================
// #define IR_SEND_PIN 4
// IRsend irsend(IR_SEND_PIN);

// // raw buffer (tăng/giảm tuỳ remote của bạn)
// static const size_t MAX_RAW_LEN = 1200;   // đủ cho raw rất dài
// uint16_t irRawBuf[MAX_RAW_LEN];

// // ================== BUTTON reset (BOOT) ==================
// #define BOOT_BTN_PIN 0
// static const uint32_t HOLD_RESET_MS = 5000;

// // ================== AP Config ==================
// const char* AP_SSID = "ESP32_IR_Config";
// const char* AP_PASS = "12345678";
// IPAddress apIP(192, 168, 4, 1);
// IPAddress apGW(192, 168, 4, 1);
// IPAddress apMASK(255, 255, 255, 0);

// // ================== MQTT Config (giữ như bản cũ) ==================
// const char* mqtt_server   = "c55d02d9fd4d4b4f812d0e68dc8b3ef6.s1.eu.hivemq.cloud";
// const int   mqtt_port     = 8883;
// const char* mqtt_user     = "ttien0181";
// const char* mqtt_password = "Ttien0181";

// // MQTT packet buffer lớn vì rawData rất dài
// #define MQTT_MAX_PACKET_SIZE 6000

// // ================== NVS storage ==================
// Preferences prefs;
// const char* NVS_NS = "ircfg";

// // ================== Global runtime ==================
// WebServer server(80);

// WiFiClientSecure espClient;
// PubSubClient client(espClient);

// String g_wifiSsid;
// String g_wifiPass;
// String g_userId;
// String g_topicSub;

// bool g_inConfigMode = false;

// // ================== Helpers ==================
// static inline String htmlEscape(const String& s) {
//   String out = s;
//   out.replace("&", "&amp;");
//   out.replace("<", "&lt;");
//   out.replace(">", "&gt;");
//   out.replace("\"", "&quot;");
//   out.replace("'", "&#39;");
//   return out;
// }

// static bool isBlank(const String& s) {
//   for (size_t i = 0; i < s.length(); i++) {
//     if (!isWhitespace(s[i])) return false;
//   }
//   return true;
// }

// static uint64_t parseHexToU64(String hex) {
//   hex.trim();
//   hex.toLowerCase();
//   if (hex.startsWith("0x")) hex = hex.substring(2);
//   // bỏ khoảng trắng nếu user gửi kiểu "38 00 00"
//   hex.replace(" ", "");
//   if (hex.length() == 0) return 0;

//   // strtoull cần c-string
//   return strtoull(hex.c_str(), nullptr, 16);
// }

// // Parse rawData: hỗ trợ string "5050 2165 361 ..." hoặc JsonArray [5050,2165,361,...]
// // Return len (0 nếu fail)
// static size_t parseRawData(JsonVariant rawVar) {
//   size_t len = 0;

//   if (rawVar.is<JsonArray>()) {
//     JsonArray arr = rawVar.as<JsonArray>();
//     for (JsonVariant v : arr) {
//       if (len >= MAX_RAW_LEN) break;
//       long val = v.as<long>();
//       if (val < 0) val = 0;
//       if (val > 65535) val = 65535;
//       irRawBuf[len++] = (uint16_t)val;
//     }
//     return len;
//   }

//   if (rawVar.is<const char*>() || rawVar.is<String>()) {
//     String s = rawVar.as<String>();
//     s.trim();
//     if (s.length() == 0) return 0;

//     // parse số cách nhau bởi space/newline/tab/ dấu phẩy
//     size_t i = 0;
//     while (i < s.length() && len < MAX_RAW_LEN) {
//       // skip separators
//       while (i < s.length() && (s[i] == ' ' || s[i] == '\n' || s[i] == '\r' || s[i] == '\t' || s[i] == ',')) i++;
//       if (i >= s.length()) break;

//       // read number
//       bool neg = false;
//       if (s[i] == '-') { neg = true; i++; }

//       long num = 0;
//       bool hasDigit = false;
//       while (i < s.length() && isDigit(s[i])) {
//         hasDigit = true;
//         num = num * 10 + (s[i] - '0');
//         i++;
//       }
//       if (!hasDigit) {
//         // ký tự lạ => skip 1 char để tránh loop kẹt
//         i++;
//         continue;
//       }
//       if (neg) num = 0;
//       if (num > 65535) num = 65535;
//       irRawBuf[len++] = (uint16_t)num;
//     }
//     return len;
//   }

//   return 0;
// }

// // ================== Config storage ==================
// void loadConfig() {
//   prefs.begin(NVS_NS, true);
//   g_wifiSsid = prefs.getString("ssid", "");
//   g_wifiPass = prefs.getString("pass", "");
//   g_userId   = prefs.getString("uid", "");
//   prefs.end();

//   g_wifiSsid.trim();
//   g_wifiPass.trim();
//   g_userId.trim();
// }

// void saveConfig(const String& ssid, const String& pass, const String& uid) {
//   prefs.begin(NVS_NS, false);
//   prefs.putString("ssid", ssid);
//   prefs.putString("pass", pass);
//   prefs.putString("uid",  uid);
//   prefs.end();
// }

// void clearConfig() {
//   prefs.begin(NVS_NS, false);
//   prefs.clear();
//   prefs.end();
// }

// // ================== AP Web UI ==================
// String pageIndex() {
//   String ssid = htmlEscape(g_wifiSsid);
//   String uid  = htmlEscape(g_userId);

//   String html;
//   html += "<!doctype html><html><head><meta charset='utf-8'/>";
//   html += "<meta name='viewport' content='width=device-width,initial-scale=1'/>";
//   html += "<title>ESP32 IR Config</title>";
//   html += "<style>";
//   html += "body{font-family:Arial,Helvetica,sans-serif;background:#0b1220;color:#e7e7e7;margin:0;padding:18px;}";
//   html += ".card{max-width:520px;margin:0 auto;background:#111a2e;border:1px solid #223155;border-radius:12px;padding:16px;}";
//   html += "h2{margin:0 0 10px 0;font-size:20px;}";
//   html += "label{display:block;margin-top:10px;font-size:13px;color:#b9c7ff;}";
//   html += "input{width:100%;padding:10px;margin-top:6px;border-radius:10px;border:1px solid #2a3a63;background:#0b1220;color:#fff;}";
//   html += "button{margin-top:14px;width:100%;padding:11px;border-radius:10px;border:0;background:#2b6cff;color:#fff;font-weight:700;}";
//   html += ".hint{margin-top:12px;font-size:12px;color:#a7b6e6;line-height:1.4;}";
//   html += "</style></head><body>";
//   html += "<div class='card'>";
//   html += "<h2>ESP32 IR - WiFi/MQTT Config</h2>";
//   html += "<form method='POST' action='/save'>";
//   html += "<label>WiFi SSID</label>";
//   html += "<input name='wifi_ssid' placeholder='Your WiFi name' value='" + ssid + "' required>";
//   html += "<label>WiFi Password</label>";
//   html += "<input name='wifi_pass' type='password' placeholder='Your WiFi password' value=''>";
//   html += "<label>User ID (để subscribe /{userid}/ir_commands/#)</label>";
//   html += "<input name='userid' placeholder='vd: user123' value='" + uid + "' required>";
//   html += "<button type='submit'>Save & Reboot</button>";
//   html += "</form>";
//   html += "<div class='hint'>";
//   html += "AP SSID: <b>ESP32_IR_Config</b> | PASS: <b>12345678</b><br/>";
//   html += "Giữ nút BOOT (GPIO0) ~5 giây để reset cấu hình về mặc định.";
//   html += "</div>";
//   html += "</div></body></html>";
//   return html;
// }

// void handleRoot() {
//   server.send(200, "text/html", pageIndex());
// }

// void handleSave() {
//   String ssid = server.arg("wifi_ssid");
//   String pass = server.arg("wifi_pass");  // có thể để trống nếu mạng không pass
//   String uid  = server.arg("userid");

//   ssid.trim(); pass.trim(); uid.trim();

//   if (ssid.length() == 0 || uid.length() == 0) {
//     server.send(400, "text/plain", "Missing wifi_ssid or userid");
//     return;
//   }

//   saveConfig(ssid, pass, uid);

//   server.send(200, "text/html",
//               "<html><body style='font-family:Arial'>"
//               "<h3>Saved! Rebooting...</h3>"
//               "</body></html>");
//   delay(800);
//   ESP.restart();
// }

// void startConfigPortal() {
//   g_inConfigMode = true;

//   WiFi.mode(WIFI_AP);
//   WiFi.softAPConfig(apIP, apGW, apMASK);
//   WiFi.softAP(AP_SSID, AP_PASS);

//   server.on("/", HTTP_GET, handleRoot);
//   server.on("/save", HTTP_POST, handleSave);
//   server.begin();

//   Serial.println("\n[AP CONFIG MODE]");
//   Serial.print("AP SSID: "); Serial.println(AP_SSID);
//   Serial.print("AP PASS: "); Serial.println(AP_PASS);
//   Serial.print("Open: http://"); Serial.println(WiFi.softAPIP());
// }

// // ================== WiFi STA ==================
// bool connectWiFiSTA(uint32_t timeoutMs = 20000) {
//   WiFi.mode(WIFI_STA);
//   WiFi.begin(g_wifiSsid.c_str(), g_wifiPass.c_str());

//   Serial.print("Connecting WiFi");
//   uint32_t start = millis();
//   while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeoutMs) {
//     delay(300);
//     Serial.print(".");
//   }
//   Serial.println();

//   if (WiFi.status() == WL_CONNECTED) {
//     Serial.print("WiFi connected. IP: ");
//     Serial.println(WiFi.localIP());
//     return true;
//   }
//   Serial.println("WiFi connect failed.");
//   return false;
// }

// // ================== MQTT ==================
// void mqttReconnect() {
//   if (client.connected()) return;

//   while (!client.connected()) {
//     Serial.print("MQTT connecting... ");

//     // Client ID random để tránh trùng
//     String cid = "ESP32IR-" + String((uint32_t)ESP.getEfuseMac(), HEX);

//     if (client.connect(cid.c_str(), mqtt_user, mqtt_password)) {
//       Serial.println("connected.");

//       // subscribe theo userid
//       g_topicSub = "/" + g_userId + "/ir_commands/#";
//       Serial.print("Subscribing: ");
//       Serial.println(g_topicSub);

//       client.subscribe(g_topicSub.c_str(), 1);
//     } else {
//       Serial.print("failed, rc=");
//       Serial.print(client.state());
//       Serial.println(" retry in 2s");
//       delay(2000);
//     }
//   }
// }

// // ================== IR send logic ==================
// void sendIRFromJson(JsonDocument& doc) {
//   // required fields by your spec:
//   // protocol, (rawData & frequency) OR (data & bits)
//   String protocol = doc["protocol"] | "";
//   protocol.trim();

//   if (protocol.length() == 0) {
//     Serial.println("JSON missing protocol");
//     return;
//   }

//   String protoLower = protocol;
//   protoLower.toLowerCase();

//   if (protoLower == "raw") {
//     // rawData + frequency
//     int frequency = doc["frequency"] | 38000; // default 38kHz if missing
//     JsonVariant rawVar = doc["rawData"];

//     size_t len = parseRawData(rawVar);
//     if (len == 0) {
//       Serial.println("RAW: rawData empty/invalid");
//       return;
//     }

//     // IRremoteESP8266 sendRaw expects kHz
//     uint16_t khz = 38;
//     if (frequency > 1000) khz = (uint16_t)(frequency / 1000); // 38000 -> 38
//     else khz = (uint16_t)frequency; // nếu user gửi 38 luôn

//     Serial.printf("Send RAW: len=%u, freq=%u kHz\n", (unsigned)len, (unsigned)khz);
//     irsend.sendRaw(irRawBuf, (uint16_t)len, khz);
//     return;
//   }

//   // Protocol others: data(hex) + bits
//   String dataStr = doc["data"] | "";
//   int bits = doc["bits"] | 0;
//   dataStr.trim();

//   if (dataStr.length() == 0 || bits <= 0) {
//     Serial.println("Non-RAW: missing data or bits");
//     return;
//   }

//   uint64_t dataVal = parseHexToU64(dataStr);

//   Serial.printf("Send %s: data=0x%llX bits=%d\n", protocol.c_str(),
//                 (unsigned long long)dataVal, bits);

//   if (protoLower == "nec") {
//     irsend.sendNEC(dataVal, bits);
//   } else if (protoLower == "sony") {
//     irsend.sendSony(dataVal, bits);
//   } else if (protoLower == "samsung") {
//     irsend.sendSamsung(dataVal, bits);
//   } else {
//     Serial.println("Unsupported protocol (only raw/Sony/Samsung/NEC).");
//   }
// }

// // MQTT callback
// void callback(char* topic, byte* payload, unsigned int length) {
//   Serial.print("MQTT msg topic: ");
//   Serial.println(topic);

//   // payload -> String
//   String msg;
//   msg.reserve(length + 1);
//   for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

//   // JSON parse: dùng doc lớn vì rawData dài
//   // Nếu rawData là string dài, cần doc lớn hơn.
//   StaticJsonDocument<8192> doc;
//   DeserializationError err = deserializeJson(doc, msg);

//   if (err) {
//     Serial.print("JSON parse error: ");
//     Serial.println(err.c_str());
//     return;
//   }

//   // chỉ dùng các field bạn cần (nhưng doc có thể chứa thêm vẫn ok)
//   sendIRFromJson(doc);
// }

// // ================== Reset button long press ==================
// void handleBootHoldReset() {
//   static bool lastPressed = false;
//   static uint32_t pressStart = 0;

//   bool pressed = (digitalRead(BOOT_BTN_PIN) == LOW); // BOOT thường kéo xuống GND khi nhấn

//   if (pressed && !lastPressed) {
//     pressStart = millis();
//     lastPressed = true;
//   } else if (!pressed && lastPressed) {
//     lastPressed = false;
//   } else if (pressed && lastPressed) {
//     if (millis() - pressStart >= HOLD_RESET_MS) {
//       Serial.println("BOOT hold detected -> clearing config & rebooting...");
//       clearConfig();
//       delay(400);
//       ESP.restart();
//     }
//   }
// }

// // ================== Setup / Loop ==================
// void setup() {
//   Serial.begin(115200);
//   delay(200);

//   pinMode(BOOT_BTN_PIN, INPUT_PULLUP);

//   irsend.begin();

//   loadConfig();

//   // Nếu chưa có config -> vào AP config portal
//   if (g_wifiSsid.length() == 0 || g_userId.length() == 0) {
//     startConfigPortal();
//     return;
//   }

//   // Có config -> thử connect WiFi STA, fail thì fallback AP portal
//   if (!connectWiFiSTA(20000)) {
//     startConfigPortal();
//     return;
//   }

//   // MQTT TLS insecure (như bản cũ)
//   espClient.setInsecure();

//   client.setServer(mqtt_server, mqtt_port);
//   client.setCallback(callback);
//   client.setBufferSize(MQTT_MAX_PACKET_SIZE);

//   mqttReconnect();
// }

// void loop() {
//   handleBootHoldReset();

//   if (g_inConfigMode) {
//     server.handleClient();
//     delay(2);
//     return;
//   }

//   if (!client.connected()) {
//     mqttReconnect();
//   }
//   client.loop();
// }
