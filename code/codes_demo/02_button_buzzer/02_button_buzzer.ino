/*
This code is part of the ARCADE project for the "Electronics and Microcontrollers" skill of INNOVATION SCHOOL at MAKERS ASYLUM
Copyright : 2025 Maker's Asylum | makersasylum.com
License : MIT

  Project: Dual Button Buzzer Test
  Description:
    This program demonstrates simple digital input handling with 
    two push buttons and an output buzzer on an Arduino-based system. 
    Each button triggers a specific number of buzzer beeps to confirm 
    input detection.

  Hardware Setup:
    - Buzzer: Connected to digital pin D3 (active HIGH)
    - Left Button: Connected to digital pin D4 (active HIGH on press)
    - Right Button: Connected to digital pin D2 (active HIGH on press)
    - External resistors are used for proper button input handling.

  Operation:
    - When the left button is pressed, the buzzer produces one short beep.
    - When the right button is pressed, the buzzer produces two short beeps.
    - A software debounce delay (200 ms) prevents false multiple triggers caused by mechanical switch bounce.
    - The program continuously monitors both buttons in the main loop and responds immediately when a valid press is detected.
*/

int buzzerPin = 3; // assign digital pin D3 on Arcade Nano to buzzer
int buttonLeft = 4; // assign digital pin D4 on Arcade Nano to Left Button
int buttonRight = 2; // assign digital pin D2 on Arcade Nano to Right Button

unsigned long lastButtonLeftPress = 0; // create a variable for storing millis when the last button state changed for LEFT button
unsigned long lastButtonRightPress = 0; // create a variable for storing millis when the last button state changed for RIGHT button

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

  // This defines the BEEP function to act according to number of beeps needed. COUNT is the number of times we want beep to run. Once for Left button and Twice for Right button
void beep(int count) {
  for (int i = 0; i < count; i++) {
    digitalWrite(buzzerPin, HIGH);
    delay(100);
    digitalWrite(buzzerPin, LOW);
    delay(100);
  }
}
