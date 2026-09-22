# 🏥 MedPulse Smart-Care AI — ESP32 Master Gateway & Sub-Station አሰራር እና አጠቃቀም መመሪያ

ይህ ሰነድ የ **MedPulse Smart-Care AI** ሲስተምን በ **ESP32** አማካኝነት በሆስፒታል ውስጥ በሁለቱም አማራጮች (በቀጥታ በ Wi-Fi ወይም በ USB ከኮምፒውተር ጋር) ለማዋቀር የተዘጋጀ የተሟላ መመሪያ ነው።

---

## 📁 የተዘጋጁት የኮድ ፋይሎች ማውጫ (Project Structure)

በ `arduino/` ፎልደር ውስጥ ለሁለቱም አማራጮች 4 የተለያዩ የተዘጋጁ `.ino` ፋይሎች ተቀምጠዋል፦

```text
ai_nurse_system/arduino/
│
├── 🌐 አማራጭ 1፡ ቀጥታ በ Wi-Fi (Standalone Mode — ምንም ኮምፒውተር አያስፈልገውም)
│   ├── esp32_master_gateway_wifi/
│   │   └── esp32_master_gateway_wifi.ino   (ESP32 + LoRa + NFC + Round TFT + Phone Wi-Fi Setup)
│   └── esp32_sub_station_wifi/
│       └── esp32_sub_station_wifi.ino       (ESP32 + NFC + Round TFT + Phone Wi-Fi Setup — ያለ LoRa)
│
└── 🔌 አማራጭ 2፡ በ USB ገመድ ከኮምፒውተር ጋር (USB Serial Mode)
    ├── esp32_master_gateway_usb/
    │   └── esp32_master_gateway_usb.ino    (ESP32 + LoRa + NFC + Round TFT ወደ PC Serial)
    └── esp32_sub_station_usb/
        └── esp32_sub_station_usb.ino        (ESP32 + NFC + Round TFT ወደ PC Serial — ያለ LoRa)
```

---

## ⚙️ አማራጭ 1፡ ሙሉ ለሙሉ በ Wi-Fi የሚሰራ (ምንም ኮምፒውተር አያስፈልገውም) — ⭐ እጅግ የሚመከረው

በዚህ አሰራር የነርስ ጣቢያዎች ላይ **ኮምፒውተር አያስፈልግም**፤ ጌትዌዩ በ 5V የስልክ ቻርጀር ብቻ ይሰካል።

### ሀ. Master Gateway (ዋናው ነርስ ጣቢያ)
* **ሞጁሎች**፦ `ESP32 + LoRa E32 + MFRC522 NFC + GC9A01 Round TFT`
* **ስራው**፦
  1. ከሁሉም ክፍሎች የሚላኩ የ LoRa ጥሪዎችን ይቀበላል።
  2. በሆስፒታሉ Wi-Fi በኩል በቀጥታ ወደ Deployed Cloud Server (`POST /api/calls/`) ይልካል።
  3. ነርሷ NFC ካርዷን ስታነብ ወደ `/api/acknowledge_nfc/` ልኮ ጥሪውን ያጠፋል።
  4. ጥሪው በዳሽቦርድ ሲጠፋ በ LoRa ወደ ታካሚው ክፍል `DONE:room` ልኮ የክፍሉን ማንቂያ ያጠፋል።

### ለ. Sub-Stations (ቅርንጫፍ ነርስ ጣቢያዎች — Floor 2, ICU, ወዘተ)
* **ሞጁሎች**፦ `ESP32 + MFRC522 NFC + GC9A01 Round TFT` ብቻ (**ያለ LoRa!**)
* **ስራው**፦
  1. ጥሪዎችን ለመቀበል የ LoRa ሞጁል አያስፈልገውም (ምክንያቱም ጥሪው በዋናው ጌትዌይ ገብቶ ሰርቨር ላይ ይገኛል)።
  2. ነርሷ በዚህ ጣቢያ ሆና የ NFC ካርዷን ስታነብ፣ ጣቢያው በ Wi-Fi አማካኝነት ወደ Cloud Server ልኮ ጥሪውን Acknowledge / Accept ያደርጋል።

---

## 📱 ዋይፋይን በስልክ የማስተካከያ ዘዴ (Captive Portal / Phone Setup)

የ Wi-Fi ስም ወይም ፓስወርድ ሲቀየር መሳሪያውን ኮምፒውተር ላይ ሰክቶ ኮድ መጻፍ **አያስፈልግም**!

1. ESP32-ው ሲበራ የተመዘገበ ዋይፋይ ካጣ (ወይም በግራ በኩል ያለውን **BOOT (GPIO 0)** ቁልፍ ተጭነው ሲያበሩት)፦
   - በስክሪኑ ላይ **"HOTSPOT ACTIVE - SETUP VIA PHONE"** የሚል ይወጣል።
2. በስልክዎ የ Wi-Fi ዝርዝር ውስጥ ገብተው የሚከተለውን ያገናኙ፦
   - **Wi-Fi Name**: `SmartCare-Master-Config` (ወይም ለ Sub-Station `SmartCare-SubStation-Config`)
   - **Password**: `12345678`
