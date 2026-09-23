// ================================================================
//  MedPulse Smart-Care AI — Patient Room LoRa Transmitter
//  Hardware: Arduino (Uno / Nano) + LoRa E32-433T20D + Buttons + LED + Buzzer
//  Company:  ACRMA TECH SOLUTION PLC
// ================================================================

#include <SoftwareSerial.h>

// እያንዳንዱ ክፍል የራሱ የሆነ ቁጥር ሊኖረው ይገባል (ለምሳሌ 4, 10, 11...)
const int ROOM_ID = 10; 

// Pins
const int BED1_BTN          = 2;  // አልጋ 1 አዝራር
const int BED2_BTN          = 5;  // አልጋ 2 አዝራር
const int BED3_BTN          = 6;  // አልጋ 3 አዝራር
const int TOILET_BTN        = 7;  // መጸዳጃ ቤት አዝራር
const int NURSE_ARRIVED_BTN = 8;  // ነርስ ደረሰች ማረጋገጫ አዝራር

const int CONFIRM_LED       = 4;  // ጥሪ ማሳወቂያ መብራት (Pin 4)
const int ROOM_BUZZER       = 3;  // ድምጽ ማጉያ (Pin 3 - Active or Passive)

SoftwareSerial e32Serial(10, 11); // Pin 10 = RX, Pin 11 = TX

// የጥሪ ሁኔታ መቆጣጠሪያ (ጥሪ ሲደረግ true ይሆናል፤ ነርሷ ስትቀበል false ሆኖ ይጠፋል)
bool isCallActive = false;

// ለእያንዳንዱ Button መደጋገም መከላከያ (Cooldown) ሰዓት መያዣ
unsigned long b1_time = 0;
unsigned long b2_time = 0;
unsigned long b3_time = 0;
unsigned long t_time  = 0;
unsigned long a_time  = 0;

void sendCall(String location);

