// ================================================================
//  MedPulse Smart-Care AI — Patient Room LoRa Transmitter
//  Hardware: Arduino (Uno / Nano) + LoRa E32-433T20D + Buttons + LED + Buzzer
//  Company:  ACRMA TECH SOLUTION PLC
// ================================================================

#include <SoftwareSerial.h>

// ----------------------------------------------------------------
// 1. የክፍል መለያ ቁጥር (Room Configuration)
//    እርስዎ በሚሞክሩት ወይም በሚያዘጋጁት ክፍል ቁጥር ይቀይሩት (ለምሳሌ 4, 10, 11...)
// ----------------------------------------------------------------
const int ROOM_ID = 10; 

// ----------------------------------------------------------------
// 2. የፒን መገናኛዎች (Pinout Configuration)
// ----------------------------------------------------------------
const int BED1_BTN          = 2;  // አልጋ 1 አዝራር (ወደ GND)
const int BED2_BTN          = 5;  // አልጋ 2 አዝራር (ወደ GND)
const int BED3_BTN          = 6;  // አልጋ 3 አዝራር (ወደ GND)
const int TOILET_BTN        = 7;  // መጸዳጃ ቤት አዝራር (ወደ GND)
const int NURSE_ARRIVED_BTN = 8;  // ነርስ ደረሰች ማረጋገጫ አዝራር (ወደ GND)

const int CONFIRM_LED       = 4;  // ጥሪ ማሳወቂያ መብራት (LED Pin 4)
const int ROOM_BUZZER       = 3;  // ድምጽ ማጉያ (Buzzer Pin 3 - Active or Passive)

// LoRa E32 SoftwareSerial (Pin 10 -> E32 TXD, Pin 11 -> E32 RXD)
SoftwareSerial e32Serial(10, 11); 

// ----------------------------------------------------------------
// 3. የ LED አሰራር አይነት (Active-HIGH / Active-LOW)
//    ተራ LED ከሆነ true ይሁን (HIGH ሲሆን ይበራል)
//    የተገዛ የ LED Module ሆኖ LOW ሲሆን የሚበራ ከሆነ false ያድርጉት
// ----------------------------------------------------------------
const bool LED_ACTIVE_HIGH = true; 

// ለእያንዳንዱ Button መደጋገም መከላከያ (Cooldown / Debounce) ሰዓት መያዣ
unsigned long b1_time = 0;
unsigned long b2_time = 0;
unsigned long b3_time = 0;
unsigned long t_time  = 0;
unsigned long a_time  = 0;

// የድምጽ እና የ LED መቆጣጠሪያ ተግባራት (Function Prototypes)
void setCallLed(bool turnOn);
void playBuzzerBeep(unsigned int frequency, unsigned long durationMs);
void playNurseAcknowledgedTone();
void playNurseArrivedTone();
void sendCall(String location);

void setup() {
  Serial.begin(9600);
  e32Serial.begin(9600);
  
  // Timeout ማሳጠር (ኮዱ እንዳይቆም እና ፈጣን ምላሽ እንዲሰጥ)
  e32Serial.setTimeout(30);
  
  // የአዝራሮች ፒን (INPUT_PULLUP - አዝራሩ ሲጫን ከ GND ጋር ይገናኛል)
  pinMode(BED1_BTN, INPUT_PULLUP);
  pinMode(BED2_BTN, INPUT_PULLUP);
  pinMode(BED3_BTN, INPUT_PULLUP);
  pinMode(TOILET_BTN, INPUT_PULLUP);
  pinMode(NURSE_ARRIVED_BTN, INPUT_PULLUP);
  
  // የ LED እና Buzzer ፒን
  pinMode(CONFIRM_LED, OUTPUT);
  pinMode(ROOM_BUZZER, OUTPUT);
  
  setCallLed(false);
  noTone(ROOM_BUZZER);
  digitalWrite(ROOM_BUZZER, LOW);
  
  Serial.println(F("\n=================================================="));
  Serial.println(F("  MedPulse Smart-Care AI — Patient Room Caller    "));
  Serial.print(F("  Configured Room ID: "));
  Serial.println(ROOM_ID);
  Serial.println(F("=================================================="));
  
  // --------------------------------------------------------------
  // ቦርዱ ሲበራ የሚሰራ ራስን የመፈተሻ ሙከራ (Power-On Self Test)
  // LED እና Buzzer በትክክል መገናኘታቸውን በ 1 አጭር ብልጭታ እና ድምጽ ያረጋግጣል
  // --------------------------------------------------------------
  Serial.println(F("[SELF-TEST] Testing LED and Buzzer..."));
  setCallLed(true);
  playBuzzerBeep(2500, 120);
  setCallLed(false);
  delay(100);
  setCallLed(true);
  delay(120);
  setCallLed(false);
  Serial.println(F("[SELF-TEST] Ready! Waiting for patient calls..."));
}

