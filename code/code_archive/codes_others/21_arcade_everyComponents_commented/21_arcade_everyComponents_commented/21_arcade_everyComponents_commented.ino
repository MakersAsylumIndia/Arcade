/*
This code is part of the ARCADE project for the "Electronics and Microcontrollers" skill of INNOVATION SCHOOL at MAKERS ASYLUM
Copyright : 2025 Maker's Asylum | makersasylum.com
License : MIT

Summary of what this code does:

Displays a menu on an OLED using U8glib to select hardware components to test.
Uses two buttons for input and a potentiometer for menu navigation.
Includes tests for buttons, buzzer, LDR, potentiometer, ultrasonic sensor, and haptic motor.
Each test mode has a custom OLED animation and an exit condition (pressing both buttons).
Uses PWM control for the haptic motor and analog/digital readings for the rest.
*/

// Include the U8glib library for controlling the OLED display
#include <U8glib.h>

// --- Pin Definitions ---
#define POT_PIN A0        // Potentiometer connected to analog pin A0
#define BUTTON1_PIN 4     // Button 1 input pin
#define BUTTON2_PIN 2     // Button 2 input pin
#define BUZZER_PIN 3      // Buzzer output pin
#define LDR_PIN A1        // Light-dependent resistor (LDR) analog input pin
#define TRIG_PIN 7        // Ultrasonic sensor trigger pin
#define ECHO_PIN 6        // Ultrasonic sensor echo pin
#define MOTOR_PIN 5       // Haptic vibration motor controlled by PWM signal

// --- State Variables ---
bool inHapticTest = false;  // Keeps track if the user is currently in haptic motor test mode
int level = 0;              // Represents motor intensity level (0–4 signal bars)

// Initialize the OLED display using U8glib library
// This particular display is a SH1106 128x64 OLED using I2C interface
U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);

// Menu items displayed on the OLED
const char* menuItems[] = {
  "1. Button", 
  "2. Buzzer", 
  "3. LDR", 
  "4. Pot", 
  "5. Ultra", 
  "6. Haptic"
};
const int totalItems = 6;   // Number of menu items

int selectedItem = 0;       // Keeps track of which menu item is currently selected
bool inMenu = true;         // Flag: true if currently in menu screen
bool inButtonTest = false;  // Individual test mode flags
bool inBuzzerTest = false;
bool inLDRTest = false;
bool inPotTest = false;
bool inUltraTest = false;

