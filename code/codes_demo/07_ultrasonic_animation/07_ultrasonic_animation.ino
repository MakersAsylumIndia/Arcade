/*
===============================================================================
This code is part of the ARCADE project for the "Electronics and 
Microcontrollers" skill of INNOVATION SCHOOL at MAKERS ASYLUM
Copyright : 2025 Maker's Asylum | makersasylum.com
License : MIT

Summary of what this code does:
   Project Title : Ultrasonic Car Animation on OLED Display
   Description   :
      This Arduino program uses an ultrasonic distance sensor
      (HC-SR04 or similar) to measure the distance to an object,
      and then displays a simple car animation on a 128x64 OLED screen
      using the U8glib graphics library.

      The car moves horizontally along a road depending on how close
      or far an object is from the ultrasonic sensor — giving a 
      visual “proximity animation.” A numeric distance value in cm 
      is also displayed at the bottom of the screen.

   Hardware Setup :
      - Ultrasonic Sensor (HC-SR04):
          VCC  → 5V
          GND  → GND
          TRIG → D7  (Arduino digital output)
          ECHO → D6  (Arduino digital input)
      - OLED Display (SH1106 128x64, I2C interface):
          SDA  → A4
          SCL  → A5

   Operation :
      1. Arduino sends a 10 µs trigger pulse via TRIG pin.
      2. Ultrasonic sensor measures echo return time via ECHO pin.
      3. Distance (in cm) is calculated from echo time.
      4. The car graphic moves left/right depending on measured distance.
         - Closer object → car moves right.
         - Farther object → car moves left.
      5. Distance is printed on OLED screen below the animation.

   Libraries Required :
      - U8glib by Oliver Kraus (for SH1106 OLED control)
===============================================================================
*/


#include <U8glib.h>  // Include the U8glib graphics library for SH1106 OLED displays


// --------------------------------------------------------------
// Pin Definitions
// --------------------------------------------------------------
#define TRIG_PIN 7  // Digital pin D7 connected to ultrasonic sensor TRIG pin (output)
#define ECHO_PIN 6  // Digital pin D6 connected to ultrasonic sensor ECHO pin (input)


// --------------------------------------------------------------
// OLED Display Initialization
// --------------------------------------------------------------
U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);
// Create an instance of the SH1106 OLED display object using I2C communication.
// The "_OPT_NONE" option means no special I2C settings are used.


// --------------------------------------------------------------
// SETUP FUNCTION - runs once when Arduino powers on or resets
// --------------------------------------------------------------
void setup() {
  pinMode(TRIG_PIN, OUTPUT);  // Set TRIG pin as OUTPUT (used to send pulse to ultrasonic sensor)
  pinMode(ECHO_PIN, INPUT);   // Set ECHO pin as INPUT (used to receive echo pulse)
  // No Serial communication used since output is on the OLED screen.
}


// --------------------------------------------------------------
// MAIN LOOP - runs repeatedly forever
// --------------------------------------------------------------
void loop() {
  // === Step 1: Trigger the Ultrasonic Pulse ===
  digitalWrite(TRIG_PIN, LOW);          // Ensure TRIG pin is LOW before starting
  delayMicroseconds(2);                 // Short stabilization delay
  digitalWrite(TRIG_PIN, HIGH);         // Send a HIGH signal to start the ultrasonic pulse
  delayMicroseconds(10);                // Keep HIGH for 10 µs per HC-SR04 spec
  digitalWrite(TRIG_PIN, LOW);          // Set LOW again to stop the pulse

  // === Step 2: Measure Echo Duration ===
  long duration = pulseIn(ECHO_PIN, HIGH);
  // pulseIn() waits for the ECHO pin to go HIGH and measures
  // how long it stays HIGH (time in microseconds).
  // The time corresponds to how long the sound wave took to travel to
  // the object and back.

  // === Step 3: Convert Duration to Distance (cm) ===
  int distance = duration * 0.034 / 2;
  // Speed of sound ≈ 0.034 cm/µs.
  // Divide by 2 because the pulse travels to the object and back.

  // === Step 4: Draw the OLED Animation ===
  drawUltra(distance);  // Call function to draw car animation based on measured distance

  delay(100);  // Short delay to stabilize readings and control refresh rate (~10 updates/sec)
}


// --------------------------------------------------------------
// FUNCTION: drawUltra()
// Purpose: Draws the "car on road" animation based on measured distance
// --------------------------------------------------------------
void drawUltra(int distance) {
  int roadY = 50;  // Y-coordinate for road line (horizontal base level for car)
  int carX;        // X-coordinate for car position (changes based on distance)

  // --- Determine car position based on distance ---
  // Only show the car when an object is between 3 cm and 18 cm from the sensor.
  // Closer = car further right; farther = car more to the left.
  if (distance >= 3 && distance <= 18) {
    carX = map(distance, 18, 3, 0, 100);
    // map(value, fromLow, fromHigh, toLow, toHigh)
    // Converts the measured distance (3–18 cm) to a position (0–100 px)
    // So: 18 cm (far) → 0 (leftmost), 3 cm (close) → 100 (rightmost)
  } else {
    carX = -20;  // If object is out of range, move car off-screen (invisible)
  }

  // --- Begin drawing on OLED ---
  u8g.firstPage();   // Start the first page of the display buffer (U8glib requires paged drawing)
  do {
    // Set text font for titles and labels
    u8g.setFont(u8g_font_6x10);

    // Draw title text at top of screen
    u8g.drawStr(30, 10, "Ultra Sensor");

    // --- Draw the road (horizontal line across screen) ---
    u8g.drawLine(0, roadY, 128, roadY);  
    // From (x=0, y=50) to (x=128, y=50), full width of display.

    // --- Draw the car only if within measurable range ---
    if (distance >= 3 && distance <= 18) {
      // === Car Body ===
      u8g.drawBox(carX, roadY - 8, 18, 6);
      // Draws a solid rectangle representing the car body
      // Position: (carX, roadY - 8), width: 18 px, height: 6 px

      // === Car Roof ===
      u8g.drawFrame(carX + 3, roadY - 12, 10, 4);
      // Draws a rectangular frame for the car roof outline

      // === Car Roof Pillars (connect roof and body) ===
      u8g.drawLine(carX + 3, roadY - 8, carX + 3, roadY - 12);   // Left pillar
      u8g.drawLine(carX + 13, roadY - 8, carX + 13, roadY - 12); // Right pillar

      // === Car Wheels ===
      u8g.drawDisc(carX + 4, roadY, 2);   // Left wheel: small filled circle
      u8g.drawDisc(carX + 14, roadY, 2);  // Right wheel: small filled circle
    }

    // --- Display measured distance at bottom of screen ---
    char buf[6];                        // Create small character buffer for distance text
    sprintf(buf, "%d cm", distance);    // Format distance value into string (e.g., "12 cm")
    u8g.drawStr(45, 63, buf);           // Draw distance label near bottom center

  } while (u8g.nextPage());  // Continue until all display pages have been drawn
  // The U8glib display update process repeats for each “page” of memory
  // to ensure full-screen refresh without flicker.
}