void setup() {
  Serial.begin(9600);
  e32Serial.begin(9600);
  
  // Timeout ማሳጠር (ኮዱ እንዳይቆም እና ፈጣን እንዲሆን)
  e32Serial.setTimeout(50);
  
  pinMode(BED1_BTN, INPUT_PULLUP);
  pinMode(BED2_BTN, INPUT_PULLUP);
  pinMode(BED3_BTN, INPUT_PULLUP);
  pinMode(TOILET_BTN, INPUT_PULLUP);
  pinMode(NURSE_ARRIVED_BTN, INPUT_PULLUP);
  
  pinMode(CONFIRM_LED, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(ROOM_BUZZER, OUTPUT);
  
  // --------------------------------------------------------------
  // ቦርዱ ሲበራ መብራቱ እና ቡዘሩ መስራታቸውን በ 200ms ፈትሾ ማረጋገጥ
  // (መብራቱ ብልጭ ብሎ ቡዘሩ ድምጽ ካሰማ ሽቦዎቹ በትክክል ተገናኝተዋል)
  // --------------------------------------------------------------
  digitalWrite(CONFIRM_LED, HIGH);
  digitalWrite(LED_BUILTIN, HIGH);
  tone(ROOM_BUZZER, 2200);
  digitalWrite(ROOM_BUZZER, HIGH);
  delay(200);
  digitalWrite(CONFIRM_LED, LOW);
  digitalWrite(LED_BUILTIN, LOW);
  noTone(ROOM_BUZZER);
  digitalWrite(ROOM_BUZZER, LOW);
  
  Serial.println("Room " + String(ROOM_ID) + " Transmitter Ready");
}

void loop() {
  unsigned long now = millis();

  // 1. ጥሪ መላክ (የአልጋ እና የመጸዳጃ ቤት አዝራሮች)
  if (digitalRead(BED1_BTN) == LOW && (now - b1_time > 2000)) { b1_time = now; sendCall("Bed 1"); }
  if (digitalRead(BED2_BTN) == LOW && (now - b2_time > 2000)) { b2_time = now; sendCall("Bed 2"); }
  if (digitalRead(BED3_BTN) == LOW && (now - b3_time > 2000)) { b3_time = now; sendCall("Bed 3"); }
  if (digitalRead(TOILET_BTN) == LOW && (now - t_time > 2000)) { t_time = now; sendCall("Bathroom"); }

  // 2. ነርሷ ክፍል ውስጥ በአካል ስትደርስ (Pin 8 ሲጫን)
  if (digitalRead(NURSE_ARRIVED_BTN) == LOW && (now - a_time > 2000)) {
    a_time = now;
    isCallActive = false;
    
    // ነርሷ ስትደርስ መብራቱ ይጠፋል (Pin 4 እና onboard Pin 13)
    digitalWrite(CONFIRM_LED, LOW);
    digitalWrite(LED_BUILTIN, LOW);
    
    // የመጀመሪያ ጥሪ
    e32Serial.println("ARRIVED:" + String(ROOM_ID));
    Serial.println("Nurse Arrived - Sent (1)");
    
    // በክፍሉ ቁጥር ልክ የተለያየ ሰዓት መጠበቅ (Collision Avoidance)
    delay(ROOM_ID * 150);
    
    // የባካፕ ጥሪ
    e32Serial.println("ARRIVED:" + String(ROOM_ID));
    Serial.println("Nurse Arrived - Sent (2)");
  }

  // 3. ነርሷ ስቴሽን ሆና NFC ስታስነካ ወይም Dashboard ላይ Accept ስታደርግ የሚመጣ መልስ
  if (e32Serial.available() > 0) {
    String feedback = e32Serial.readStringUntil('\n');
    feedback.trim();
    
    // ከጌትዌይ የ DONE መልእክት ሲመጣ
    if (feedback.startsWith("DONE")) {
      // ይህ ክፍል ጥሪ አድርጎ እየጠበቀ ከሆነ ወይም የክፍሉ ቁጥር ከተመሳሰለ
      if (isCallActive || feedback.indexOf(String(ROOM_ID)) != -1 || feedback.indexOf("ALL") != -1) {
        isCallActive = false;
        
        // 1. መብራቱን ያጠፋል (Pin 4 እና onboard Pin 13 ይጠፋሉ)
        digitalWrite(CONFIRM_LED, LOW);
        digitalWrite(LED_BUILTIN, LOW);
        
        // 2. ነርሷ ጥሪውን እንደተቀበለችው ታካሚው እንዲያውቅ ቡዘሩ ለ 2 ሰከንድ ይጮሃል
        tone(ROOM_BUZZER, 2200);          // ለ Passive Buzzer
        digitalWrite(ROOM_BUZZER, HIGH);  // ለ Active Buzzer
        
        unsigned long startBuzz = millis();
        while(millis() - startBuzz < 2000) {
          // 2 ሰከንድ ይጠብቃል
        }
        noTone(ROOM_BUZZER);
        digitalWrite(ROOM_BUZZER, LOW);
        
        Serial.println("Nurse Accepted Call -> LED OFF, Buzzer Done!");
      }
    }
  }
}

void sendCall(String location) {
  // 1. ጥሪው ስለተደረገ መብራቱን ያበራል (Pin 4 እንዲሁም ቦርዱ ላይ ያለውን Pin 13 ያበራል)
  isCallActive = true;
  digitalWrite(CONFIRM_LED, HIGH); 
  digitalWrite(LED_BUILTIN, HIGH); 
  
  // 2. የመጀመሪያ ጥሪ በ LoRa ይላካል
  e32Serial.println("START:" + String(ROOM_ID) + ":" + location);
  Serial.println("Sent (1) -> START:" + String(ROOM_ID) + ":" + location);
  
  // 3. Collision Avoidance (TDMA)
  delay(ROOM_ID * 150); 
  
  // 4. ሁለተኛ ጥሪ (Backup)
  e32Serial.println("START:" + String(ROOM_ID) + ":" + location);
  Serial.println("Sent (2) -> START:" + String(ROOM_ID) + ":" + location);
}
