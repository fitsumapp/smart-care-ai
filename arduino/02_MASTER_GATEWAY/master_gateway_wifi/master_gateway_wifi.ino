// ================================================================
//  MedPulse Smart-Care AI — ESP32 Master Station Gateway (Cloud Wi-Fi Mode)
//  Hardware: ESP32 DevKit V1 (30-pin)
//  Peripherals:
//    1. LoRa E32-900T20D / E32-TTL-100 (Serial2: GPIO 16 RX, GPIO 17 TX)
//    2. MFRC522 RFID / NFC Reader (VSPI: SCK=18, MOSI=23, MISO=19, CS=5, RST=22)
//    3. GC9A01A 240x240 Round TFT Display (VSPI: SCK=18, MOSI=23, CS=15, DC=4, RST=2)
//    4. Built-in Phone Captive Portal (Setup Wi-Fi & Server URL from phone!)
//  Company: ACRMA TECH SOLUTION PLC
// ================================================================

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>

// ── Pin Definitions ────────────────────────────────────────────
// LoRa E32 Pins (Hardware Serial2 on ESP32)
#define LORA_RX_PIN 16  // ESP32 GPIO16 (RX2) -> LoRa TX
#define LORA_TX_PIN 17  // ESP32 GPIO17 (TX2) -> LoRa RX

// NFC (MFRC522) Pins
#define NFC_SS_PIN 5    // Chip Select for NFC
#define NFC_RST_PIN 22  // Reset for NFC

// Round TFT (GC9A01A) Pins
#define TFT_CS 15  // Chip Select for TFT
#define TFT_DC 4   // Data/Command for TFT
#define TFT_RST 14  // Reset for TFT (GPIO 14 is clean and reliable)
#define TFT_MOSI 23 // SDA (MOSI)
#define TFT_SCLK 18 // SCL (Clock)

// Hardware SPI Pins (VSPI default on ESP32):
// SCK  = GPIO 18 (Shared between TFT and NFC)
// MOSI = GPIO 23 (Shared between TFT and NFC)
// MISO = GPIO 19 (NFC MISO only)

// ── Color Palette (16-bit RGB565) ──────────────────────────────
#define C_VOID 0x0000
#define C_SURFACE 0x0842
#define C_CYAN 0x07FF
#define C_CYAN_DIM 0x0398
#define C_GREEN 0x3FE6
#define C_RED 0xF185
#define C_WHITE 0xFFFF
#define C_GRAY_MID 0x7BEF
#define C_BLUE_BG 0x0113

// ── Hardware Instances ─────────────────────────────────────────
HardwareSerial loraSerial(2);  // Serial2
MFRC522 rfid(NFC_SS_PIN, NFC_RST_PIN);
Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_RST);
Preferences prefs;
WebServer webServer(80);
DNSServer dnsServer;

// ── Configuration State ────────────────────────────────────────
String wifiSSID = "";
String wifiPassword = "";
String serverUrl = "https://smartcareai.acrmatech.com";
String stationId = "Nurse Station 1";
const String STATION_KEY = "medpulse-station-secret-key";

bool isConfigMode = false;
bool showingMessage = false;
unsigned long messageTimer = 0;
unsigned long pulseTimer = 0;
uint8_t pulseStep = 0;
unsigned long lastResetCheck = 0;
unsigned long lastHeartbeat = 0;
unsigned long lastWifiCheck = 0;

// ── Function Declarations ──────────────────────────────────────
void loadConfiguration();
void saveConfiguration(const String &s, const String &p, const String &u, const String &id);
void startConfigPortal();
void handlePortalRoot();
void handlePortalSave();
void drawBootSplash();
void showConfigScreen();
void showIdleScreen();
void animatePulseRing();
void showCallScreen(const String &room, const String &bed);
void showGrantedScreen(const String &name);
void showDeniedScreen();
void centerText(const char *text, int16_t y, uint8_t size, uint16_t color);

void sendCallToCloud(const String &room, const String &bed, const String &action);
void sendNfcToCloud(const String &uid);
void checkCloudResets();
void sendHeartbeat();
void ensureWifiConnected();

