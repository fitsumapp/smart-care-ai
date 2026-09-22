// ================================================================
//  MedPulse AI — HUD Access Terminal v3.0 (ESP32 Version)
//  Style: Aerospace HUD · Biopunk · Holographic Ring System
//
//  Hardware : ESP32 DevKit V1 (30-pin) · MFRC522 · GC9A01A 240×240
//  Libraries: Adafruit GFX · Adafruit GC9A01A · MFRC522
//  Company  : ACRMA TECH SOLUTION PLC
// ================================================================

#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include <math.h>

// ── ESP32 Pin Definitions ──────────────────────────────────────
// ⚠️ ማሳሰቢያ፡ በ ESP32 ላይ ፒን 6, 7, 8, 9, 10 ለ Flash Memory ስለሆኑ መጠቀም አይቻልም!
// ስለዚህ ንጹህ እና አስተማማኝ የሆኑትን የሚከተሉትን የ ESP32 ፒኖች እንጠቀማለን፡

// 1. NFC (MFRC522) Pins
#define NFC_SS_PIN   5    // NFC Chip Select (SS) -> ESP32 GPIO 5
#define NFC_RST_PIN  22   // NFC Reset -> ESP32 GPIO 22

// 2. Round TFT (GC9A01A) Pins
#define TFT_CS       15   // TFT Chip Select -> ESP32 GPIO 15
#define TFT_DC       4    // TFT Data/Command -> ESP32 GPIO 4
#define TFT_RST      14   // TFT Reset -> ESP32 GPIO 14
#define TFT_MOSI     23   // SDA (MOSI) -> ESP32 GPIO 23
#define TFT_SCLK     18   // SCL (Clock) -> ESP32 GPIO 18

// 3. Hardware SPI (ለ NFC MISO ብቻ):
#define NFC_MISO_PIN 19   // MISO -> ESP32 GPIO 19 (ለ NFC ብቻ)

// ── Station config ─────────────────────────────────────────────
const char* STATION_ID = "STA-01";
const char* DEPT_TAG   = "HEMATOLOGY";

// ── Hardware Instances ─────────────────────────────────────────
MFRC522           rfid(NFC_SS_PIN, NFC_RST_PIN);
Adafruit_GC9A01A  tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// ── Display constants ──────────────────────────────────────────
#define W    240
#define CX   120
#define CY   120

// ── Palette — HUD color system ─────────────────────────────────
#define IDLE_HI    0x07FC   // #00FFD0 — bright
#define IDLE_MID   0x0398   // #00716C — mid
#define IDLE_LO    0x0190   // #003028 — dark

#define SCAN_HI    0x4D9F   // #4D9FFF — bright
#define SCAN_MID   0x194C   // #192C60 — mid
#define SCAN_LO    0x090A   // #091814 — dark

#define GRNT_HI    0x07EA   // #00FD60 — bright
#define GRNT_MID   0x0380   // #007000 — mid
#define GRNT_LO    0x0200   // #002800 — very dark

#define DENY_HI    0xF9A6   // #FF3050 — bright
#define DENY_MID   0x6000   // #C00000 — mid
#define DENY_LO    0x3000   // #600000 — dark

#define C_BLACK    0x0000
#define C_WHITE    0xFFFF
#define C_DIM      0x2104

// ── State machine ──────────────────────────────────────────────
enum Screen { SCR_IDLE, SCR_SCAN, SCR_GRANTED, SCR_DENIED };
Screen        currentScreen = SCR_IDLE;
unsigned long msgTimer      = 0;
bool          timerActive   = false;
unsigned long animTimer     = 0;
uint8_t       animFrame     = 0;

// ── Function Declarations ──────────────────────────────────────
void drawBootSplash();
void renderIdle();
void drawIdleRingAnim();
void renderScanning();
void renderGranted(String name);
void renderDenied();
void drawArcSegs(int cx, int cy, int r, int n, uint16_t color, int thickness);
void drawArc(int cx, int cy, int r, float a0, float a1, uint16_t color, int steps);
void drawHexagon(int cx, int cy, int r, uint16_t color, int thickness);
void dotRing(int cx, int cy, int r, int n, uint16_t color, int dotR);
void dashedHLine(int x0, int x1, int y, uint16_t color);
void dashedVLine(int x, int y0, int y1, uint16_t color);
void hudText(const char* text, int x, int y, uint8_t size, uint16_t color);

