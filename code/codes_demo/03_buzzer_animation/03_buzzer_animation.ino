/*
This code is part of the ARCADE project for the "Electronics and Microcontrollers" skill of INNOVATION SCHOOL at MAKERS ASYLUM
Copyright : 2025 Maker's Asylum | makersasylum.com
License : MIT

  Project: Buzzer Test with OLED Display
  Description:
  This program tests a buzzer module and displays its status 
  on a 128x64 SH1106 OLED screen. Two push buttons control 
  the buzzer — pressing either one activates the buzzer and 
  displays animated "sound waves" on the screen. Releasing 
  buttons stops the buzzer and shows a muted speaker icon.

  Hardware Setup:
  - Display: 128x64 SH1106 OLED (I2C connection)
  - Buttons: Connected between 5V and pins 2 & 4 with external pull-down resistors to GND
  - Buzzer:  Connected to pin 3 (active HIGH)
 
  Operation:
  - Press either button → buzzer turns ON, display shows sound waves.
  - Release either button → buzzer turns OFF, display shows a crossed-out speaker.
  - Loop continuously refreshes display and checks inputs with simple debouncing for stable operation.

  Libraries Required:
  - U8glib (for OLED display control)
*/


#include <U8glib.h>  // Include OLED display library

// -------------------- PIN DEFINITIONS --------------------
#define BUTTON1_PIN 4    // Button 1 input pin
#define BUTTON2_PIN 2    // Button 2 input pin
#define BUZZER_PIN 3     // Buzzer output pin

// -------------------- DISPLAY OBJECT --------------------
// Create SH1106 OLED display object (I2C mode)
U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);

// -------------------- STATE VARIABLES --------------------
bool buzzerActive = false;          // Tracks buzzer ON/OFF state
unsigned long lastCheck = 0;        // Last time we read button states
const unsigned long debounceDelay = 100; // Minimum delay between reads (ms)

// ---------------------------------------------------------------
// SETUP: Configure pins and initialize display
// ---------------------------------------------------------------
void setup() {
  // Buttons use external pull-downs → LOW when not pressed, HIGH when pressed
  pinMode(BUTTON1_PIN, INPUT);
  pinMode(BUTTON2_PIN, INPUT);

  // Set buzzer pin as output
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // Start with buzzer off
}

// ---------------------------------------------------------------
// LOOP: Periodically check buttons and update buzzer/display
// ---------------------------------------------------------------
void loop() {
  unsigned long currentMillis = millis(); // Get current time in ms

  // Only check inputs after debounce interval
  if (currentMillis - lastCheck > debounceDelay) {
    lastCheck = currentMillis;  // Update timestamp

    // For external pull-downs: pressed = HIGH, not pressed = LOW
    bool button1Pressed = (digitalRead(BUTTON1_PIN) == HIGH);
    bool button2Pressed = (digitalRead(BUTTON2_PIN) == HIGH);

    // Turn on buzzer if either button is pressed
    buzzerActive = (button1Pressed || button2Pressed);

    // Apply buzzer state
    digitalWrite(BUZZER_PIN, buzzerActive ? HIGH : LOW);
  }

  // Update OLED screen based on current buzzer state
  drawBuzzerTest(buzzerActive);

  // Small delay to limit display refresh rate and reduce flicker
  delay(50);
}

// ---------------------------------------------------------------
// FUNCTION: Draw OLED UI showing buzzer test status
// ---------------------------------------------------------------
void drawBuzzerTest(bool isActive) {
  // Start a new drawing page for SH1106
  u8g.firstPage();

  do {
    // Select a small readable font
    u8g.setFont(u8g_font_6x10);

    // Display instructions
    u8g.drawStr(15, 10, "Press any Button");

    // Draw a simple speaker icon (triangle)
    u8g.drawTriangle(30, 30, 30, 50, 45, 40);

    // If buzzer is active, draw "sound waves"
    if (isActive) {
      u8g.drawCircle(50, 40, 5);
      u8g.drawCircle(55, 40, 8);
      u8g.drawCircle(60, 40, 11);
    } 
    // If inactive, draw an "X" over the speaker
    else {
      u8g.drawLine(48, 30, 60, 50);
      u8g.drawLine(48, 50, 60, 30);
    }

  } while (u8g.nextPage()); // Repeat until all pages drawn
}
