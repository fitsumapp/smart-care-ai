// ================================================================
//  MedPulse Smart-Care AI — Sub-Station NFC Terminal Firmware
//  Hardware: Arduino (Uno/Nano/ESP32)
//  Modules:
//    1. MFRC522 RFID / NFC Reader (Hardware SPI: Pins 11, 12, 13 + SS=10, RST=9)
//    2. GC9A01A 240x240 Round TFT Display (Hardware SPI: Pins 11, 13 + CS=7, DC=6, RST=8)
//  Design:   Premium Medical Smartwatch UI
//  Company:  ACRMA TECH SOLUTION PLC
// ================================================================

#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>

// ── Pin Definitions ────────────────────────────────────────────
#define NFC_SS_PIN   10  // Chip Select for NFC
#define NFC_RST_PIN  9   // Reset for NFC
#define TFT_CS       7   // Chip Select for TFT
#define TFT_DC       6   // Data/Command for TFT
#define TFT_RST      8   // Reset for TFT

// ── Station Identity (Change for each sub-station: STA-02, STA-03, etc.)
const char STATION_ID[] = "STA-02 (SUB)";

// ── Hardware Instances ─────────────────────────────────────────
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

// ── UI State ───────────────────────────────────────────────────
unsigned long messageTimer   = 0;
bool          showingMessage = false;
unsigned long pulseTimer     = 0;
uint8_t       pulseStep      = 0;

void drawBootSplash();
void showIdleScreen();
void animatePulseRing();
void showReadingScreen();
void showGrantedScreen(const String &name);
void showDeniedScreen();
void centerText(const char* text, int16_t y, uint8_t size, uint16_t color);

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(50);

  SPI.begin();

  pinMode(NFC_SS_PIN, OUTPUT);
  digitalWrite(NFC_SS_PIN, HIGH);
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);

  rfid.PCD_Init();

  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(C_VOID);

  drawBootSplash();
  delay(1200);

  showIdleScreen();
  Serial.println(F("[SYSTEM] Sub-Station Terminal Ready (NFC + Round TFT)"));
}

void loop() {
  unsigned long now = millis();

  // 1. INBOUND SERIAL FROM PC (Python Bridge)
  if (Serial.available() > 0) {
    String msg = Serial.readStringUntil('\n');
    msg.trim();

    if (msg.startsWith("TFT_SUCCESS:")) {
      String nurseName = msg.substring(12);
      nurseName.trim();
      showGrantedScreen(nurseName);
    } else if (msg == "TFT_ERROR") {
      showDeniedScreen();
    }
  }

  // 2. UI Screen Auto-Return to Idle
  if (showingMessage && (now - messageTimer > 3500)) {
    showIdleScreen();
    showingMessage = false;
  }

  // 3. Pulse Animation on Idle Screen
  if (!showingMessage && (now - pulseTimer > 80)) {
    animatePulseRing();
    pulseTimer = now;
  }

  // 4. NFC Card Detection
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(NFC_SS_PIN, LOW);

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String uid = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      if (rfid.uid.uidByte[i] < 0x10) uid += "0";
      uid += String(rfid.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();

    digitalWrite(NFC_SS_PIN, HIGH);

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
//  SCREENS
// ══════════════════════════════════════════════════════════════

void drawBootSplash() {
  tft.fillScreen(C_VOID);
  tft.drawCircle(120, 120, 117, C_CYAN_DARK);
  tft.drawCircle(120, 120, 116, C_CYAN_DARK);

  tft.fillRoundRect(112, 88, 16, 64, 8, C_CYAN);
  tft.fillRoundRect(88, 112, 64, 16, 8, C_CYAN);

  centerText("MedPulse AI", 175, 2, C_WHITE);
  centerText("NFC ACCESS TERMINAL", 198, 1, C_GRAY_LO);
}

void showIdleScreen() {
  pulseStep = 0;
  tft.fillScreen(C_VOID);

  tft.drawCircle(120, 120, 117, C_CYAN_DARK);
  tft.drawCircle(120, 120, 116, C_CYAN_DARK);

  tft.fillRoundRect(60, 22, 120, 18, 9, C_SURFACE);
  tft.drawRoundRect(60, 22, 120, 18, 9, C_CYAN_DARK);
  centerText(STATION_ID, 27, 1, C_CYAN_DIM);

  centerText("MedPulse", 74, 2, C_WHITE);
  centerText("AI", 92, 2, C_CYAN);

  tft.drawFastHLine(72, 114, 96, C_CYAN_DARK);

  tft.drawCircle(120, 145, 24, C_CYAN_DIM);
  tft.drawCircle(120, 145, 16, C_CYAN_DIM);
  tft.drawCircle(120, 145,  8, C_CYAN);
  tft.fillCircle(120, 145,  3, C_CYAN);

  centerText("TAP NURSE CARD", 186, 1, C_GRAY_MID);
  centerText("TO ACCEPT CALL", 198, 1, C_GRAY_LO);
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
