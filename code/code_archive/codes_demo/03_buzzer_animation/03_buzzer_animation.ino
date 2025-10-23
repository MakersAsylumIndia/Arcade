/* 
===============================================================================
This code is part of the ARCADE project for the "Electronics and 
Microcontrollers" skill of INNOVATION SCHOOL at MAKERS ASYLUM.

Copyright : 2025 Maker's Asylum | makersasylum.com
License : MIT

Summary of what this code does:
   Project: Dual-Button Buzzer with OLED Animation (Enhanced Graphics)

   PURPOSE:
   Demonstrates the use of two push-buttons to trigger a buzzer and display 
   animated visual feedback on an SH1106 OLED screen using U8glib. 
   Button 1 causes one beep, and Button 2 causes two beeps. 
   The OLED shows a speaker icon emitting animated “sound waves” that expand 
   smoothly to simulate real audio waves.

   HARDWARE SETUP:
   ---------------------------------------------------------------------------
   - BUTTON1 → Digital Pin 4 (with external pull-down resistor)
   - BUTTON2 → Digital Pin 2 (with external pull-down resistor)
   - BUZZER  → Digital Pin 3 (connected through NPN transistor)
   - OLED    → SH1106 128x64 I²C display
   ---------------------------------------------------------------------------

   OPERATION:
   - Press Button 1 → One beep + short wave animation
   - Press Button 2 → Two beeps + longer wave animation
   - The idle screen displays a muted speaker icon until a button is pressed.
   =============================================================================
*/

#include <U8glib.h>   // Include U8glib library for SH1106 OLED

// ---------------------------
// Pin Definitions
// ---------------------------
#define BUTTON1_PIN 4   // Button 1 (triggers 1 beep)
#define BUTTON2_PIN 2   // Button 2 (triggers 2 beeps)
#define BUZZER_PIN 3    // Buzzer output (via transistor base)

// ---------------------------
// OLED Display Object
// ---------------------------
U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);

// ---------------------------
// Debounce Variables
// ---------------------------
unsigned long lastDebounceTime1 = 0;
unsigned long lastDebounceTime2 = 0;
unsigned long debounceDelay = 200;  // 200 ms debounce time

bool lastButton1State = LOW;
bool lastButton2State = LOW;

// ---------------------------
// setup() — Initialize pins and startup display
// ---------------------------
void setup() {
  pinMode(BUTTON1_PIN, INPUT);
  pinMode(BUTTON2_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // Ensure buzzer is off

  // Display startup splash
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x13B);
    u8g.drawStr(25, 30, "Buzzer Test Ready");
  } while (u8g.nextPage());
  delay(1000);
}

// ---------------------------
// loop() — Continuously monitor button input
// ---------------------------
void loop() {
  handleButtons();   // Detect and process button presses
  drawIdleScreen();  // Default screen when idle
  delay(50);
}

// ---------------------------
// handleButtons() — Debounced button press detection
// ---------------------------
void handleButtons() {
  bool reading1 = digitalRead(BUTTON1_PIN);
  bool reading2 = digitalRead(BUTTON2_PIN);
  unsigned long now = millis();

  // Button 1 → One beep and short animation
  if (reading1 != lastButton1State && (now - lastDebounceTime1) > debounceDelay) {
    lastDebounceTime1 = now;
    if (reading1 == HIGH) {
      beep(1);
      animateWaves(1);
    }
  }
  lastButton1State = reading1;

  // Button 2 → Two beeps and extended animation
  if (reading2 != lastButton2State && (now - lastDebounceTime2) > debounceDelay) {
    lastDebounceTime2 = now;
    if (reading2 == HIGH) {
      beep(2);
      animateWaves(2);
    }
  }
  lastButton2State = reading2;
}

// ---------------------------
// beep(count) — generate tone using NPN transistor
// ---------------------------
void beep(int count) {
  for (int i = 0; i < count; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }
}

// ---------------------------
// drawIdleScreen() — display mute speaker when no button is pressed
// ---------------------------
void drawIdleScreen() {
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x10);
    u8g.drawStr(15, 10, "Press Btn1 or Btn2");

    // Speaker body
    u8g.drawTriangle(30, 30, 30, 50, 45, 40);

    // Muted “X” over the speaker
    u8g.drawLine(50, 30, 60, 50);
    u8g.drawLine(50, 50, 60, 30);
  } while (u8g.nextPage());
}

// ---------------------------
// animateWaves(type) — Display animated expanding sound waves
// ---------------------------
void animateWaves(int type) {
  // Determine animation duration and number of frames
  int waveCount = (type == 1) ? 2 : 3;     // Fewer waves for Button 1
  int maxRadius = (type == 1) ? 12 : 20;   // Expand farther for Button 2

  // Run animation loop
  for (int r = 0; r <= maxRadius; r += 2) { // Increment radius for smooth motion
    u8g.firstPage();
    do {
      u8g.setFont(u8g_font_6x10);

      // Display label for which button was pressed
      if (type == 1)
        u8g.drawStr(28, 10, "Btn 1: One Beep");
      else
        u8g.drawStr(28, 10, "Btn 2: Two Beeps");

      // Draw the speaker body (fixed)
      u8g.drawTriangle(30, 30, 30, 50, 45, 40);

      // Draw expanding waves: each wave spaced evenly
      for (int i = 0; i < waveCount; i++) {
        int offset = i * 4; // Space between waves
        int radius = r - offset;
        if (radius > 0) {
          u8g.drawCircle(50 + i, 40, radius); // Slight offset for realism
        }
      }

    } while (u8g.nextPage());

    delay(70); // Controls animation speed
  }

  // After animation ends, return to idle
  drawIdleScreen();
}