// ══════════════════════════════════════════════════════════════
//  SETUP
// ══════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(500); // ESP32 Serial እንዲረጋጋ ጥቂት መቆየት

  Serial.println("\n[SYSTEM] Starting MedPulse AI HUD Terminal on ESP32...");

  // 1. SPI ማስጀመር
  SPI.begin(18, 19, 23, -1);

  // 2. Chip Select ፒኖችን ማዘጋጀት (እንዳይጋጩ)
  pinMode(NFC_SS_PIN, OUTPUT);
  digitalWrite(NFC_SS_PIN, HIGH);
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);

  // 3. TFT ስክሪኑን ማስጀመር
  Serial.println("[TFT] Initializing GC9A01 Display...");
  tft.begin();
  tft.setRotation(0);

  // 4. RFID ማስጀመር
  Serial.println("[NFC] Initializing MFRC522...");
  rfid.PCD_Init();

  Serial.println("[UI] Drawing Boot Splash...");
  drawBootSplash();
  delay(1400);

  Serial.println("[UI] Rendering Idle HUD...");
  renderIdle();
}

// ══════════════════════════════════════════════════════════════
//  LOOP
// ══════════════════════════════════════════════════════════════
void loop() {
  // 1. Serial inbound (ከኮምፒውተር ወይም ድልድይ ለሚመጣ ዳታ)
  if (Serial.available() > 0) {
    String msg = Serial.readStringUntil('\n');
    msg.trim();
    if (msg.startsWith("TFT_SUCCESS:")) {
      renderGranted(msg.substring(12));
    } else if (msg == "TFT_ERROR") {
      renderDenied();
    }
  }

  // 2. Auto-return to idle
  if (timerActive && millis() - msgTimer > 3500) {
    timerActive = false;
    renderIdle();
  }

  // 3. Idle ring animation (~80ms tick)
  if (currentScreen == SCR_IDLE && millis() - animTimer > 80) {
    animFrame++;
    drawIdleRingAnim();
    animTimer = millis();
  }

  // 4. NFC ካርድ ማንበብ
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  Serial.print("SCAN:"); Serial.println(uid);

  renderScanning();
  delay(900);
  rfid.PICC_HaltA();
}

// ══════════════════════════════════════════════════════════════
//  BOOT SPLASH
// ══════════════════════════════════════════════════════════════
void drawBootSplash() {
  tft.fillScreen(C_BLACK);

  // Outer chrome ring — double line
  tft.drawCircle(CX, CY, 118, IDLE_MID);
  tft.drawCircle(CX, CY, 116, IDLE_LO);

  // Segmented ring at 100
  drawArcSegs(CX, CY, 100, 12, IDLE_MID, 2);

  // Hexagon
  drawHexagon(CX, CY, 50, IDLE_MID, 1);
  drawHexagon(CX, CY, 38, IDLE_LO, 1);

  // Center glyph — plus / cross (medical)
  tft.fillRoundRect(CX-3,  CY-18, 6, 36, 3, IDLE_HI);
  tft.fillRoundRect(CX-18, CY-3,  36, 6,  3, IDLE_HI);

  // Logotype
  hudText("MedPulse AI", CX, CY + 52, 2, IDLE_HI);
  hudText("ACRMA TECH SOLUTION", CX, CY + 70, 1, IDLE_MID);
  hudText("INITIALIZING...", CX, CY + 88, 1, IDLE_LO);

  // Decorative dots
  dotRing(CX, CY, 115, 36, IDLE_LO, 1);
}

// ══════════════════════════════════════════════════════════════
//  IDLE SCREEN
// ══════════════════════════════════════════════════════════════
void renderIdle() {
  currentScreen = SCR_IDLE;
  animFrame = 0;

  tft.fillScreen(C_BLACK);

  // Dot microring around bezel
  dotRing(CX, CY, 115, 48, IDLE_LO, 1);

  // Three concentric segmented arcs
  drawArcSegs(CX, CY, 108, 12, IDLE_LO, 1);
  drawArcSegs(CX, CY, 94,  8,  IDLE_MID, 1);
  drawArcSegs(CX, CY, 76,  6,  IDLE_LO, 1);

  // Hexagonal target frame
  drawHexagon(CX, CY, 46, IDLE_MID, 1);
  drawHexagon(CX, CY, 35, IDLE_LO, 1);

  // NFC target bullseye (static)
  tft.drawCircle(CX, CY, 22, IDLE_MID);
  tft.drawCircle(CX, CY, 13, IDLE_LO);
  tft.fillCircle(CX, CY, 4,  IDLE_HI);

  // Horizontal crosshair dashes
  dashedHLine(CX - 110, CX - 50, CY, IDLE_LO);
  dashedHLine(CX + 50,  CX + 110, CY, IDLE_LO);
  dashedVLine(CX, CY - 110, CY - 50, IDLE_LO);
  dashedVLine(CX, CY + 50,  CY + 110, IDLE_LO);

  // Station badge strip
  tft.fillRoundRect(CX - 36, 18, 72, 15, 7, C_DIM);
  tft.drawRoundRect(CX - 36, 18, 72, 15, 7, IDLE_LO);
  hudText(STATION_ID, CX, 27, 1, IDLE_MID);

  // Title block
  hudText("MedPulse AI", CX, CY - 60, 2, IDLE_HI);
  hudText("NFC TERMINAL", CX, CY - 48, 1, IDLE_MID);

  // Prompt
  hudText("HOLD CARD TO SENSOR", CX, CY + 80, 1, IDLE_MID);
  hudText("ACRMA TECH", CX, CY + 100, 1, IDLE_LO);

  // Outer chrome ring
  tft.drawCircle(CX, CY, 119, IDLE_LO);
  tft.drawCircle(CX, CY, 117, IDLE_MID);
}