// -------------------- SETUP --------------------
void setup() {
  // Configure all the input/output pins
  pinMode(BUTTON1_PIN, INPUT);
  pinMode(BUTTON2_PIN, INPUT);
  pinMode(LDR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // Make sure the buzzer starts OFF
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

// -------------------- MAIN LOOP --------------------
void loop() {

  // --- MENU SCREEN ---
  if (inMenu) {
    // Continuously update menu selection based on potentiometer value
    updateSelection();

    // Start drawing menu on OLED
    u8g.firstPage();
    do {
      drawMenu();   // Display all menu items with selection indicator
    } while (u8g.nextPage());

    // Check if any button was pressed to select a menu option
    if (buttonPressedOnce()) {
      // Determine which test to enter based on current selection
      if (selectedItem == 0) {
        inMenu = false; inButtonTest = true;
      } else if (selectedItem == 1) {
        inMenu = false; inBuzzerTest = true;
      } else if (selectedItem == 2) {
        inMenu = false; inLDRTest = true;
      } else if (selectedItem == 3) {
        inMenu = false; inPotTest = true;
      } else if (selectedItem == 4) {
        inMenu = false; inUltraTest = true;
      } else if (selectedItem == 5) {  // Haptic test option
        inMenu = false; inHapticTest = true;
      }
    }
  }

  // --- BUTTON TEST MODE ---
  else if (inButtonTest) {
    drawButtonTest(); // Draw button visualizer
    // Exit condition: both buttons pressed together
    if (digitalRead(BUTTON1_PIN) == HIGH && digitalRead(BUTTON2_PIN) == HIGH) {
      delay(300);
      // Wait until both are released to prevent accidental triggers
      while (digitalRead(BUTTON1_PIN) == HIGH || digitalRead(BUTTON2_PIN) == HIGH);
      inMenu = true;
      inButtonTest = false;
    }
  }

  // --- BUZZER TEST MODE ---
  else if (inBuzzerTest) {
    drawBuzzerTest(); // Draw buzzer symbol on screen

    // Turn buzzer ON when either button is pressed
    if (digitalRead(BUTTON1_PIN) == HIGH || digitalRead(BUTTON2_PIN) == HIGH) {
      digitalWrite(BUZZER_PIN, HIGH);
    } else {
      digitalWrite(BUZZER_PIN, LOW);
    }

    // Exit if both buttons pressed
    if (digitalRead(BUTTON1_PIN) == HIGH && digitalRead(BUTTON2_PIN) == HIGH) {
      delay(300);
      while (digitalRead(BUTTON1_PIN) == HIGH || digitalRead(BUTTON2_PIN) == HIGH);
      digitalWrite(BUZZER_PIN, LOW); // Ensure buzzer stops
      inMenu = true;
      inBuzzerTest = false;
    }
  }

  // --- LDR TEST MODE ---
  else if (inLDRTest) {
    drawLDRTest(); // Display light sensor visualization
    if (digitalRead(BUTTON1_PIN) == HIGH && digitalRead(BUTTON2_PIN) == HIGH) {
      delay(300);
      while (digitalRead(BUTTON1_PIN) == HIGH || digitalRead(BUTTON2_PIN) == HIGH);
      inMenu = true;
      inLDRTest = false;
    }
  }

  // --- POTENTIOMETER TEST MODE ---
  else if (inPotTest) {
    drawPotTest(); // Show spiral animation responding to potentiometer
    if (digitalRead(BUTTON1_PIN) == HIGH && digitalRead(BUTTON2_PIN) == HIGH) {
      delay(300);
      while (digitalRead(BUTTON1_PIN) == HIGH || digitalRead(BUTTON2_PIN) == HIGH);
      inMenu = true;
      inPotTest = false;
    }
  }

  // --- ULTRASONIC TEST MODE ---
  else if (inUltraTest) {
    drawUltraTest(); // Show distance with moving car animation
    if (digitalRead(BUTTON1_PIN) == HIGH && digitalRead(BUTTON2_PIN) == HIGH) {
      delay(300);
      while (digitalRead(BUTTON1_PIN) == HIGH || digitalRead(BUTTON2_PIN) == HIGH);
      inMenu = true;
      inUltraTest = false;
    }
  }

  // --- HAPTIC MOTOR TEST MODE ---
  else if (inHapticTest) {
    drawHapticTest(); // Display signal bar visualization for motor strength

    // Increase intensity with button 2
    if (digitalRead(BUTTON2_PIN) == HIGH) {
      level++;
      if (level > 4) level = 4;  // Cap at max level
    }

    // Decrease intensity with button 1
    if (digitalRead(BUTTON1_PIN) == HIGH) {
      level--;
      if (level < 0) level = 0;  // Limit to zero
    }

    // Map level to PWM values for smooth control
    int pwmValue = 0;
    switch (level) {
      case 0: pwmValue = 0; break;
      case 1: pwmValue = 64; break;
      case 2: pwmValue = 128; break;
      case 3: pwmValue = 192; break;
      case 4: pwmValue = 255; break;
    }
    analogWrite(MOTOR_PIN, pwmValue); // Apply PWM signal to motor

    // Exit if both buttons are pressed simultaneously
    if (digitalRead(BUTTON1_PIN) == HIGH && digitalRead(BUTTON2_PIN) == HIGH) {
      delay(300);
      while (digitalRead(BUTTON1_PIN) == HIGH || digitalRead(BUTTON2_PIN) == HIGH);
      analogWrite(MOTOR_PIN, 0); // Turn off motor
      inMenu = true;
      inHapticTest = false;
    }
  }

  // Small delay to reduce flickering and debounce minor input noise
  delay(100);
}

// -------------------- FUNCTION DEFINITIONS --------------------

// Reads potentiometer value and maps it to one of the menu items
void updateSelection() {
  int potValue = 1023 - analogRead(POT_PIN); // Reverse direction of knob movement
  int bandSize = 1024 / totalItems;          // Divide 1024 range into equal sections
  selectedItem = potValue / bandSize;        // Determine which menu item is active
  if (selectedItem >= totalItems) selectedItem = totalItems - 1;
}

// Draws the main menu with selection arrow
void drawMenu() {
  u8g.setFont(u8g_font_6x10);
  u8g.drawStr(0, 8, "Select Component:");
  for (int i = 0; i < totalItems; i++) {
    if (i == selectedItem) {
      u8g.drawStr(0, 18 + i * 9, ">");     // Draw arrow for current selection
    }
    u8g.drawStr(10, 18 + i * 9, menuItems[i]);
  }
}

// Detects a single press (edge detection)
bool buttonPressedOnce() {
  static bool lastState = LOW;
  bool current = (digitalRead(BUTTON1_PIN) == HIGH || digitalRead(BUTTON2_PIN) == HIGH);

  if (current && !lastState) {   // Button transitioned from LOW to HIGH
    lastState = true;
    delay(20);                   // Small debounce delay
    return true;
  } else if (!current) {
    lastState = false;
  }
  return false;
}

// Button test: visualize buttons as circles that grow when pressed
void drawButtonTest() {
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x10);
    u8g.drawStr(20, 10, "Press any Button");

    int x1 = 40, y1 = 40;   // Position for Button 1
    int x2 = 90, y2 = 40;   // Position for Button 2

    // Button 1 visual indicator
    if (digitalRead(BUTTON1_PIN) == HIGH)
      u8g.drawDisc(x1, y1, 12);  // Larger circle when pressed
    else
      u8g.drawDisc(x1, y1, 5);   // Small circle when not pressed

    // Button 2 visual indicator
    if (digitalRead(BUTTON2_PIN) == HIGH)
      u8g.drawDisc(x2, y2, 12);
    else
      u8g.drawDisc(x2, y2, 5);

  } while (u8g.nextPage());
}

