/* =============================================================================
   LDR TEST
   Hardware: LDR connected to A1 with pull-down resistor
   Purpose : Display analog light readings on Serial Monitor
   =============================================================================
*/

#define LDR_PIN A1

void setup() {
  Serial.begin(9600);
}

void loop() {
  int lightValue = analogRead(LDR_PIN);
  Serial.print("LDR Value: ");
  Serial.println(lightValue);
  delay(500);
}
