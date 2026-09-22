# 🔌 MedPulse Smart-Care AI — የሃርድዌር ገመድ ማገናኘት እና ማዋቀር መመሪያ (Hardware Wiring & Setup Guide)

ይህ መመሪያ ሁሉንም የሃርድዌር ክፍሎች (Master Gateway, Sub-Station, Patient Room, እና AI Vision Camera) እንዴት ማገናኘት እና ማዋቀር እንዳለብዎት በዝርዝር ያብራራል።

---

## 📚 1. በአርዱኢኖ IDE መጫን ያለባቸው ላይብረሪዎች (Required Arduino Libraries)

በ Arduino IDE ውስጥ **Sketch ➔ Include Library ➔ Manage Libraries...** በመክፈት የሚከተሉትን ይጫኑ፦

1. **`MFRC522`** (by GithubCommunity) — ለ NFC/RFID ካርድ አንባቢ
2. **`Adafruit GFX Library`** (by Adafruit) — ለግራፊክስ
3. **`Adafruit GC9A01A`** (by Adafruit) — ለ 240x240 ክብ TFT ስክሪን
4. **`SoftwareSerial`** — አብሮ የሚመጣ (Built-in)

---

## 📡 2. ማስተር ጣቢያ (Station 1 - Master Gateway Wiring)
**1 Arduino (Uno/Nano/Mega/ESP32) ➔ LoRa E32 + MFRC522 + GC9A01A Round TFT**

### የፒን ማገናኛ ሰንጠረዥ (Pinout Table)፦

| ሞጁል (Module) | የሞጁሉ ፒን | የአርዱኢኖ ፒን (Uno/Nano) | ማስታወሻ |
| :--- | :--- | :--- | :--- |
| **LoRa (E32)** | VCC | **5V** | 5V ኃይል |
| | GND | **GND** | የጋራ ምድር |
| | TX | **Pin 2** (RX) | SoftwareSerial |
| | RX | **Pin 3** (TX) | SoftwareSerial |
| | M0, M1 | **GND** (ወይም Floating) | Normal Transmit Mode |
| **NFC (MFRC522)** | 3.3V | **3.3V** | ⚠️ 3.3V ብቻ! |
| | GND | **GND** | የጋራ ምድር |
| | MOSI | **Pin 11** | Shared SPI MOSI |
| | MISO | **Pin 12** | Shared SPI MISO |
| | SCK | **Pin 13** | Shared SPI Clock |
| | SDA / SS | **Pin 10** | NFC Chip Select (CS) |
| | RST | **Pin 9** | NFC Reset |
| **Round TFT (GC9A01A)** | VCC | **3.3V ወይም 5V** | የስክሪኑ VCC |
| | GND | **GND** | የጋራ ምድር |
| | SCL / SCK | **Pin 13** | Shared SPI Clock |
| | SDA / MOSI | **Pin 11** | Shared SPI MOSI |
| | CS | **Pin 7** | TFT Chip Select (CS) |
| | DC | **Pin 6** | TFT Data/Command |
| | RST | **Pin 8** | TFT Reset |
| | BLK (Backlight) | **3.3V ወይም 5V** | የበስተጀርባ መብራት |

---

## 🏷️ 3. ንዑስ ነርስ ጣቢያዎች (Sub-Station Terminal Wiring)
**1 Arduino ➔ MFRC522 + GC9A01A Round TFT (LoRa አያስፈልገውም)**

| ሞጁል | የሞጁሉ ፒን | የአርዱኢኖ ፒን |
| :--- | :--- | :--- |
| **NFC (MFRC522)** | 3.3V | **3.3V** |
| | GND | **GND** |
| | MOSI | **Pin 11** |
| | MISO | **Pin 12** |
| | SCK | **Pin 13** |
| | SDA / SS | **Pin 10** |
| | RST | **Pin 9** |
| **Round TFT (GC9A01A)** | VCC / GND | **5V / GND** |
| | SCL / SCK | **Pin 13** |
| | SDA / MOSI | **Pin 11** |
| | CS | **Pin 7** |
| | DC | **Pin 6** |
| | RST | **Pin 8** |
| | BLK | **3.3V** |

---