// ================================================================
//  SETUP
// ================================================================
void setup() {
  Serial.begin(115200);
  loraSerial.begin(9600, SERIAL_8N1, LORA_RX_PIN, LORA_TX_PIN);

  Serial.println(F("\n=================================================="));
  Serial.println(F("  MedPulse Smart-Care AI — Master Gateway (Wi-Fi)  "));
  Serial.println(F("=================================================="));

  // Initialize Chip Selects (Keep both unselected initially)
  pinMode(NFC_SS_PIN, OUTPUT);
  digitalWrite(NFC_SS_PIN, HIGH);
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);

  // Initialize Hardware SPI
  SPI.begin(18, 19, 23, -1);

  // 1. Init RFID (Hardware SPI)
  rfid.PCD_Init();
  delay(50);
  rfid.PCD_SetAntennaGain(MFRC522::RxGain_max);
  byte nfcVer = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  Serial.printf("[NFC HARDWARE] MFRC522 Chip Version: 0x%02X\n", nfcVer);

  // Deselect RFID before TFT operations
  digitalWrite(NFC_SS_PIN, HIGH);

  // 2. Init TFT (Hardware SPI at 24 MHz)
  tft.begin(24000000);
  tft.setRotation(0);
  tft.fillScreen(C_VOID);
  drawBootSplash();

  // Load saved credentials from Flash
  loadConfiguration();

  // If BOOT button (GPIO 0) held during startup, force phone configuration portal
  pinMode(0, INPUT_PULLUP);
  bool forceConfig = (digitalRead(0) == LOW);

  if (wifiSSID.length() == 0 || forceConfig) {
    Serial.println(F("[SYSTEM] No Wi-Fi configured or BOOT button pressed. Starting Phone Setup Portal..."));
    startConfigPortal();
    return;
  }

  // Connect to saved Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
  Serial.printf("[Wi-Fi] Connecting to: %s\n", wifiSSID.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 24) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[Wi-Fi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println(F("\n[Wi-Fi] Failed to connect. Switching to Phone Setup Portal."));
    startConfigPortal();
    return;
  }

  delay(600);
  showIdleScreen();
  Serial.println(F("[SYSTEM] Master Gateway Active & Online."));
}

// ================================================================
//  MAIN LOOP
// ================================================================
void loop() {
  // If in Phone Setup Portal mode, handle DNS and Web requests
  if (isConfigMode) {
    dnsServer.processNextRequest();
    webServer.handleClient();
    return;
  }

  unsigned long now = millis();

  // 1. Maintain Wi-Fi Connection
  if (now - lastWifiCheck > 10000) {
    ensureWifiConnected();
    lastWifiCheck = now;
  }

  // 2. Listen for LoRa Packets from Patient Rooms
  if (loraSerial.available() > 0) {
    String incoming = loraSerial.readStringUntil('\n');
    incoming.trim();

    if (incoming.length() > 0) {
      Serial.printf("[LORA IN] %s\n", incoming.c_str());

      if (incoming.startsWith("START:")) {
        int firstColon = incoming.indexOf(':');
        int secondColon = incoming.indexOf(':', firstColon + 1);

        String room = "";
        String bed = "General";

        if (secondColon != -1) {
          room = incoming.substring(firstColon + 1, secondColon);
          bed = incoming.substring(secondColon + 1);
        } else {
          room = incoming.substring(firstColon + 1);
        }
        room.trim();
        bed.trim();

        showCallScreen(room, bed);
        sendCallToCloud(room, bed, "start");
      } else if (incoming.startsWith("ARRIVED:")) {
        String room = incoming.substring(8);
        room.trim();
        sendCallToCloud(room, "General", "arrived");
      }
    }
  }

  // 3. Listen for Nurse NFC Card Scan (MFRC522)
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(NFC_SS_PIN, HIGH);

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String uid = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      if (rfid.uid.uidByte[i] < 0x10) uid += "0";
      uid += String(rfid.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();
    Serial.printf("[NFC SCAN] Card UID: %s\n", uid.c_str());

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    sendNfcToCloud(uid);
  }

  // 4. Poll Cloud API for Cleared/Reset Calls (every 5 seconds)
  if (now - lastResetCheck > 5000) {
    checkCloudResets();
    lastResetCheck = now;
  }

  // 5. Send Heartbeat to Cloud API (every 12 seconds)
  if (now - lastHeartbeat > 12000) {
    sendHeartbeat();
    lastHeartbeat = now;
  }

  // 6. Built-in Hardware Test: Press BOOT button (GPIO 0) to simulate a Patient Call
  static unsigned long lastBtnPress = 0;
  if (digitalRead(0) == LOW && (now - lastBtnPress > 4000)) {
    lastBtnPress = now;
    Serial.println(F("[TEST TRIGGER] BOOT button pressed! Dispatching Test Call..."));
    showCallScreen("1", "Bed 1");
    sendCallToCloud("1", "Bed 1", "start");
  }

  // 7. TFT Animation & Idle Screen Timeout
  if (showingMessage && (now - messageTimer > 3500)) {
    showIdleScreen();
    showingMessage = false;
  }

  if (!showingMessage && (now - pulseTimer > 80)) {
    animatePulseRing();
    pulseTimer = now;
  }
}

