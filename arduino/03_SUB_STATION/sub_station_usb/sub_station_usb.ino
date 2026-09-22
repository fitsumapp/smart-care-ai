// ================================================================
//  MedPulse Smart-Care AI — ESP32 Sub-Station Node (USB Serial Mode)
//  Hardware: ESP32 DevKit V1 (30-pin)
//  Peripherals:
//    1. MFRC522 RFID / NFC Reader (VSPI: SCK=18, MOSI=23, MISO=19, CS=5, RST=22)
//    2. GC9A01A 240x240 Round TFT Display (VSPI: SCK=18, MOSI=23, CS=15, DC=4, RST=2)
//  Connection: USB Cable to Sub-Station PC
//  Note: NO LoRa module needed! Reads nurse badge and sends SCAN:uid over USB Serial.
//  Company: ACRMA TECH SOLUTION PLC
// ================================================================

#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>

// ── Pin Definitions ────────────────────────────────────────────
// NFC (MFRC522) Pins
#define NFC_SS_PIN   5    // Chip Select for NFC
#define NFC_RST_PIN  22   // Reset for NFC

// Round TFT (GC9A01A) Pins
#define TFT_CS       15   // Chip Select for TFT
#define TFT_DC       4    // Data/Command for TFT
#define TFT_RST      14   // Reset for TFT (GPIO 14)
#define TFT_MOSI     23   // SDA (MOSI)
#define TFT_SCLK     18   // SCL (Clock)

// Hardware SPI Pins (VSPI on ESP32):
// SCK  = GPIO 18 (Shared between TFT and NFC)
// MOSI = GPIO 23 (Shared between TFT and NFC)
// MISO = GPIO 19 (NFC MISO only)

// ── Color Palette (16-bit RGB565) ──────────────────────────────
#define C_VOID       0x0000
#define C_SURFACE    0x0842
#define C_CYAN       0x07FF
#define C_CYAN_DIM   0x0398
#define C_GREEN      0x3FE6
#define C_RED        0xF185
#define C_WHITE      0xFFFF
#define C_GRAY_MID   0x7BEF

// ── Hardware Instances ─────────────────────────────────────────
MFRC522          rfid(NFC_SS_PIN, NFC_RST_PIN);
Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

const char STATION_ID[] = "SUB-STATION (NFC)";

// ── UI State Variables ─────────────────────────────────────────
unsigned long messageTimer   = 0;
bool          showingMessage = false;
unsigned long pulseTimer     = 0;
uint8_t       pulseStep      = 0;

// ── Function Declarations ──────────────────────────────────────
void drawBootSplash();
void showIdleScreen();
void animatePulseRing();
void showGrantedScreen(const String &name);
void showDeniedScreen();
void centerText(const char* text, int16_t y, uint8_t size, uint16_t color);

// ================================================================
//  SETUP
// ================================================================
void setup() {
  Serial.begin(115200);
  Serial.setTimeout(50);

  // Initialize SPI Bus (VSPI)
  SPI.begin(18, 19, 23, -1);

  // Initialize Chip Selects
  pinMode(NFC_SS_PIN, OUTPUT);
  digitalWrite(NFC_SS_PIN, HIGH);
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);

  // Init RFID Reader
  rfid.PCD_Init();

  // Init GC9A01 Round TFT Display
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(C_VOID);

  drawBootSplash();
  delay(1200);

  showIdleScreen();
  Serial.println(F("[SYSTEM] ESP32 Sub-Station Node Ready (USB + NFC + TFT)"));
}

// ================================================================
//  MAIN LOOP
// ================================================================
void loop() {
  unsigned long now = millis();

  // 1. INBOUND SERIAL FROM PC (Python Bridge or Web Serial)
  if (Serial.available() > 0) {
    String pcCmd = Serial.readStringUntil('\n');
    pcCmd.trim();

    if (pcCmd.length() > 0) {
      // Command A: TFT Access Granted (Nurse Name)
      if (pcCmd.startsWith("TFT_SUCCESS:")) {
        String nurseName = pcCmd.substring(12);
        nurseName.trim();
        showGrantedScreen(nurseName);
      }
      // Command B: TFT Access Denied
      else if (pcCmd.startsWith("TFT_ERROR")) {
        showDeniedScreen();
      }
    }
  }

  // 2. NFC CARD SCANNING (MFRC522)
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(NFC_SS_PIN, LOW);

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String uid = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      if (rfid.uid.uidByte[i] < 0x10) uid += "0";
      uid += String(rfid.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();

    // Send SCAN event to PC via USB Serial
    Serial.println("SCAN:" + uid);

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }

  digitalWrite(NFC_SS_PIN, HIGH);

  // 3. TFT UI SCREEN TIMEOUT & IDLE ANIMATION
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
//  GC9A01A ROUND TFT UI RENDERING
// ================================================================

void centerText(const char* text, int16_t y, uint8_t size, uint16_t color) {
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
  centerText("SUB-STATION NODE", 125, 1, C_GRAY_MID);
  centerText("Ready", 160, 1, C_GREEN);
}

void showIdleScreen() {
  tft.fillScreen(C_VOID);
  tft.drawCircle(120, 120, 115, C_SURFACE);
  centerText("MEDPULSE AI", 55, 1, C_CYAN_DIM);
  centerText("SCAN NFC", 105, 2, C_WHITE);
  centerText("TAP BADGE TO ACK", 135, 1, C_GRAY_MID);
  centerText(STATION_ID, 175, 1, C_CYAN_DIM);
}

void animatePulseRing() {
  int r = 100 + (pulseStep % 10);
  tft.drawCircle(120, 120, r, (pulseStep % 2 == 0) ? C_CYAN_DIM : C_VOID);
  pulseStep++;
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
  centerText(STATION_ID, 185, 1, C_CYAN_DIM);
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
