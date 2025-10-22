/* =============================================================================
   POTENTIOMETER TEST
   Hardware: Potentiometer wiper → A0
   Purpose : Read analog value and display on Serial Monitor
   =============================================================================
*/

#define POT_PIN A0

void setup() {
  Serial.begin(9600);
}

void loop() {
  int potValue = analogRead(POT_PIN);
  Serial.print("Potentiometer: ");
  Serial.println(potValue);
  delay(300);
}
