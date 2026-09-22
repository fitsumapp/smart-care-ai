// ================================================================
//  GC9A01 Round TFT Display Ultimate Diagnostic (ESP32)
//  ዓላማ፡- ስክሪኑ በ ESP32 ላይ 100% እንዲበራ የተዘጋጀ አስተማማኝ የፍተሻ ኮድ
// ================================================================

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>

// ── Pin Definitions (ESP32 DevKit V1) ──
#define TFT_CS   15  // Chip Select -> D15
#define TFT_DC   4   // Data / Command -> D4
#define TFT_MOSI 23  // SDA (Data In) -> D23
#define TFT_SCLK 18  // SCL (Clock) -> D18
#define TFT_RST  14  // Reset -> D14 (ወይም ወደ 3.3V ተገናኝቶ ከሆነ -1 አድርገው)

#define ONBOARD_LED 2 // ESP32 ላይ ያለችው ሰማያዊ መብራት

// ⚠️ ማስታወሻ፦ ፒኖቹን በግልጽ በመጥቀስ ልክ እንደ Arduino Uno በቀስታ እና በንጽህና እንዲሰራ አድርገነዋል
Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(ONBOARD_LED, OUTPUT);
  digitalWrite(ONBOARD_LED, HIGH); // ESP32 መስራቱን ለማሳየት LED ይበራል

  Serial.println("\n==========================================");
  Serial.println("  GC9A01 Round TFT Diagnostic (ESP32)     ");
  Serial.println("==========================================");
  Serial.println("[INFO] Pins: CS=15, DC=4, SCL=18, SDA=23, RST=14");

  Serial.println("[1] Resetting display pins...");
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  pinMode(TFT_DC, OUTPUT);
  digitalWrite(TFT_DC, HIGH);

  if (TFT_RST > 0) {
    pinMode(TFT_RST, OUTPUT);
    digitalWrite(TFT_RST, HIGH);
    delay(50);
    digitalWrite(TFT_RST, LOW);
    delay(50);
    digitalWrite(TFT_RST, HIGH);
    delay(150);
  }

  Serial.println("[2] Initializing GC9A01 driver...");
  tft.begin();
  tft.setRotation(0);

  Serial.println("[3] Drawing Test Screens...");
  
  // RED
  Serial.println(" -> SCREEN: RED");
  tft.fillScreen(GC9A01A_RED);
  delay(1200);

  // GREEN
  Serial.println(" -> SCREEN: GREEN");
  tft.fillScreen(GC9A01A_GREEN);
  delay(1200);

  // BLUE
  Serial.println(" -> SCREEN: BLUE");
  tft.fillScreen(GC9A01A_BLUE);
  delay(1200);

  // BLACK + UI
  Serial.println(" -> SCREEN: BLACK + GRAPHICS");
  tft.fillScreen(GC9A01A_BLACK);

  tft.drawCircle(120, 120, 116, GC9A01A_CYAN);
  tft.drawCircle(120, 120, 112, GC9A01A_WHITE);

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
  tft.setCursor(48, 160);
  tft.println("TFT IS WORKING 100%!");

  Serial.println("[DONE] Setup finished! Entering loop...");
}

void loop() {
  // Onboard LED እና የክቡን ቀለም ማብራት ማጥፋት
  digitalWrite(ONBOARD_LED, HIGH);
  tft.drawCircle(120, 120, 116, GC9A01A_CYAN);
  delay(500);

  digitalWrite(ONBOARD_LED, LOW);
  tft.drawCircle(120, 120, 116, GC9A01A_BLUE);
  delay(500);

  Serial.println("[HEARTBEAT] ESP32 is running and pulsing display...");
}