// ================================================================
//  CONFIGURATION & PHONE CAPTIVE PORTAL
// ================================================================

void loadConfiguration() {
  prefs.begin("smartcare", true);
  wifiSSID = prefs.getString("ssid", "");
  wifiPassword = prefs.getString("pass", "");
  serverUrl = prefs.getString("server_url", "https://smartcareai.acrmatech.com");
  stationId = prefs.getString("station_id", "Nurse Station 1");
  prefs.end();
}

void saveConfiguration(const String &s, const String &p, const String &u, const String &id) {
  prefs.begin("smartcare", false);
  prefs.putString("ssid", s);
  prefs.putString("pass", p);
  prefs.putString("server_url", u);
  prefs.putString("station_id", id);
  prefs.end();
}

void startConfigPortal() {
  isConfigMode = true;
  WiFi.mode(WIFI_AP);
  WiFi.softAP("SmartCare-Master-Config", "12345678");

  IPAddress apIP = WiFi.softAPIP();
  Serial.printf("[PORTAL] Hotspot started: SmartCare-Master-Config | IP: %s\n", apIP.toString().c_str());

  dnsServer.start(53, "*", apIP);

  webServer.on("/", handlePortalRoot);
  webServer.on("/save", HTTP_POST, handlePortalSave);
  webServer.onNotFound(handlePortalRoot);
  webServer.begin();

  showConfigScreen();
}

void handlePortalRoot() {
  // Scan Wi-Fi networks
  int n = WiFi.scanNetworks();
  String options = "";
  for (int i = 0; i < n; ++i) {
    options += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + " (" + String(WiFi.RSSI(i)) + " dBm)</option>";
  }

  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>SmartCare Gateway Setup</title><style>";
  html += "body{font-family:sans-serif;background:#0f172a;color:#f8fafc;margin:0;padding:20px;}";
  html += ".card{background:#1e293b;border-radius:16px;padding:24px;max-width:400px;margin:auto;box-shadow:0 8px 24px rgba(0,0,0,0.4);}";
  html += "h2{color:#38bdf8;margin-top:0;}label{font-size:13px;color:#94a3b8;display:block;margin-top:14px;}";
  html += "input,select{width:100%;padding:10px;border-radius:8px;border:1px solid #334155;background:#0f172a;color:#fff;box-sizing:border-box;margin-top:4px;}";
  html += "button{width:100%;margin-top:24px;padding:12px;border:none;border-radius:8px;background:#0284c7;color:#fff;font-size:16px;font-weight:bold;cursor:pointer;}";
  html += "</style></head><body><div class='card'>";
  html += "<h2>🏥 Master Gateway</h2>";
  html += "<p style='font-size:13px;color:#94a3b8;'>Configure hospital Wi-Fi and cloud connection.</p>";
  html += "<form action='/save' method='POST'>";
  html += "<label>Select Hospital Wi-Fi:</label><select name='ssid' id='ssidSelect'>" + options + "</select>";
  html += "<label>Or Type Wi-Fi Name:</label><input type='text' name='custom_ssid' placeholder='SSID if hidden'>";
  html += "<label>Wi-Fi Password:</label><input type='password' name='pass' required>";
  html += "<label>Cloud Server URL:</label><input type='text' name='server_url' value='" + serverUrl + "' required>";
  html += "<label>Station Identifier:</label><input type='text' name='station_id' value='" + stationId + "' required>";
  html += "<button type='submit'>Save & Connect Gateway</button>";
  html += "</form></div></body></html>";

  webServer.send(200, "text/html", html);
}