void loop() {
  unsigned long now = millis();

  // 1. ጥሪ መላክ (የአልጋ እና የመጸዳጃ ቤት አዝራሮች)
  if (digitalRead(BED1_BTN) == LOW && (now - b1_time > 2000)) { 
    b1_time = now; 
    sendCall("Bed 1"); 
  }
  if (digitalRead(BED2_BTN) == LOW && (now - b2_time > 2000)) { 
    b2_time = now; 
    sendCall("Bed 2"); 
  }
  if (digitalRead(BED3_BTN) == LOW && (now - b3_time > 2000)) { 
    b3_time = now; 
    sendCall("Bed 3"); 
  }
  if (digitalRead(TOILET_BTN) == LOW && (now - t_time > 2000)) { 
    t_time = now; 
    sendCall("Bathroom"); 
  }

  // 2. ነርሷ ክፍል ውስጥ በአካል ስትደርስ አዝራር (Pin 8) ስትጫን
  if (digitalRead(NURSE_ARRIVED_BTN) == LOW && (now - a_time > 2000)) {
    a_time = now;
    
    // ጥሪው ስለተመለሰ መብራቱን ያጠፋል
    setCallLed(false);
    
    // ነርሷ መድረሷን በክፍሉ ውስጥ በ 2 ፈጣን ድምጽ ማረጋገጥ
    playNurseArrivedTone();
    
    // የመጀመሪያ ጥሪ ወደ Master Gateway
    String msg = "ARRIVED:" + String(ROOM_ID);
    e32Serial.println(msg);
    Serial.println("Nurse Arrived Sent (1) -> " + msg);
    
    // Collision Avoidance (TDMA)
    delay(ROOM_ID * 80);
    
    // የባካፕ ጥሪ
    e32Serial.println(msg);
    Serial.println("Nurse Arrived Sent (2) -> " + msg);
  }

  // 3. ነርሷ ስቴሽን ላይ NFC Card ስታስነካ ወይም Dashboard ላይ Acknowledge ስታደርግ የሚመጣ መልስ
  if (e32Serial.available() > 0) {
    String feedback = e32Serial.readStringUntil('\n');
    feedback.trim();
    
    if (feedback.length() > 0) {
      Serial.print(F("[LORA IN] Received: "));
      Serial.println(feedback);
      
      String targetDone = "DONE:" + String(ROOM_ID);
      // ለዚህ ክፍል ወይም ለሁሉም የተላከ መሆኑን ማረጋገጥ
      if (feedback.indexOf(targetDone) != -1 || feedback.indexOf("DONE:ALL") != -1 || feedback.indexOf("DONE:0") != -1) {
        Serial.println(F("[ACK CONFIRMED] Nurse acknowledged call! Silencing LED and alerting patient."));
        
        // 1. መብራቱን ያጠፋል (ጥሪው መመለሱን ያሳያል)
        setCallLed(false);
        
        // 2. ታካሚው ነርሷ ምላሽ እንደሰጠች እንዲያውቅ 3 የደስታ ፈጣን የቢፕ ድምጾች ማሰማት
        playNurseAcknowledgedTone();
      }
    }
  }
}

// ----------------------------------------------------------------
// ጥሪ መላኪያ እና ማሳወቂያ ተግባር (Send Call)
// ----------------------------------------------------------------
void sendCall(String location) {
  // 1. መብራቱን ወዲያው ማብራት
  setCallLed(true);
  
  // 2. ታካሚው አዝራሩ በትክክል መስራቱን እንዲረዳ አጭር የቢፕ ድምጽ (Chirp)
  playBuzzerBeep(2600, 80);
  
  String callMsg = "START:" + String(ROOM_ID) + ":" + location;
  
  // 3. 1ኛ የመጀመሪያ ጥሪ በ LoRa መላክ
  e32Serial.println(callMsg);
  Serial.println("Sent (1) -> " + callMsg);
  
  // 4. መጋጨት ካለ ለማስተካከል በእያንዳንዱ ክፍል የተለያየ ሰዓት መጠበቅ (TDMA - Collision Avoidance)
  delay(ROOM_ID * 100); 
  
  // 5. 2ኛ ሁለተኛ ጥሪ (Backup - መንገዱ ሲለቀቅለት በሰላም ይደርሳል)
  e32Serial.println(callMsg);
  Serial.println("Sent (2) -> " + callMsg);
}

// ----------------------------------------------------------------
// የ LED ማብሪያ እና ማጥፊያ ተግባር (Active-HIGH እና Active-LOW የሚደግፍ)
// ----------------------------------------------------------------
void setCallLed(bool turnOn) {
  if (LED_ACTIVE_HIGH) {
    digitalWrite(CONFIRM_LED, turnOn ? HIGH : LOW);
  } else {
    digitalWrite(CONFIRM_LED, turnOn ? LOW : HIGH);
  }
}

// ----------------------------------------------------------------
// ሁለንተናዊ የ Buzzer ድምጽ ማሰሚያ (Active እና Passive Buzzer ሁለቱንም የሚደግፍ)
// ----------------------------------------------------------------
void playBuzzerBeep(unsigned int frequency, unsigned long durationMs) {
  // Passive Buzzer ከሆነ ፍሪኩዌንሲ ያወጣል፤ Active Buzzer ከሆነ ደግሞ በ HIGH ይጮሃል
  tone(ROOM_BUZZER, frequency);
  digitalWrite(ROOM_BUZZER, HIGH);
  delay(durationMs);
  noTone(ROOM_BUZZER);
  digitalWrite(ROOM_BUZZER, LOW);
}

// ----------------------------------------------------------------
// ነርሷ NFC ካርድ ስታነብ ለታካሚው የሚሰማ 3 ፈጣን የደስታ ድምጽ (Beep-Beep-Beep)
// ----------------------------------------------------------------
void playNurseAcknowledgedTone() {
  for (int i = 0; i < 3; i++) {
    playBuzzerBeep(2200, 180);
    delay(100);
  }
}

// ----------------------------------------------------------------
// ነርሷ ክፍል ውስጥ በአካል ስትደርስ የሚሰማ የ 2 ድምጽ ማረጋገጫ
// ----------------------------------------------------------------
void playNurseArrivedTone() {
  playBuzzerBeep(2400, 100);
  delay(60);
  playBuzzerBeep(2800, 180);
}
