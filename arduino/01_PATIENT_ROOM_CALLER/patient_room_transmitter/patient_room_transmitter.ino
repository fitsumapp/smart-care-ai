#include <SoftwareSerial.h>

// እያንዳንዱ ክፍል የራሱ የሆነ ቁጥር ሊኖረው ይገባል (ለምሳሌ 4, 10, 11...)
// ማሳሰቢያ፡ በሙከራ ወቅት ክፍል 4 እየተጠቀሙ ከሆነ ይህንን 4 ያድርጉት!
const int ROOM_ID = 10; 

// Pins
const int BED1_BTN = 2;
const int BED2_BTN = 5;
const int BED3_BTN = 6;
const int TOILET_BTN = 7;
const int NURSE_ARRIVED_BTN = 8; 

const int CONFIRM_LED = 4; 
const int ROOM_BUZZER = 3; 

// የ LED አሰራር (ተራ LED ከሆነ HIGH ሲሆን ይበራል፤ የተገዛ LED Module ከሆነ LOW ሊሆን ይችላል)
const int LED_ON  = HIGH; 
const int LED_OFF = LOW;  

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
  
  digitalWrite(CONFIRM_LED, LED_OFF);
  digitalWrite(ROOM_BUZZER, LOW);
  noTone(ROOM_BUZZER);
  
  // ቦርዱ ሲበራ LED መብራቱ መስራቱን ማረጋገጫ (አጭር ብልጭታ)
  digitalWrite(CONFIRM_LED, LED_ON);
  delay(150);
  digitalWrite(CONFIRM_LED, LED_OFF);
  
  Serial.println("Room " + String(ROOM_ID) + " Transmitter Ready");
}

void loop() {
  unsigned long now = millis();

  // 1. ጥሪ መላክ (ሁሉንም እኩል መጫን እንዲቻል 'else if' አጥፍተን በ 'if' ቀይረነዋል)
  // እያንዳንዱ Button ከተጫነ በኋላ ለ 2 ሰከንድ (2000ms) ሌላ ጥሪ አይልክም (Debounce)
  if (digitalRead(BED1_BTN) == LOW && (now - b1_time > 2000)) { b1_time = now; sendCall("Bed 1"); }
  if (digitalRead(BED2_BTN) == LOW && (now - b2_time > 2000)) { b2_time = now; sendCall("Bed 2"); }
  if (digitalRead(BED3_BTN) == LOW && (now - b3_time > 2000)) { b3_time = now; sendCall("Bed 3"); }
  if (digitalRead(TOILET_BTN) == LOW && (now - t_time > 2000)) { t_time = now; sendCall("Bathroom"); }

  // 2. ነርሷ መድረሷን ማሳወቅ (ሰዓት ለማቆም)
  if (digitalRead(NURSE_ARRIVED_BTN) == LOW && (now - a_time > 2000)) {
    a_time = now;
    
    // ነርሷ ስትደርስ ጥሪው ስለተመለሰ መብራቱ ይጠፋል
    digitalWrite(CONFIRM_LED, LED_OFF);
    
    // የመጀመሪያ ጥሪ
    e32Serial.println("ARRIVED:" + String(ROOM_ID));
    Serial.println("Nurse Arrived - Sent (1)");
    
    // በክፍሉ ቁጥር ልክ የተለያየ ሰዓት መጠበቅ (Collision Avoidance)
    delay(ROOM_ID * 150);
    
    // የባካፕ ጥሪ
    e32Serial.println("ARRIVED:" + String(ROOM_ID));
    Serial.println("Nurse Arrived - Sent (2)");
  }

  // 3. ነርሷ ስቴሽን ሆና NFC ስታስነካ ወይም "Enter" ስትጫን የሚመጣ መልስ
  if (e32Serial.available() > 0) {
    String feedback = e32Serial.readStringUntil('\n');
    feedback.trim();
    
    // ለዚህ ክፍል ወይም ለሁሉም የተላከ መሆኑን ማረጋገጥ (DONE:10 ወይም DONE:ALL)
    if (feedback.indexOf("DONE:" + String(ROOM_ID)) != -1 || feedback.indexOf("DONE:ALL") != -1) {
      digitalWrite(CONFIRM_LED, LED_OFF); // መብራቱ ይጠፋል
      
      // Buzzer ድምጽ (Active Buzzer በ HIGH ይጮሃል፤ Passive Buzzer ደግሞ በ tone ይጮሃል)
      tone(ROOM_BUZZER, 2000);
      digitalWrite(ROOM_BUZZER, HIGH);  // ነርሷ እየመጣች እንደሆነ ለማሳወቅ
      
      unsigned long startBuzz = millis();
      // Buzzer ኮዱን block እንዳያደርገው non-blocking መዘግየት (2 ሰከንድ)
      while(millis() - startBuzz < 2000) {
        // ምንም ሳያደርግ 2 ሰከንድ ይጠብቃል
      }
      noTone(ROOM_BUZZER);
      digitalWrite(ROOM_BUZZER, LOW);
    }
  }
}

void sendCall(String location) {
  // መብራቱን ያበራል
  digitalWrite(CONFIRM_LED, LED_ON); 
  
  // 1ኛ. የመጀመሪያ ጥሪ (ወዲያው ይላካል - ብቻውን ከሆነ ይደርሳል፣ እኩል ከተጫኑት አየር ላይ ይጋጫል)
  e32Serial.println("START:" + String(ROOM_ID) + ":" + location);
  Serial.println("Sent (1) -> START:" + String(ROOM_ID) + ":" + location);
  
  // 2ኛ. መጋጨት ካለ ለማስተካከል በእያንዳንዱ ክፍል የተለያየ ሰዓት ይጠብቃል (TDMA - Collision Avoidance)
  // ክፍል 4  -> 4 * 150 = 600ms
  // ክፍል 10 -> 10 * 150 = 1500ms
  delay(ROOM_ID * 150); 
  
  // 3ኛ. ሁለተኛ ጥሪ (Backup - መንገዱ ሲለቀቅለት በሰላም ይደርሳል)
  e32Serial.println("START:" + String(ROOM_ID) + ":" + location);
  Serial.println("Sent (2) -> START:" + String(ROOM_ID) + ":" + location);
}