// Animating outer pulse ring on idle
void drawIdleRingAnim() {
  static uint8_t lastR = 0;
  if (lastR > 0) tft.drawCircle(CX, CY, lastR, C_BLACK);
  uint8_t phase = animFrame % 16;
  uint8_t r = 88 + phase;
  uint16_t col = (phase < 5) ? IDLE_HI : (phase < 10) ? IDLE_MID : IDLE_LO;
  tft.drawCircle(CX, CY, r, col);
  lastR = r;
}

// ══════════════════════════════════════════════════════════════
//  SCANNING SCREEN
// ══════════════════════════════════════════════════════════════
void renderScanning() {
  currentScreen = SCR_SCAN;
  tft.fillScreen(C_BLACK);

  dotRing(CX, CY, 115, 48, SCAN_LO, 1);

  drawArcSegs(CX, CY, 108, 3,  SCAN_HI, 2);
  drawArcSegs(CX, CY, 108, 3,  SCAN_LO, 1);
  drawArcSegs(CX, CY, 94,  8,  SCAN_MID, 1);
  drawArcSegs(CX, CY, 76,  12, SCAN_LO, 1);

  tft.drawCircle(CX, CY + 2, 22, SCAN_HI);
  tft.drawCircle(CX, CY + 2, 32, SCAN_MID);
  tft.drawCircle(CX, CY + 2, 42, SCAN_LO);

  drawHexagon(CX, CY, 42, SCAN_HI, 2);
  drawHexagon(CX, CY, 32, SCAN_MID, 1);

  tft.drawCircle(CX, CY, 16, SCAN_HI);
  tft.fillCircle(CX, CY, 5, SCAN_HI);

  hudText("READING CARD", CX, CY - 68, 1, SCAN_MID);
  hudText("NFC", CX, CY + 52, 3, SCAN_HI);
  hudText("PROCESSING", CX, CY + 70, 1, SCAN_MID);
  hudText("PLEASE WAIT", CX, CY + 82, 1, SCAN_LO);

  tft.drawCircle(CX, CY, 119, SCAN_MID);
  tft.drawCircle(CX, CY, 117, SCAN_LO);
}

// ══════════════════════════════════════════════════════════════
//  ACCESS GRANTED
// ══════════════════════════════════════════════════════════════
void renderGranted(String name) {
  currentScreen = SCR_GRANTED;
  tft.fillScreen(C_BLACK);

  dotRing(CX, CY, 115, 48, GRNT_LO, 1);
  drawArcSegs(CX, CY, 108, 1,  GRNT_HI, 3);
  drawArcSegs(CX, CY, 108, 8,  GRNT_MID, 1);
  drawArcSegs(CX, CY, 92,  6,  GRNT_LO, 1);

  drawHexagon(CX, CY, 50, GRNT_HI, 2);
  drawHexagon(CX, CY, 38, GRNT_MID, 1);

  for (int r = 28; r >= 20; r -= 2)
    tft.drawCircle(CX, CY, r, (r > 25) ? GRNT_LO : GRNT_MID);
  tft.drawCircle(CX, CY, 26, GRNT_HI);

  for (int t = -1; t <= 1; t++) {
    tft.drawLine(CX - 10 + t, CY,     CX - 2 + t,  CY + 9, GRNT_HI);
    tft.drawLine(CX - 2 + t,  CY + 9, CX + 12 + t, CY - 9, GRNT_HI);
  }

  hudText("IDENTITY VERIFIED", CX, CY - 68, 1, GRNT_MID);

  String n = name;
  if (n.length() > 12) n = n.substring(0, 12);
  hudText(n.c_str(), CX, CY + 50, 2, GRNT_HI);
  hudText(DEPT_TAG, CX, CY + 66, 1, GRNT_MID);

  tft.fillRoundRect(CX - 52, CY + 78, 104, 18, 9, C_DIM);
  tft.drawRoundRect(CX - 52, CY + 78, 104, 18, 9, GRNT_HI);
  hudText("ACCESS GRANTED", CX, CY + 88, 1, GRNT_HI);

  tft.drawCircle(CX, CY, 119, GRNT_HI);
  tft.drawCircle(CX, CY, 117, GRNT_MID);

  msgTimer = millis(); 
  timerActive = true;
}