3. ልክ እንደተገናኙ ስልክዎ ላይ **የማስተካከያ ገጽ (Portal) በራሱ ይከፈታል** (ካልከፈተ በ Chrome/Safari ላይ `192.168.4.1` ብለው ይክፈቱ)።
4. በሚመጣው ፎርም ላይ፦
   - የሆስፒታሉን ዋይፋይ ይምረጡና ፓስወርዱን ያስገቡ።
   - የሲስተሙን ድረ-ገጽ URL ያስገቡ (ለምሳሌ፡ `https://your-domain.com`)።
   - የስቴሽኑን ስም ይስጡ (ለምሳሌ፡ `Nurse Station 1` ወይም `Nurse Station 2`)።
5. **"Save & Connect"** የሚለውን ሲጫኑ ESP32-ው መረጃውን በቋሚ ሚሞሪው (NVS Flash) መዝግቦ ራሱን አጥፍቶ በማብራት ከሆስፒታሉ ዋይፋይ ጋር ይገናኛል!

---

## 🔌 አማራጭ 2፡ በ USB ገመድ ከኮምፒውተር ጋር (USB Serial Mode)

በዚህ አማራጭ ESP32-ው በ USB ገመድ ከኮምፒውተሩ ጋር ይሰካል፦
1. **Master Gateway** ጥሪ ሲመጣ በ USB Serial (`START:10:Bed 1`) ለኮምፒውተሩ ይሰጣል፤ ኮምፒውተሩ ጥሪውን ለሰርቨር ያስተላልፋል።
2. **Sub-Station** ነርሷ ካርድ ስታነብ በ USB Serial (`SCAN:UID`) ለኮምፒውተሩ ይሰጣል፤ ኮምፒውተሩ ጥሪውን Acknowledge ያደርጋል።
3. ኮምፒውተሩ ላይ ያለው የነርስ ዳሽቦርድ በ **Web Serial API** ወይም በ `client_bridges/` ፓይዘን ፕሮግራም ከ ESP32 ጋር ይነጋገራል።

---

## 📌 የገመድ ማያያዣ ንድፍ (ESP32 DevKit V1 30-Pin Wiring)

### 1. Master Gateway (ESP32 + LoRa + NFC + Round TFT)

| ሞጁል | የሞጁሉ ፒን | የ ESP32 ፒን | መግለጫ |
| :--- | :--- | :--- | :--- |
| **LoRa E32** | **TX** | **GPIO 16 (RX2)** | Hardware Serial2 መቀበያ |
| | **RX** | **GPIO 17 (TX2)** | Hardware Serial2 መላኪያ |
| | **M0, M1** | **GND** | Normal Transparent Mode |
| | **VCC / GND** | **5V (VIN) / GND** | 5V ሃይል |
| **MFRC522 (NFC)** | **SCK** | **GPIO 18** | Shared Hardware SPI Clock |
| | **MOSI** | **GPIO 23** | Shared Hardware SPI MOSI |
| | **MISO** | **GPIO 19** | SPI MISO |
| | **SDA (SS)** | **GPIO 5** | NFC Chip Select |
| | **RST** | **GPIO 22** | NFC Reset |
| | **3.3V / GND** | **3.3V / GND** | ⚠️ **3.3V ብቻ** |
| **GC9A01 (TFT)** | **SCL (SCK)** | **GPIO 18** | Shared Hardware SPI Clock |
| | **SDA (MOSI)**| **GPIO 23** | Shared Hardware SPI MOSI |
| | **CS** | **GPIO 15** | TFT Chip Select |
| | **DC** | **GPIO 4** | Data / Command |
| | **RES (RST)** | **GPIO 14** | TFT Reset (GPIO 14 ንጹህ ፒን ነው) |
| | **VCC / GND** | **5V (VIN) / GND** | 5V ሃይል (ከ 3.3V ይልቅ 5V/VIN) |

---

### 2. Sub-Station Node (ESP32 + NFC + Round TFT — ያለ LoRa)

*የ LoRa ፒኖችን (GPIO 16 እና 17) ክፍት ትተዋቸዋላችሁ፤ የተቀረው የ NFC እና የ TFT ገመድ ልክ ከላይ ባለው ሰንጠረዥ መሰረት ይያያዛል።*

---

## 💻 Arduino IDE ላይ ለመጫን የሚያስፈልጉ ላይብረሪዎች (Libraries)

በ Arduino IDE ውስጥ **Sketch ➔ Include Library ➔ Manage Libraries...** በመግባት የሚከተሉትን ይጫኑ፦
1. **Adafruit GFX Library** (by Adafruit)
2. **Adafruit GC9A01A** (by Adafruit)
3. **MFRC522** (by GithubCommunity)

*(ማስታወሻ፡ `WiFi.h`, `HTTPClient.h`, `WebServer.h`, `DNSServer.h`, `Preferences.h`, `SPI.h` የ ESP32 ቦርድን ሲጭኑ አብረው የሚመጡ ናቸው፤ ተጨማሪ ጭነት አያስፈልጋቸውም)*
