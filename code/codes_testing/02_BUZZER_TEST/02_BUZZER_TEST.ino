/* =============================================================================
   BUZZER TEST
   Hardware: Buzzer driven by NPN transistor, base connected to D3
   Purpose : Verify buzzer functionality using simple beeps.
   =============================================================================
*/

#define BUZZER_PIN 3

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
}

void loop() {
  digitalWrite(BUZZER_PIN, HIGH);  // Turn buzzer ON
  delay(200);
  digitalWrite(BUZZER_PIN, LOW);   // Turn buzzer OFF
  delay(800);                      // Wait before next beep
}