## 🛏️ 4. የታካሚ ክፍል መላኪያ (Patient Room Transmitter Wiring)
**Arduino ➔ LoRa E32 + Buttons + LED + Buzzer**

| አካል (Component) | ፒን | የአርዱኢኖ ፒን |
| :--- | :--- | :--- |
| **Bed 1 Button** | Normal Open | **Pin 2** (ወደ GND የሚጫን) |
| **Bed 2 Button** | Normal Open | **Pin 5** (ወደ GND የሚጫን) |
| **Bed 3 Button** | Normal Open | **Pin 6** (ወደ GND የሚጫን) |
| **Bathroom Button** | Normal Open | **Pin 7** (ወደ GND የሚጫን) |
| **Nurse Arrived Button** | Normal Open | **Pin 8** (ወደ GND የሚጫን) |
| **Confirm LED** | Anode (+) | **Pin 4** (ከ 220Ω resistor ጋር) |
| **Room Buzzer** | Positive (+) | **Pin 3** |
| **LoRa (E32)** | TX | **Pin 10** (SoftwareSerial RX) |
| | RX | **Pin 11** (SoftwareSerial TX) |

---

## 📹 5. የ AI Vision ካሜራ ክፍል (Raspberry Pi + Camera)

### 🚀 ፈጣን ጭነት በ 1 ትዕዛዝ (Automated Setup)፦
በ Raspberry Pi ተርሚናል ውስጥ የሚከተለውን ያሂዱ፦
```bash
cd ~/ai_nurse_system/ai_vision
bash setup_pi.sh
```
ይህ ስክሪፕት አስፈላጊ የሆኑትን የካሜራ እና የ AI ላይብረሪዎች (OpenCV, MediaPipe, Requests) በሙሉ በራስ-ሰር ይጭናል።

### ⚙️ ማስተካከል (Configuration)፦
[patient_monitor.py](file:///c:/Users/BAB%20AL%20SAFA/Desktop/ai_nurse_system/ai_vision/patient_monitor.py) ፋይልን ከፍተው ከላይ ያሉትን መስመሮች ያቀናጁ፦
```python
SERVER_BASE_URL = "https://yourdomain.com"     # የሰርቨርዎ አድራሻ (ወይም http://127.0.0.1:8000)
LOCATION_NAME   = "Corridor 2nd Floor"         # የቦታው ስም (ለምሳሌ፡ Room 5, Corridor 2nd Floor, Staircase B)
LOCATION_TYPE   = "CORRIDOR"                   # የቦታው አይነት (ROOM, CORRIDOR, STAIRS, BATHROOM)
```

### 🔄 ለ 24/7 ያለማቋረጥ ከበስተጀርባ እንዲሰራ ማድረግ (Auto-Start Service)፦
ፒአይ ሲበራ ካሜራው በራሱ ተነስቶ ያለማቋረጥ እንዲሰራ ለማድረግ፦
```bash
cd ~/ai_nurse_system/ai_vision
bash install_service.sh
```
- **ሁኔታውን ለማየት:** `sudo systemctl status patient_monitor`
- **የቀጥታ መረጃ (Logs) ለማየት:** `sudo journalctl -u patient_monitor -f`

---

## 💻 6. የኮምፒውተር ደንበኛ ስክሪፕቶችን በራስ-ሰር ማስነሳት (Windows Auto-Start)

ነርሷ ኮምፒውተሩን ስታበራ `master_bridge.py` ወይም `sub_station_bridge.py` ከበስተጀርባ (Background) በራሱ እንዲነሳ፦

1. `Win + R` ተጭነው `shell:startup` ብለው ይጻፉ (የ Startup ፎልደር ይከፈታል)።
2. በፎልደሩ ውስጥ `start_bridge.bat` የሚል ፋይል ይፍጠሩ እና የሚከተለውን ያስገቡ፦
```bat
@echo off
cd /d "C:\Users\...\ai_nurse_system\client_bridges"
start /min python master_bridge.py
```
3. አሁን ኮምፒውተሩ ሲበራ ስክሪፕቱ በራሱ ተነስቶ ከአርዱኢኖ እና ከክላውድ ጋር ይገናኛል!