// Buzzer test: draw buzzer symbol and animate when ON
void drawBuzzerTest() {
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x10);
    u8g.drawStr(15, 10, "Press any Button");

    u8g.drawTriangle(30, 30, 30, 50, 45, 40);  // Speaker shape

    // When button is pressed, show sound waves
    if (digitalRead(BUTTON1_PIN) == HIGH || digitalRead(BUTTON2_PIN) == HIGH) {
      digitalWrite(BUZZER_PIN, HIGH);
      u8g.drawCircle(50, 40, 5);
      u8g.drawCircle(55, 40, 8);
      u8g.drawCircle(60, 40, 11);
    } else {
      digitalWrite(BUZZER_PIN, LOW);
      // Draw “X” over speaker when OFF
      u8g.drawLine(48, 30, 60, 50);
      u8g.drawLine(48, 50, 60, 30);
    }

  } while (u8g.nextPage());
}

// LDR test: draws a light bulb glowing proportionally to detected light
void drawLDRTest() {
  int ldrValue = analogRead(LDR_PIN);           // Read light intensity
  int glowLevel = map(ldrValue, 0, 1023, 0, 10); // Map to number of glow lines

  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x10);
    u8g.drawStr(35, 10, "LDR Sensor");

    int cx = 64, cy = 50;
    u8g.drawCircle(cx, cy, 8);             // Bulb outline
    u8g.drawLine(cx, cy + 8, cx, cy + 14); // Bulb base line
    u8g.drawLine(cx - 5, cy + 14, cx + 5, cy + 14); // Base bottom

    // Draw glow lines increasing with light intensity
    for (int i = 0; i < glowLevel; i++) {
      int y = cy - 5 - i * 2;
      int w = 4 + i * 2;
      u8g.drawLine(cx - w, y, cx + w, y);
    }

  } while (u8g.nextPage());
}

// Potentiometer test: draws a spiral with a ball moving along it
void drawPotTest() {
  int potValue = analogRead(POT_PIN);
  float t = map(potValue, 0, 1023, 0, 180);  // Ball position on spiral path

  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x10);
    u8g.drawStr(35, 10, "Pot Spiral");

    int cx = 64, cy = 35;      // Spiral center point
    float angleStep = 0.08;    // Controls spiral smoothness
    float a = 1.8;             // Controls how tightly spiral grows

    // Draw mirrored spiral pattern
    for (float theta = 0; theta < angleStep * 180; theta += angleStep) {
      float r = a * theta;
      int x = cx - r * cos(theta);
      int y = cy + r * sin(theta);
      u8g.drawPixel(x, y);
    }

    // Draw moving ball showing potentiometer position
    float ballTheta = angleStep * t;
    float rBall = a * ballTheta;
    int ballX = cx - rBall * cos(ballTheta);
    int ballY = cy + rBall * sin(ballTheta);
    u8g.drawCircle(ballX, ballY, 2);

  } while (u8g.nextPage());
}

// Ultrasonic test: visual car moves based on distance
void drawUltraTest() {
  // Trigger ultrasonic sensor pulse
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Measure pulse duration and convert to distance (cm)
  long duration = pulseIn(ECHO_PIN, HIGH);
  int distance = duration / 58;

  int roadY = 50;
  int carX = -20;  // Default off-screen

  // Map measured distance to car position
  if (distance >= 3 && distance <= 18) {
    carX = map(distance, 18, 3, 0, 100); // Near = right side, far = left
  }

  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x10);
    u8g.drawStr(30, 10, "Ultra Sensor");

    // Draw ground line
    u8g.drawLine(0, roadY, 128, roadY);

    // Draw car only when within valid detection range
    if (distance >= 3 && distance <= 18) {
      u8g.drawBox(carX, roadY - 8, 18, 6);            // Car body
      u8g.drawFrame(carX + 3, roadY - 12, 10, 4);     // Roof
      u8g.drawLine(carX + 3, roadY - 8, carX + 3, roadY - 12);
      u8g.drawLine(carX + 13, roadY - 8, carX + 13, roadY - 12);
      u8g.drawDisc(carX + 4, roadY, 2);               // Wheels
      u8g.drawDisc(carX + 14, roadY, 2);
    }

    // Display numeric distance at bottom
    char buf[10];
    sprintf(buf, "%d cm", distance);
    u8g.drawStr(45, 63, buf);

  } while (u8g.nextPage());
}

// Haptic test: displays bars representing motor strength
void drawHapticTest() {
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x10);
    u8g.drawStr(20, 10, "Haptic Strength");

    // Draw 4 vertical signal bars
    for (int i = 0; i < 4; i++) {
      int barHeight = (i + 1) * 8;
      int x = 40 + i * 12;
      int y = 60 - barHeight;
      if (i < level) {
        u8g.drawBox(x, y, 8, barHeight);   // Filled for active strength
      } else {
        u8g.drawFrame(x, y, 8, barHeight); // Hollow for inactive
      }
    }
  } while (u8g.nextPage());
}
