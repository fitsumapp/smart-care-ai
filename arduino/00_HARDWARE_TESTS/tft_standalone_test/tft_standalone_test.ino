// ================================================================
//  GC9A01 Round TFT Display Standalone Test (ESP32)
//  ዓላማ፡- ስክሪኑ በ ESP32 ላይ ብቻውን ያለምንም ሌላ ሞጁል በትክክል መስራቱን ማረጋገጫ
// ================================================================

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>

// ── Pin Definitions (ESP32 DevKit V1) ──
// ⚠️ ማሳሰቢያ፡ GPIO 2 Onboard LED ያለበት ስለሆነ Reset እንዳይደናቀፍ RST ን ወደ GPIO 14 አድርገነዋል!
#define TFT_CS   15  // Chip Select -> ESP32 GPIO 15
#define TFT_DC   4   // Data / Command -> ESP32 GPIO 4
#define TFT_RST  14  // Reset -> ESP32 GPIO 14

// Hardware SPI Pins on ESP32:
// SCL (Clock) -> GPIO 18
// SDA (MOSI)  -> GPIO 23

Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_RST);

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n=================================");
  Serial.println("  GC9A01 Round TFT Quick Test    ");
  Serial.println("=================================");

  // SPI ን በ 18 (SCK) እና 23 (MOSI) ማስጀመር
  SPI.begin(18, 19, 23, -1);

  // CS ፒንን HIGH ማድረግ
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);

  Serial.println("[1] Initializing TFT (default speed like Arduino Uno)...");
  tft.begin(); // Arduino Uno ላይ ይሰራል ያልከው ልክ እንደዚህ ነው!

  Serial.println("[2] Resetting and setting rotation...");
  tft.setRotation(0);

  Serial.println("[3] Filling Screen RED...");
  tft.fillScreen(GC9A01A_RED);
  delay(1000);

  Serial.println("[4] Filling Screen GREEN...");
  tft.fillScreen(GC9A01A_GREEN);
  delay(1000);

  Serial.println("[5] Filling Screen BLUE...");
  tft.fillScreen(GC9A01A_BLUE);
  delay(1000);

  Serial.println("[6] Filling Screen BLACK and drawing text...");
  tft.fillScreen(GC9A01A_BLACK);

  // ክብ መስመር (Outer Ring)
  tft.drawCircle(120, 120, 115, GC9A01A_CYAN);
  tft.drawCircle(120, 120, 110, GC9A01A_WHITE);

  // ጽሑፍ
  tft.setTextColor(GC9A01A_CYAN);
  tft.setTextSize(2);
  tft.setCursor(45, 80);
  tft.println("MedPulse AI");

  tft.setTextColor(GC9A01A_GREEN);
  tft.setTextSize(3);
  tft.setCursor(70, 115);
  tft.println("READY");

  tft.setTextColor(GC9A01A_WHITE);
  tft.setTextSize(1);
  tft.setCursor(55, 160);
  tft.println("TFT IS WORKING OK!");

  Serial.println("[DONE] TFT test completed successfully!");
}

void loop() {
  tft.drawCircle(120, 120, 115, GC9A01A_CYAN);
  delay(500);
  tft.drawCircle(120, 120, 115, GC9A01A_BLUE);
  delay(500);
}
