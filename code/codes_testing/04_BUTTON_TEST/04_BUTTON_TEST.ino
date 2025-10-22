/* =============================================================================
   BUTTON TEST
   Hardware: LEFT button (D4), RIGHT button (D2), both with pull-down resistors
   Purpose : Verify button presses using Serial Monitor
   =============================================================================
*/

#define LEFT_BTN 4
#define RIGHT_BTN 2

void setup() {
  pinMode(LEFT_BTN, INPUT);
  pinMode(RIGHT_BTN, INPUT);
  Serial.begin(9600);
}

void loop() {
  if (digitalRead(LEFT_BTN) == HIGH) {
    Serial.println("LEFT Button Pressed");
    delay(300);
  }

  if (digitalRead(RIGHT_BTN) == HIGH) {
    Serial.println("RIGHT Button Pressed");
    delay(300);
  }
}
