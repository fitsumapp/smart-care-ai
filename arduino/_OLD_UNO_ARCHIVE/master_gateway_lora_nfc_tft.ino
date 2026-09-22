// ================================================================
//  MedPulse Smart-Care AI — Master Station Gateway Firmware
//  Hardware: 1 Arduino (Uno/Nano/Mega/ESP32)
//  Modules:
//    1. LoRa E32-TTL-100/E32-900T20D (Pins 2, 3 - SoftwareSerial)
//    2. MFRC522 RFID / NFC Reader (Hardware SPI: Pins 11, 12, 13 + SS=10, RST=9)
//    3. GC9A01A 240x240 Round TFT Display (Hardware SPI: Pins 11, 13 + CS=7, DC=6, RST=8)
//  Company: ACRMA TECH SOLUTION PLC
// ================================================================

#include <SPI.h>
#include <SoftwareSerial.h>
#include <MFRC522.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>

// ── Pin Definitions ────────────────────────────────────────────
// LoRa E32 Pins (SoftwareSerial)
#define LORA_RX_PIN  2   // Arduino Pin 2 connects to E32 TX
#define LORA_TX_PIN  3   // Arduino Pin 3 connects to E32 RX

// NFC (MFRC522) Pins
#define NFC_SS_PIN   10  // Chip Select for NFC
#define NFC_RST_PIN  9   // Reset for NFC

// Round TFT (GC9A01A) Pins
#define TFT_CS       7   // Chip Select for TFT
#define TFT_DC       6   // Data/Command for TFT
#define TFT_RST      8   // Reset for TFT
// Note: Hardware SPI Pins: MOSI=Pin 11, MISO=Pin 12 (NFC), SCK=Pin 13

// ── Station Identity ───────────────────────────────────────────
const char STATION_ID[] = "STA-01 (MASTER)";

// ── Hardware Instances ─────────────────────────────────────────
SoftwareSerial   loraSerial(LORA_RX_PIN, LORA_TX_PIN);
MFRC522          rfid(NFC_SS_PIN, NFC_RST_PIN);
Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_RST);

// ── Color Palette (16-bit RGB565) ──────────────────────────────
#define C_VOID       0x0000   // Pure black
#define C_SURFACE    0x0842   // Deep charcoal
#define C_CYAN       0x07FF   // Electric cyan
#define C_CYAN_DIM   0x0398   // Muted cyan
#define C_CYAN_DARK  0x0190   // Darkest cyan
#define C_GREEN      0x3FE6   // Mint green
#define C_GREEN_DIM  0x0380   // Dim green
#define C_RED        0xF185   // Coral red
#define C_RED_DIM    0x6000   // Dark red
#define C_WHITE      0xFFFF
#define C_GRAY_HI    0xCE59
#define C_GRAY_MID   0x7BEF
#define C_GRAY_LO    0x39E7

// ── UI State Variables ─────────────────────────────────────────
unsigned long messageTimer   = 0;
bool          showingMessage = false;
unsigned long pulseTimer     = 0;
uint8_t       pulseStep      = 0;

// ── Function Declarations ──────────────────────────────────────
void drawBootSplash();
void showIdleScreen();
void animatePulseRing();
void showReadingScreen();
void showGrantedScreen(const String &name);
void showDeniedScreen();
void centerText(const char* text, int16_t y, uint8_t size, uint16_t color);

void setup() {
  Serial.begin(9600);
  loraSerial.begin(9600);

  Serial.setTimeout(50);
  loraSerial.setTimeout(50);

  // Initialize SPI Bus
  SPI.begin();

  // Initialize NFC
  pinMode(NFC_SS_PIN, OUTPUT);
  digitalWrite(NFC_SS_PIN, HIGH); // Deselect NFC
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);     // Deselect TFT

  // Init RFID
  rfid.PCD_Init();

  // Init TFT Display
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(C_VOID);

  drawBootSplash();
  delay(1200);

  showIdleScreen();
  Serial.println(F("[SYSTEM] Master Gateway Ready (LoRa + NFC + Round TFT)"));
}

