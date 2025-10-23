/*
===============================================================================
This code is part of the ARCADE project for the "Electronics and 
Microcontrollers" skill of INNOVATION SCHOOL at MAKERS ASYLUM
Copyright : 2025 Maker's Asylum | makersasylum.com
License : MIT

Summary of what this code does:
 * Project: Ultrasonic Distance Display with Close-Proximity Warning (Buzzer)
 * Purpose:
   Measure distance with an HC-SR04 ultrasonic sensor, display the reading on a
   SH1106 128x64 OLED, and sound a warning buzzer when the object is either out
   of range (too far/invalid) or too close (within a defined safety threshold).

 * Hardware:
   - Ultrasonic HC-SR04:
       TRIG -> D7
       ECHO -> D6
   - SH1106 128x64 OLED (I2C):
       SDA -> A4, SCL -> A5
   - Passive buzzer:
       + -> D3, - -> GND

 * Operation:
    - Send a 10µs pulse on TRIG, measure echo on ECHO.
    - Convert echo time to centimeters.
    - Display distance on OLED (or "Out of range").
    - If distance is > MAX_DIST (too far / might be invalid) OR
      distance <= CLOSE_THRESHOLD (object too close) OR invalid (<=0),
      turn buzzer ON as a warning. Otherwise keep buzzer OFF.
 
 * Notes:
   - Choose CLOSE_THRESHOLD according to sensor physical limits.
   - HC-SR04 typical reliable range: 2–400 cm. Minimum reliable reading ~2–3 cm
 ===============================================================================
 */

#include <U8glib.h>               // U8glib for SH1106 OLED display

// === OLED INITIALIZATION ===
U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);  // Create display object (I2C, default options)

// === Pin definitions ===
#define trigPin 7    // TRIG pin for ultrasonic sensor (output from Arduino)
#define echoPin 6    // ECHO pin for ultrasonic sensor (input to Arduino)
#define buzzerPin 3  // Buzzer control pin (active HIGH)

// === Distance thresholds ===
const int MAX_DIST = 50;        // If measured distance >= this, consider "out of range / far"
const int CLOSE_THRESHOLD = 5;  // If measured distance <= this, consider "too close" (warning)

// ----------------------
// Setup: runs once
// ----------------------
void setup() {
  pinMode(trigPin, OUTPUT);     // TRIG must be OUTPUT to send pulses
  pinMode(echoPin, INPUT);      // ECHO must be INPUT to read echo pulse
  pinMode(buzzerPin, OUTPUT);   // Buzzer is an OUTPUT we drive HIGH/LOW
  digitalWrite(buzzerPin, LOW); // Ensure buzzer starts OFF
}

// ----------------------
// Main loop
// ----------------------
void loop() {
  long duration;     // Time for echo pulse in microseconds (round-trip)
  long distance;     // Calculated distance in centimeters (use long to match duration scale)

  // ----- Trigger ultrasonic pulse -----
  digitalWrite(trigPin, LOW);        // Ensure a clean LOW pulse start
  delayMicroseconds(2);              // Wait 2 µs to stabilize
  digitalWrite(trigPin, HIGH);       // Send the trigger pulse (HIGH)
  delayMicroseconds(10);             // Keep HIGH for 10 µs as required by HC-SR04
  digitalWrite(trigPin, LOW);        // End trigger pulse

  // ----- Read echo duration -----
  // pulseIn waits for the pin to go HIGH and measures how long it stays HIGH (µs)
  duration = pulseIn(echoPin, HIGH);

  // ----- Convert duration to distance (cm) -----
  // speed of sound ≈ 343 m/s => 0.0343 cm/µs. Using 0.034 gives similar result.
  // distance = (duration_us * speed_cm_per_us) / 2
  // Use float calculation then convert to long to reduce integer truncation artifacts.
  float dist_cm = (duration * 0.0343f) / 2.0f;  // intermediate float for accuracy
  distance = (long)(dist_cm + 0.5f);            // round to nearest cm (add 0.5 before casting)

  // ----- OLED display: render distance or out-of-range message -----
  u8g.firstPage();                             // Begin paged drawing for U8glib
  do {
    u8g.setFont(u8g_font_6x13B);               // Bold-ish font for title
    u8g.drawStr(0, 15, "Ultrasonic Sensor:");  // Title text

    u8g.setFont(u8g_font_6x10);                // Smaller font for data lines

    // If distance is invalid (<=0) OR beyond MAX_DIST, show out of range.
    // Note: invalid reading can be duration==0 (no echo) -> dist_cm==0
    if (distance <= 0 || distance >= MAX_DIST) {
      u8g.drawStr(0, 40, "Distance: Out of range");
    } else {
      // Otherwise print the numeric distance
      char buf[24];
      sprintf(buf, "Distance: %ld cm", distance);
      u8g.drawStr(0, 40, buf);
    }
  } while (u8g.nextPage());                   // Finish all pages

  // ----- Buzzer control (FIXED) -----
  // Turn buzzer ON when:
  //   1) reading invalid or object too far (distance <=0 OR >= MAX_DIST), OR
  //   2) object is too close (distance <= CLOSE_THRESHOLD)
  //
  // This covers "too close" which the previous code missed.
  if (distance <= 0 || distance >= MAX_DIST || distance <= CLOSE_THRESHOLD) {
    digitalWrite(buzzerPin, HIGH);  // Activate buzzer (warning)
  } else {
    digitalWrite(buzzerPin, LOW);   // Silence buzzer (normal)
  }

  // Wait half a second between readings to limit update rate
  delay(500);
}

