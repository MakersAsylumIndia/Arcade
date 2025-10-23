/*
===============================================================================
This code is part of the ARCADE project for the "Electronics and 
Microcontrollers" skill of INNOVATION SCHOOL at MAKERS ASYLUM
Copyright : 2025 Maker's Asylum | makersasylum.com
License : MIT

Summary of what this code does:
  Project: Ultrasonic Sensor Visual Distance Display
  Description:
    This program uses an ultrasonic distance sensor to measure 
    the distance to an object and visualizes the result on a 
    128x64 SH1106 OLED display. The closer the object is, the 
    brighter (more filled) the OLED display appears.

  Hardware Setup:
    - Ultrasonic Sensor:
       Trig Pin → D7  (Output from Arduino to send pulse)
       Echo Pin → D6  (Input to receive reflected pulse)
    - OLED Display:
       128x64 SH1106 module connected via I2C
    - Power:
        Both modules powered by Arduino 5V and GND

   Operation:
    - The Arduino sends a 10 µs trigger pulse to the ultrasonic sensor.
    - The sensor returns a pulse whose duration corresponds to distance.
    - The distance is calculated and mapped to a visual brightness level.
    - The OLED screen displays horizontal fill lines proportional to brightness
      (closer object = denser pattern).
    - The process repeats continuously, updating roughly every 100 ms.

===============================================================================
*/


#include <U8glib.h>  // Include the U8glib library for controlling SH1106 OLED displays

// --- OLED Display Initialization ---
U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);  
// Create an instance of the OLED display object using I2C interface (no special options).

// --- Ultrasonic Sensor Pin Definitions ---
const int trigPin = 7;  // Trigger pin sends ultrasonic pulses
const int echoPin = 6;  // Echo pin receives reflected signal

// --- Variables for distance measurement ---
long duration;   // Variable to store the time (in microseconds) that the echo pin stays HIGH
int distance;    // Calculated distance in centimeters
int contrast = 0;  // Used to determine brightness level for the OLED pattern


// ----------------------
// Setup Function
// ----------------------
void setup() {
  pinMode(trigPin, OUTPUT);  // Set trigger pin as an OUTPUT (we send a pulse)
  pinMode(echoPin, INPUT);   // Set echo pin as an INPUT (we read the reflection)
}


// ----------------------
// Main Loop
// ----------------------
void loop() {
  // --- Step 1: Send a 10µs Ultrasonic Pulse ---
  digitalWrite(trigPin, LOW);          // Ensure trigger pin starts LOW
  delayMicroseconds(2);                // Short delay to stabilize signal
  digitalWrite(trigPin, HIGH);         // Send HIGH pulse to start measurement
  delayMicroseconds(10);               // Keep HIGH for 10 microseconds (standard sensor requirement)
  digitalWrite(trigPin, LOW);          // Bring trigger LOW again to end the pulse

  // --- Step 2: Measure Echo Duration ---
  duration = pulseIn(echoPin, HIGH);   // Measure how long the echo pin stays HIGH (time for sound to return)

  // --- Step 3: Convert Duration to Distance (in cm) ---
  // Speed of sound = 0.034 cm/µs; divide by 2 because sound travels to the object and back
  distance = duration * 0.034 / 2;

  // --- Step 4: Map Distance to Display Brightness ---
  int minDist = 3;    // Minimum measurable distance (cm) - closer than this is ignored
  int maxDist = 10;   // Maximum distance for brightness mapping (cm)
  
  // Constrain measured distance within range to prevent display flicker or out-of-range values
  distance = constrain(distance, minDist, maxDist);

  // Map distance to contrast value:
  //   closer = higher brightness (255), farther = dimmer (0)
  contrast = map(distance, minDist, maxDist, 255, 0);

  // --- Step 5: Draw Visualization on OLED ---
  // The OLED display is updated in "pages" (required by U8glib)
  u8g.firstPage();  
  do {
    // Convert contrast value (0–255) to a number of horizontal lines (0–64)
    // This gives a visual sense of brightness: more lines → brighter area
    int fillLines = map(contrast, 0, 255, 0, 64);

    // Draw horizontal "brightness" lines across the screen
    // We step by 2 pixels vertically to create evenly spaced stripes
    for (int i = 0; i < fillLines; i += 2) {
      u8g.drawBox(0, i, 128, 1);  // Draw a thin horizontal line (1 pixel thick) across the full width
    }
  } while (u8g.nextPage());  // Repeat for all display memory pages until the full frame is rendered

  // --- Step 6: Small Delay for Stability ---
  delay(100);  // Wait 100 ms before repeating (about 10 updates per second)
}
