// ================================================================
//  MFRC522 RFID / NFC Reader Diagnostic Test (ESP32)
//  ዓላማ፡- የ RFID አንባቢው መስራቱን እና ካርድ ማንበቡን 100% በ Serial Monitor ለማረጋገጥ
// ================================================================

#include <SPI.h>
#include <MFRC522.h>

// ── Pin Definitions (ESP32 DevKit V1) ──
#define NFC_SS_PIN   5   // SDA (SS) -> GPIO 5
#define NFC_RST_PIN  22  // RST -> GPIO 22
#define NFC_SCK_PIN  18  // SCK -> GPIO 18
#define NFC_MISO_PIN 19  // MISO -> GPIO 19
#define NFC_MOSI_PIN 23  // MOSI -> GPIO 23

#define ONBOARD_LED  2   // Onboard Blue LED

MFRC522 rfid(NFC_SS_PIN, NFC_RST_PIN);

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(ONBOARD_LED, OUTPUT);
  digitalWrite(ONBOARD_LED, LOW);

  // TFT CS ካለ እንዳይረብሽ HIGH እናደርገዋለን
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);

  Serial.println("\n==========================================");
  Serial.println("  MFRC522 NFC / RFID Diagnostic (ESP32)   ");
  Serial.println("==========================================");
  Serial.println("[INFO] Pins: SS=5, RST=22, SCK=18, MISO=19, MOSI=23");

  // 1. Initialize Hardware SPI
  SPI.begin(NFC_SCK_PIN, NFC_MISO_PIN, NFC_MOSI_PIN, NFC_SS_PIN);

  // 2. Initialize MFRC522
  rfid.PCD_Init();
  delay(100);

  // 3. Set Antenna Gain to Maximum for maximum reading distance
  rfid.PCD_SetAntennaGain(MFRC522::RxGain_max);

  // 4. Check if MFRC522 chip is communicating
  byte version = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  Serial.printf("[HARDWARE CHECK] Firmware Version: 0x%02X\n", version);

  if (version == 0x00 || version == 0xFF) {
    Serial.println("❌ [ERROR] MFRC522 NOT DETECTED!");
    Serial.println("   እባክዎ የሚከተሉትን ያረጋግጡ፦");
    Serial.println("   1. የ 3.3V እና GND ገመድ በትክክል መገናኘቱን (⚠️ 5V እንዳይሆን!)");
    Serial.println("   2. SCK (18), MOSI (23), MISO (19), SDA (5), RST (22) ገመዶችን");
  } else {
    Serial.println("✅ [SUCCESS] MFRC522 CHIP DETECTED & HEALTHY!");
    if (version == 0x92) Serial.println("   -> Version: MFRC522 v2.0");
    else if (version == 0x91) Serial.println("   -> Version: MFRC522 v1.0");
    Serial.println("👉 እባክዎ ካርድ ወይም ሰማያዊ Keyfob በአንባቢው ላይ ያስጠጉ...");
  }
}

void loop() {
  // Look for new cards
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  // Card Detected!
  digitalWrite(ONBOARD_LED, HIGH);

  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();

  Serial.println("\n🎉 --------------------------------------");
  Serial.printf("  [CARD DETECTED!] UID: %s\n", uid.c_str());
  Serial.printf("  [PICC TYPE]     %s\n", rfid.PICC_GetTypeName(rfid.PICC_GetType(rfid.uid.sak)));
  Serial.println("----------------------------------------");

  // Halt PICC
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  delay(300);
  digitalWrite(ONBOARD_LED, LOW);
}
