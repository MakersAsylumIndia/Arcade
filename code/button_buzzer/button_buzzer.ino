#define buzzerPin 3
#define button4Pin 4
#define button2Pin 2

unsigned long lastButton4Press = 0;
unsigned long lastButton2Press = 0;

unsigned long debounceDelay = 200;  // debounce delay in milliseconds

void setup() {
  pinMode(buzzerPin, OUTPUT);
  pinMode(button4Pin, INPUT);  // No internal pull-up because you're using a pull-down resistor
  pinMode(button2Pin, INPUT);  // Same as above
}

void loop() {
  // Read button 4 and handle debounce
  if (digitalRead(button4Pin) == HIGH && (millis() - lastButton4Press) > debounceDelay) {
    beep(1);
    lastButton4Press = millis();
  }

  // Read button 2 and handle debounce
  if (digitalRead(button2Pin) == HIGH && (millis() - lastButton2Press) > debounceDelay) {
    beep(2);
    lastButton2Press = millis();
  }
}

void beep(int count) {
  for (int i = 0; i < count; i++) {
    digitalWrite(buzzerPin, HIGH);
    delay(100);
    digitalWrite(buzzerPin, LOW);
    delay(100);
  }
}
