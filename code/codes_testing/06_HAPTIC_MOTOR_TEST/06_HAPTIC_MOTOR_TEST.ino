/* =============================================================================
   HAPTIC MOTOR TEST
   Hardware: Motor connected to D5
   Purpose : Test vibration motor feedback with short pulses
   =============================================================================
*/

#define HAPTIC_PIN 5

void setup() {
  pinMode(HAPTIC_PIN, OUTPUT);
}

void loop() {
  digitalWrite(HAPTIC_PIN, HIGH);  // Vibrate ON
  delay(200);
  digitalWrite(HAPTIC_PIN, LOW);   // Vibrate OFF
  delay(800);
}