void handlePortalSave() {
  String s = webServer.arg("custom_ssid");
  if (s.length() == 0) s = webServer.arg("ssid");
  String p = webServer.arg("pass");
  String u = webServer.arg("server_url");
  String id = webServer.arg("station_id");

  u.trim();
  while (u.endsWith("/")) u = u.substring(0, u.length() - 1);

  saveConfiguration(s, p, u, id);

  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<style>body{font-family:sans-serif;background:#0f172a;color:#fff;text-align:center;padding:50px;}</style></head><body>";
  html += "<h2>✅ Saved Successfully!</h2><p>Gateway is connecting to Wi-Fi. You can disconnect now.</p>";
  html += "</body></html>";
  webServer.send(200, "text/html", html);

  delay(1200);
  ESP.restart();
}

// ================================================================
//  HTTP CLOUD DISPATCHERS
// ================================================================

void ensureWifiConnected() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[Wi-Fi] Reconnecting..."));
    WiFi.disconnect();
    WiFi.reconnect();
  }
}

void sendCallToCloud(const String &room, const String &bed, const String &action) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[HTTP ERROR] Cannot send: Wi-Fi disconnected"));
    return;
  }

  HTTPClient http;
  String url = serverUrl + "/api/calls/";
  http.begin(url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.addHeader("X-Station-ID", stationId);
  http.addHeader("X-Station-API-Key", STATION_KEY);
  http.setTimeout(4000);

  String postData = "room_number=" + room + "&bed_number=" + bed + "&action=" + action;
  int code = http.POST(postData);

  if (code > 0) {
    Serial.printf("[CLOUD] Call Dispatched: Room %s (%s) -> HTTP %d\n", room.c_str(), bed.c_str(), code);
  } else {
    Serial.printf("[CLOUD ERROR] %s\n", http.errorToString(code).c_str());
  }
  http.end();
}

void sendNfcToCloud(const String &uid) {
  if (WiFi.status() != WL_CONNECTED) {
    showDeniedScreen();
    return;
  }

  HTTPClient http;
  String url = serverUrl + "/api/acknowledge_nfc/";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Station-ID", stationId);
  http.addHeader("X-Station-API-Key", STATION_KEY);
  http.setTimeout(4000);

  String payload = "{\"uid\":\"" + uid + "\",\"station_name\":\"" + stationId + "\"}";
  int code = http.POST(payload);

  if (code == 200) {
    String res = http.getString();
    Serial.printf("[CLOUD NFC RESP] %s\n", res.c_str());

    if (res.indexOf("\"status\":\"success\"") != -1) {
      String nurseName = "Nurse";
      int nameIdx = res.indexOf("\"nurse_name\":\"");
      if (nameIdx != -1) {
        int start = nameIdx + 14;
        int end = res.indexOf("\"", start);
        if (end != -1) nurseName = res.substring(start, end);
      }
      showGrantedScreen(nurseName);
    } else {
      showDeniedScreen();
    }
  } else {
    showDeniedScreen();
  }
  http.end();
}

void checkCloudResets() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = serverUrl + "/api/calls/check_reset/";
  http.begin(url);
  http.addHeader("X-Station-ID", stationId);
  http.addHeader("X-Station-API-Key", STATION_KEY);
  http.setTimeout(2500);

  int code = http.GET();
  if (code == 200) {
    String res = http.getString();
    if (res.indexOf("\"resets\":[") != -1 && res.indexOf("[]") == -1) {
      int start = res.indexOf("\"resets\":[") + 10;
      int end = res.indexOf("]", start);
      if (end != -1) {
        String arrayContent = res.substring(start, end);
        int from = 0;
        while (from < arrayContent.length()) {
          int q1 = arrayContent.indexOf("\"", from);
          if (q1 == -1) break;
          int q2 = arrayContent.indexOf("\"", q1 + 1);
          if (q2 == -1) break;
          String resetRoom = arrayContent.substring(q1 + 1, q2);
          resetRoom.trim();
          if (resetRoom.length() > 0) {
            loraSerial.println("DONE:" + resetRoom);
            Serial.printf("[LORA RESET ➜] DONE:%s\n", resetRoom.c_str());
          }
          from = q2 + 1;
        }
      }
    }
  }
  http.end();
}

