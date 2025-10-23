/*
===============================================================================
This code is part of the ARCADE project for the "Electronics and 
Microcontrollers" skill of INNOVATION SCHOOL at MAKERS ASYLUM.

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
    - A software debounce delay (200 ms) prevents false multiple triggers 
      caused by mechanical switch bounce.
    - The program continuously monitors both buttons in the main loop and 
      responds immediately when a valid press is detected.
===============================================================================
*/

int buzzerPin = 3;          // Arduino output pin connected to NPN base (through 1k resistor)
int buttonLeft = 4;         // Digital input for LEFT button (pull-down resistor → reads HIGH when pressed)
int buttonRight = 2;        // Digital input for RIGHT button (pull-down resistor → reads HIGH when pressed)

// Timestamps to handle software debounce
unsigned long lastButtonLeftPress = 0;
unsigned long lastButtonRightPress = 0;

int debounceDelay = 200;    // Minimum time (ms) between valid presses

// -------------------------
// setup() - Runs once
// -------------------------
void setup() {
  pinMode(buzzerPin, OUTPUT);     // Configure buzzer pin as output (drives NPN base)
  digitalWrite(buzzerPin, LOW);   // Ensure buzzer starts OFF (transistor OFF)

  pinMode(buttonLeft, INPUT);     // Buttons with hardware pull-downs → use INPUT (not INPUT_PULLUP)
  pinMode(buttonRight, INPUT);

  // Optional: Serial monitor for debugging
  // Serial.begin(9600);
  // Serial.println("System Ready - Dual Button Buzzer with NPN Driver");
}

// -------------------------
// loop() - Main polling loop
// -------------------------
void loop() {
  // Read both button states
  int leftState = digitalRead(buttonLeft);
  int rightState = digitalRead(buttonRight);

  // ---------- LEFT BUTTON ----------
  // If pressed (HIGH) and debounce period has passed, trigger 1 beep
  if (leftState == HIGH && (millis() - lastButtonLeftPress) > debounceDelay) {
    beep(1);                           // Single short beep
    lastButtonLeftPress = millis();    // Update last press timestamp
  }

  // ---------- RIGHT BUTTON ----------
  // If pressed (HIGH) and debounce period has passed, trigger 2 beeps
  if (rightState == HIGH && (millis() - lastButtonRightPress) > debounceDelay) {
    beep(2);                           // Two short beeps
    lastButtonRightPress = millis();   // Update last press timestamp
  }

  // Note: No code runs during beep() (blocking). For real-time response, 
  // replace delay() with millis()-based timing in beep().
}

// -------------------------
// beep(count)
// Produce 'count' short beeps via transistor-driven buzzer.
// HIGH turns transistor ON → current flows → buzzer ON.
// -------------------------
void beep(int count) {
  for (int i = 0; i < count; i++) {
    digitalWrite(buzzerPin, HIGH);  // Turn transistor ON → buzzer sounds
    delay(100);                     // Beep duration
    digitalWrite(buzzerPin, LOW);   // Turn transistor OFF → buzzer silent
    delay(100);                     // Short gap before next beep
  }
}
