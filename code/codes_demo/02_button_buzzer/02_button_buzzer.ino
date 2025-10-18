int buzzerPin = 3; // assign digital pin D3 on Arcade Nano to buzzer
int buttonLeft = 4; // assign digital pin D4 on Arcade Nano to Left Button
int buttonRight = 2; // assign digital pin D2 on Arcade Nano to Left Button

unsigned long lastButtonLeftPress = 0; // create a variable for storing button state and assign an initial value to it
unsigned long lastButtonRightPress = 0; // create a variable for storing button state and assign an initial value to it

int debounceDelay = 200;  // debounce delay in milliseconds. This is used to avoid multiple button press detection

void setup() {
  pinMode(buzzerPin, OUTPUT);  // buzzerPin defined as digital output with no internal pull-up because using external resistor
  pinMode(buttonLeft, INPUT);  // similar as above
  pinMode(buttonRight, INPUT);  // similar as above
}

void loop() {
  // Read button Left and handle debounce
  if (digitalRead(buttonLeft) == HIGH && (millis() - lastButtonLeftPress) > debounceDelay) {
    beep(1); // Left button beeps once
    lastButtonLeftPress = millis();
  }

  // Read button Right and handle debounce
  if (digitalRead(buttonRight) == HIGH && (millis() - lastButtonRightPress) > debounceDelay) {
    beep(2); //Right button beeps twice
    lastButtonRightPress = millis();
  }
}

  // This defines the BEEP function to act according to number of beeps needed. COUNT is the number of times we want beep to run.
void beep(int count) {
  for (int i = 0; i < count; i++) {
    digitalWrite(buzzerPin, HIGH);
    delay(100);
    digitalWrite(buzzerPin, LOW);
    delay(100);
  }
}