void sendHeartbeat() {
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  http.begin(serverUrl + "/api/heartbeat/");
  http.addHeader("X-Station-ID", stationId);
  http.setTimeout(2000);
  http.GET();
  http.end();
}

// ================================================================
//  GC9A01A ROUND TFT UI RENDERING
// ================================================================

void centerText(const char *text, int16_t y, uint8_t size, uint16_t color) {
  int16_t x1, y1;
  uint16_t w, h;
  tft.setTextSize(size);
  tft.setTextColor(color);
  tft.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
  tft.setCursor((240 - w) / 2, y);
  tft.print(text);
}

void drawBootSplash() {
  tft.fillScreen(C_VOID);
  tft.drawCircle(120, 120, 115, C_CYAN_DIM);
  tft.drawCircle(120, 120, 113, C_CYAN);
  centerText("MEDPULSE", 75, 2, C_CYAN);
  centerText("SMART-CARE AI", 100, 1, C_WHITE);
  centerText("MASTER GATEWAY", 125, 1, C_GRAY_MID);
  centerText("Booting System...", 160, 1, C_GREEN);
}

void showConfigScreen() {
  tft.fillScreen(C_VOID);
  tft.drawCircle(120, 120, 115, C_CYAN);
  centerText("HOTSPOT ACTIVE", 45, 1, C_CYAN);
  centerText("SETUP VIA PHONE", 70, 2, C_WHITE);
  centerText("Wi-Fi Network:", 115, 1, C_GRAY_MID);
  centerText("SmartCare-Master-Config", 135, 1, C_GREEN);
  centerText("Password: 12345678", 155, 1, C_WHITE);
  centerText("IP: 192.168.4.1", 185, 1, C_CYAN_DIM);
}

void showIdleScreen() {
  tft.fillScreen(C_VOID);
  tft.drawCircle(120, 120, 115, C_SURFACE);
  centerText("MEDPULSE AI", 55, 1, C_CYAN_DIM);

  if (WiFi.status() == WL_CONNECTED) {
    centerText("Cloud: Online", 75, 1, C_GREEN);
  } else {
    centerText("Cloud: Offline", 75, 1, C_RED);
  }

  centerText("SCAN NFC", 110, 2, C_WHITE);
  centerText("TAP BADGE TO ACK", 140, 1, C_GRAY_MID);
  centerText(stationId.c_str(), 180, 1, C_CYAN_DIM);
}

void animatePulseRing() {
  int r = 100 + (pulseStep % 10);
  tft.drawCircle(120, 120, r, (pulseStep % 2 == 0) ? C_CYAN_DIM : C_VOID);
  pulseStep++;
}

void showCallScreen(const String &room, const String &bed) {
  showingMessage = true;
  messageTimer = millis();

  tft.fillScreen(C_VOID);
  tft.fillCircle(120, 120, 115, C_RED);
  tft.fillCircle(120, 120, 108, C_VOID);

  centerText("PATIENT CALL", 50, 1, C_RED);
  String rStr = "ROOM " + room;
  centerText(rStr.c_str(), 95, 3, C_WHITE);
  String bStr = "(" + bed + ")";
  centerText(bStr.c_str(), 145, 2, C_CYAN);
  centerText("TRANSMITTED TO CLOUD", 185, 1, C_GRAY_MID);
}

void showGrantedScreen(const String &name) {
  showingMessage = true;
  messageTimer = millis();

  tft.fillScreen(C_VOID);
  tft.fillCircle(120, 120, 115, C_GREEN);
  tft.fillCircle(120, 120, 108, C_VOID);

  centerText("ACCESS GRANTED", 50, 1, C_GREEN);
  centerText(name.c_str(), 105, 2, C_WHITE);
  centerText("CALL CLEARED", 145, 2, C_GREEN);
  centerText("STATION SYNCD", 185, 1, C_CYAN_DIM);
}

void showDeniedScreen() {
  showingMessage = true;
  messageTimer = millis();

  tft.fillScreen(C_VOID);
  tft.fillCircle(120, 120, 115, C_RED);
  tft.fillCircle(120, 120, 108, C_VOID);

  centerText("UNRECOGNIZED", 65, 1, C_RED);
  centerText("CARD DENIED", 110, 2, C_WHITE);
  centerText("NOT REGISTERED", 155, 1, C_GRAY_MID);
}
