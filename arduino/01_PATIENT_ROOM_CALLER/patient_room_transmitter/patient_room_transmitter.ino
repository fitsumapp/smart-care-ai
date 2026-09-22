// ================================================================
//  MedPulse Smart-Care AI — Patient Room LoRa Transmitter
//  Hardware: Arduino (Uno/Nano) + LoRa E32 + Buttons + LED + Buzzer
//  Company:  ACRMA TECH SOLUTION PLC
// ================================================================

#include <SoftwareSerial.h>

// እያንዳንዱ ክፍል የራሱ የሆነ ቁጥር ሊኖረው ይገባል (ለምሳሌ 4, 5, 10, 11...)
const int ROOM_ID = 10; 

// Pins
const int BED1_BTN          = 2;
const int BED2_BTN          = 5;
const int BED3_BTN          = 6;
const int TOILET_BTN        = 7;
const int NURSE_ARRIVED_BTN = 8; 

const int CONFIRM_LED       = 4; 
const int ROOM_BUZZER       = 3; 

// LoRa E32 SoftwareSerial
SoftwareSerial e32Serial(10, 11); 

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
  pinMode(ROOM_BUZZER, OUTPUT);
  
  digitalWrite(CONFIRM_LED, LOW);
  digitalWrite(ROOM_BUZZER, LOW);
  
  Serial.println("Room " + String(ROOM_ID) + " Transmitter Ready");
}

void loop() {
  unsigned long now = millis();

  // 1. ጥሪ መላክ (የአልጋ እና የሽንት ቤት አዝራሮች)
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

  // 2. ነርሷ ክፍል መድረሷን ማሳወቅ
  if (digitalRead(NURSE_ARRIVED_BTN) == LOW && (now - a_time > 2000)) {
    a_time = now;
    
    // የመጀመሪያ ጥሪ
    e32Serial.println("ARRIVED:" + String(ROOM_ID));
    Serial.println("Nurse Arrived - Sent (1)");
    
    // በክፍሉ ቁጥር ልክ የተለያየ ሰዓት መጠበቅ (Collision Avoidance - TDMA)
    delay(ROOM_ID * 150);
    
    // የባካፕ ጥሪ
    e32Serial.println("ARRIVED:" + String(ROOM_ID));
    Serial.println("Nurse Arrived - Sent (2)");
  }

  // 3. ነርሷ ስቴሽን ሆና ካርድ ስታስነካ (NFC Scan) የሚመጣ የ Reset መልስ
  if (e32Serial.available() > 0) {
    String feedback = e32Serial.readStringUntil('\n');
    feedback.trim();
    
    if (feedback == "DONE:" + String(ROOM_ID)) {
      digitalWrite(CONFIRM_LED, LOW);   // መብራቱ ይጠፋል
      digitalWrite(ROOM_BUZZER, HIGH);  // ነርሷ ጥሪውን እንደተቀበለችው በሽተኛውን ለማሳወቅ
      
      unsigned long startBuzz = millis();
      // Buzzer ኮዱን block እንዳያደርገው non-blocking መዘግየት (2 ሰከንድ)
      while(millis() - startBuzz < 2000) {
        // ይጠብቃል
      }
      digitalWrite(ROOM_BUZZER, LOW);
    }
  }
}

void sendCall(String location) {
  // በክፍሉ ውስጥ መብራቱን ያበራል
  digitalWrite(CONFIRM_LED, HIGH); 
  
  // 1ኛ. የመጀመሪያ ጥሪ
  e32Serial.println("START:" + String(ROOM_ID) + ":" + location);
  Serial.println("Sent (1) -> START:" + String(ROOM_ID) + ":" + location);
  
  // 2ኛ. መጋጨት ካለ ለማስተካከል በእያንዳንዱ ክፍል የተለያየ ሰዓት መጠበቅ (TDMA - Collision Avoidance)
  delay(ROOM_ID * 150); 
  
  // 3ኛ. ሁለተኛ ጥሪ (Backup)
  e32Serial.println("START:" + String(ROOM_ID) + ":" + location);
  Serial.println("Sent (2) -> START:" + String(ROOM_ID) + ":" + location);
}