void loop() {
  unsigned long now = millis();

  // ─────────────────────────────────────────────────────────────
  // 1. INBOUND SERIAL FROM PC (Python Bridge Commands)
  // ─────────────────────────────────────────────────────────────
  if (Serial.available() > 0) {
    String pcCmd = Serial.readStringUntil('\n');
    pcCmd.trim();

    if (pcCmd.length() > 0) {
      // Command A: RESET signal to send back to patient room over LoRa
      if (pcCmd.startsWith("RESET:")) {
        String resetRoom = pcCmd.substring(6);
        resetRoom.trim();
        loraSerial.println("DONE:" + resetRoom);
        Serial.println("ACK:Room_" + resetRoom + "_Reset");
      }
      // Command B: TFT Access Granted (Nurse Name)
      else if (pcCmd.startsWith("TFT_SUCCESS:")) {
        String nurseName = pcCmd.substring(12);
        nurseName.trim();
        showGrantedScreen(nurseName);
      }
      // Command C: TFT Access Denied
      else if (pcCmd.startsWith("TFT_ERROR")) {
        showDeniedScreen();
      }
    }
  }

  // ─────────────────────────────────────────────────────────────
  // 2. INBOUND LORA SIGNALS FROM PATIENT ROOMS
  // ─────────────────────────────────────────────────────────────
  if (loraSerial.available() > 0) {
    String incoming = loraSerial.readStringUntil('\n');
    incoming.trim();

    if (incoming.length() > 0) {
      // Forward raw signal directly to PC via USB Serial
      // e.g. "START:10:Bed 1" or "ARRIVED:10"
      Serial.println(incoming);
    }
  }

  // ─────────────────────────────────────────────────────────────
  // 3. TFT UI SCREEN TIMEOUT & IDLE ANIMATION
  // ─────────────────────────────────────────────────────────────
  if (showingMessage && (now - messageTimer > 3500)) {
    showIdleScreen();
    showingMessage = false;
  }

  if (!showingMessage && (now - pulseTimer > 80)) {
    animatePulseRing();
    pulseTimer = now;
  }

  // ─────────────────────────────────────────────────────────────
  // 4. NFC CARD SCANNING (MFRC522)
  // ─────────────────────────────────────────────────────────────
  // Select NFC on SPI Bus
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(NFC_SS_PIN, LOW);

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String uid = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      if (rfid.uid.uidByte[i] < 0x10) uid += "0";
      uid += String(rfid.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();

    // Deselect NFC
    digitalWrite(NFC_SS_PIN, HIGH);

    // Send Scan to PC Python Bridge
    Serial.print("SCAN:");
    Serial.println(uid);

    showReadingScreen();
    delay(800);
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  } else {
    digitalWrite(NFC_SS_PIN, HIGH);
  }
}

// ══════════════════════════════════════════════════════════════
//  TFT SCREENS (GC9A01A Round Display UI)
// ══════════════════════════════════════════════════════════════

void drawBootSplash() {
  tft.fillScreen(C_VOID);
  tft.drawCircle(120, 120, 117, C_CYAN_DARK);
  tft.drawCircle(120, 120, 116, C_CYAN_DARK);

  // Medical Cross Icon
  tft.fillRoundRect(112, 88, 16, 64, 8, C_CYAN);
  tft.fillRoundRect(88, 112, 64, 16, 8, C_CYAN);

  centerText("MedPulse AI", 175, 2, C_WHITE);
  centerText("MASTER GATEWAY", 198, 1, C_GRAY_LO);
}

void showIdleScreen() {
  pulseStep = 0;
  tft.fillScreen(C_VOID);

  tft.drawCircle(120, 120, 117, C_CYAN_DARK);
  tft.drawCircle(120, 120, 116, C_CYAN_DARK);

  // Station Badge
  tft.fillRoundRect(60, 22, 120, 18, 9, C_SURFACE);
  tft.drawRoundRect(60, 22, 120, 18, 9, C_CYAN_DARK);
  centerText(STATION_ID, 27, 1, C_CYAN_DIM);

  centerText("MedPulse", 74, 2, C_WHITE);
  centerText("AI", 92, 2, C_CYAN);

  tft.drawFastHLine(72, 114, 96, C_CYAN_DARK);

  // NFC Target Rings
  tft.drawCircle(120, 145, 24, C_CYAN_DIM);
  tft.drawCircle(120, 145, 16, C_CYAN_DIM);
  tft.drawCircle(120, 145,  8, C_CYAN);
  tft.fillCircle(120, 145,  3, C_CYAN);

  centerText("TAP NURSE CARD", 186, 1, C_GRAY_MID);
  centerText("LORA GATEWAY ACTIVE", 198, 1, C_GRAY_LO);
}

