# 🏥 MedPulse Smart-Care AI — የአርዱኢኖ (Arduino / ESP32) ፋይሎች ማውጫ

ግራ እንዳይገባህ ሁሉም የኮድ ፋይሎች በየስራቸው ተከፋፍለው በቁጥር ተቀምጠዋል፦

---

### 📂 00_HARDWARE_TESTS (ለፍተሻ ብቻ)
* 📁 `tft_standalone_test/tft_standalone_test.ino`
  * **ጥቅሙ፦** የ GC9A01 Round TFT ስክሪን ብቻውን በትክክል መስራቱን (ቀይ፣ አረንጓዴ፣ ሰማያዊ ቀለም እና ጽሑፍ ማሳየቱን) በፍጥነት ማረጋገጫ።

---

### 📂 01_PATIENT_ROOM_CALLER (የታካሚ አልጋ ጥሪ ማድረጊያ)
* 📁 `patient_room_transmitter/patient_room_transmitter.ino`
  * **ጥቅሙ፦** በታካሚ ክፍል ወይም አልጋ አጠገብ የሚቀመጥ መሳሪያ ነው። 
  * ታካሚው ቁልፉን ሲጫን (Emergency, Nurse Call, Doctor Call, Assist) በ LoRa አማካኝነት ወደ ነርስ ጣቢያ ጥሪ ይልካል።

---

### 📂 02_MASTER_GATEWAY (ዋናው ነርስ ስቴሽን — ESP32 + LoRa + NFC + TFT)
ታካሚዎች በ LoRa የሚልኩትን ጥሪ ተቀብሎ በስክሪኑ ላይ ያሳያል፤ ነርሶች ካርዳቸውን በ NFC ሲነኩ ጥሪውን ያጠፋል፤ ለዌብሳይቱ (ለሆስፒታሉ ሲስተም) ይልካል።

* 📁 **`master_gateway_wifi/master_gateway_wifi.ino`** 👈 **(በጣም የሚመከረው!)**
  * **እንዴት ይሰራል?** ኮምፒውተር ሳይፈልግ በራሱ በ Wi-Fi ከ Cloud ሰርቨርህ ጋር ይገናኛል። የሆስፒታሉ የዋይፋይ ስም እና ፓስወርድ በሞባይል ስልክህ Browser ገብተህ በቀላሉ የምታስገባበት Captive Portal አለው።
* 📁 **`master_gateway_usb/master_gateway_usb.ino`**
  * **እንዴት ይሰራል?** በ USB ገመድ ከኮምፒውተር ጋር ተሰክቶ ከ Python Serial Bridge ጋር የሚሰራ።

---

### 📂 03_SUB_STATION (ተጨማሪ ንዑስ ነርስ ስቴሽኖች — ESP32 + NFC + TFT)
በሌሎች ክፍሎች (ዋርድ) ለሚቀመጡ ተጨማሪ ነርሶች የሚሆን መሳሪያ ነው። (LoRa አያስፈልገውም)

* 📁 **`sub_station_hud_terminal/sub_station_hud_terminal.ino`** 👈 **(የአንተ ያማረው Aerospace HUD UI!)**
  * **ጥቅሙ፦** Arduino Uno ላይ ስትጠቀምበት የነበረው ያማረው የሳይበርፐንክ/HUD ክብ አኒሜሽን እና የ NFC ካርድ ንባብ በ ESP32 ላይ እንዲሰራ የተስተካከለ።
* 📁 **`sub_station_wifi/sub_station_wifi.ino`**
  * በ Wi-Fi በቀጥታ ለ Cloud ሲስተም ምላሽ የሚሰጥ።
* 📁 **`sub_station_usb/sub_station_usb.ino`**
  * በ USB ገመድ ከኮምፒውተር ጋር ተገናኝቶ የሚሰራ።

---

### 📂 _OLD_UNO_ARCHIVE (የድሮ ፋይሎች ማህደር)
* ድሮ በ Arduino Uno/Nano ትጠቀምባቸው የነበሩ ፋይሎች ለታሪክ እና ለማጣቀሻነት እንዳይጠፉ እዚህ ተቀምጠዋል።