// ══════════════════════════════════════════════════════════════
//  ACCESS DENIED
// ══════════════════════════════════════════════════════════════
void renderDenied() {
  currentScreen = SCR_DENIED;
  tft.fillScreen(C_BLACK);

  dotRing(CX, CY, 115, 48, DENY_LO, 1);
  drawArcSegs(CX, CY, 108, 2,  DENY_HI, 3);
  drawArcSegs(CX, CY, 108, 6,  DENY_MID, 1);
  drawArcSegs(CX, CY, 90,  4,  DENY_LO, 1);

  drawHexagon(CX, CY, 48, DENY_HI, 2);
  drawHexagon(CX, CY, 36, DENY_MID, 1);

  for (int r = 28; r >= 20; r -= 2)
    tft.drawCircle(CX, CY, r, (r > 25) ? DENY_LO : DENY_MID);
  tft.drawCircle(CX, CY, 26, DENY_HI);

  for (int t = -1; t <= 1; t++) {
    tft.drawLine(CX - 9 + t, CY - 9, CX + 9 + t, CY + 9, DENY_HI);
    tft.drawLine(CX + 9 + t, CY - 9, CX - 9 + t, CY + 9, DENY_HI);
  }

  hudText("CARD NOT REGISTERED", CX, CY - 68, 1, DENY_MID);
  hudText("UNKNOWN", CX, CY + 48, 2, DENY_HI);
  hudText("IDENTITY", CX, CY + 64, 1, DENY_MID);

  tft.fillRoundRect(CX - 48, CY + 78, 96, 18, 9, C_DIM);
  tft.drawRoundRect(CX - 48, CY + 78, 96, 18, 9, DENY_HI);
  hudText("ACCESS DENIED", CX, CY + 88, 1, DENY_HI);

  tft.drawCircle(CX, CY, 119, DENY_HI);
  tft.drawCircle(CX, CY, 117, DENY_MID);

  msgTimer = millis(); 
  timerActive = true;
}

// ══════════════════════════════════════════════════════════════
//  DRAWING PRIMITIVES
// ══════════════════════════════════════════════════════════════
void drawArcSegs(int cx, int cy, int r, int n, uint16_t color, int thickness) {
  float gapAngle = 0.22;
  float segAngle = (TWO_PI - gapAngle * n) / n;
  for (int i = 0; i < n; i++) {
    float startA = -HALF_PI + i * (segAngle + gapAngle);
    float endA   = startA + segAngle;
    for (int t = 0; t < thickness; t++) {
      drawArc(cx, cy, r + t, startA, endA, color, 1 + (int)(segAngle * r / 8));
    }
  }
}

void drawArc(int cx, int cy, int r, float a0, float a1, uint16_t color, int steps) {
  float prev_x = cx + cos(a0) * r;
  float prev_y = cy + sin(a0) * r;
  for (int i = 1; i <= steps; i++) {
    float a = a0 + (a1 - a0) * i / steps;
    float nx = cx + cos(a) * r;
    float ny = cy + sin(a) * r;
    tft.drawLine((int)prev_x, (int)prev_y, (int)nx, (int)ny, color);
    prev_x = nx; prev_y = ny;
  }
}

void drawHexagon(int cx, int cy, int r, uint16_t color, int thickness) {
  float offset = PI / 6.0;
  for (int t = 0; t < thickness; t++) {
    int rr = r + t;
    for (int i = 0; i < 6; i++) {
      float a0 = offset + (PI / 3.0) * i;
      float a1 = offset + (PI / 3.0) * (i + 1);
      tft.drawLine(
        cx + (int)(cos(a0) * rr), cy + (int)(sin(a0) * rr),
        cx + (int)(cos(a1) * rr), cy + (int)(sin(a1) * rr),
        color
      );
    }
  }
}

void dotRing(int cx, int cy, int r, int n, uint16_t color, int dotR) {
  for (int i = 0; i < n; i++) {
    float a = (TWO_PI / n) * i - HALF_PI;
    tft.fillCircle(cx + (int)(cos(a) * r), cy + (int)(sin(a) * r), dotR, color);
  }
}

void dashedHLine(int x0, int x1, int y, uint16_t color) {
  for (int x = x0; x < x1; x += 6) tft.drawPixel(x, y, color);
}

void dashedVLine(int x, int y0, int y1, uint16_t color) {
  for (int y = y0; y < y1; y += 6) tft.drawPixel(x, y, color);
}

void hudText(const char* text, int x, int y, uint8_t size, uint16_t color) {
  tft.setTextSize(size);
  tft.setTextColor(color);
  int w = strlen(text) * 6 * size;
  tft.setCursor(x - w / 2, y - (4 * size));
  tft.print(text);
}