void animatePulseRing() {
  static uint8_t lastStep = 255;
  uint8_t phase = pulseStep % 16;

  if (lastStep != 255) {
    uint8_t lastPhase = lastStep % 16;
    uint8_t lastR = 96 + lastPhase;
    tft.drawCircle(120, 120, lastR, C_VOID);
  }

  uint8_t r = 96 + phase;
  uint16_t col = (phase < 8) ? C_CYAN_DIM : C_CYAN_DARK;
  tft.drawCircle(120, 120, r, col);

  lastStep = pulseStep;
  pulseStep++;
}

void showReadingScreen() {
  tft.fillScreen(C_VOID);
  tft.drawCircle(120, 120, 117, C_CYAN_DARK);
  tft.drawCircle(120, 120, 116, C_CYAN_DARK);

  for (int a = 0; a < 360; a += 45) {
    tft.drawCircle(120, 120, 80, C_CYAN_DARK);
  }
  tft.drawCircle(120, 120, 80, C_CYAN);

  tft.drawCircle(120, 100, 22, C_CYAN);
  tft.drawCircle(120, 100, 14, C_CYAN_DIM);
  tft.fillCircle(120, 100,  5, C_CYAN);

  centerText("VERIFYING", 132, 2, C_WHITE);
  centerText("PLEASE WAIT", 152, 1, C_GRAY_MID);

  tft.fillCircle(103, 172, 4, C_CYAN);
  tft.fillCircle(120, 172, 4, C_CYAN_DIM);
  tft.fillCircle(137, 172, 4, C_CYAN_DARK);
}

void showGrantedScreen(const String &name) {
  tft.fillScreen(C_VOID);
  tft.drawCircle(120, 120, 117, C_GREEN_DIM);
  tft.drawCircle(120, 120, 116, C_GREEN_DIM);
  tft.fillCircle(120, 120, 90, 0x0841);

  // Green Check Badge
  tft.fillCircle(120, 78, 28, C_GREEN);
  for (int t = 0; t < 3; t++) {
    tft.drawLine(106+t, 78, 115+t, 88, C_VOID);
    tft.drawLine(115+t, 88, 132+t, 66, C_VOID);
  }

  centerText("VERIFIED", 120, 1, C_GREEN);
  tft.drawFastHLine(72, 132, 96, C_GREEN_DIM);

  centerText(name.c_str(), 148, 2, C_WHITE);

  tft.fillRoundRect(68, 172, 104, 20, 10, C_GREEN_DIM);
  tft.drawRoundRect(68, 172, 104, 20, 10, C_GREEN);
  centerText("CALL ACCEPTED", 178, 1, C_GREEN);

  messageTimer   = millis();
  showingMessage = true;
}

void showDeniedScreen() {
  tft.fillScreen(C_VOID);
  tft.drawCircle(120, 120, 117, C_RED_DIM);
  tft.drawCircle(120, 120, 116, C_RED_DIM);
  tft.fillCircle(120, 120, 90, 0x0841);

  // Red X Badge
  tft.fillCircle(120, 78, 28, C_RED);
  for (int t = 0; t < 3; t++) {
    tft.drawLine(108+t, 66, 134+t, 90, C_VOID);
    tft.drawLine(134+t, 66, 108+t, 90, C_VOID);
  }

  centerText("UNRECOGNISED", 120, 1, C_RED);
  tft.drawFastHLine(72, 132, 96, C_RED_DIM);

  centerText("CARD NOT FOUND", 150, 1, C_GRAY_MID);
  centerText("IN REGISTRY", 164, 1, C_GRAY_LO);

  tft.fillRoundRect(76, 180, 88, 20, 10, C_RED_DIM);
  tft.drawRoundRect(76, 180, 88, 20, 10, C_RED);
  centerText("DENIED", 186, 1, C_RED);

  messageTimer   = millis();
  showingMessage = true;
}

void centerText(const char* text, int16_t y, uint8_t size, uint16_t color) {
  int16_t  x1, y1;
  uint16_t w, h;
  tft.setTextSize(size);
  tft.setTextColor(color);
  tft.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
  int16_t x = (240 - (int16_t)w) / 2;
  if (x < 10) x = 10;
  tft.setCursor(x, y);
  tft.print(text);
}
