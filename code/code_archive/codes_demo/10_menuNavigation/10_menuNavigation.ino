/* 
===============================================================================
This code is part of the ARCADE project for the "Electronics and 
Microcontrollers" skill of INNOVATION SCHOOL at MAKERS ASYLUM

Copyright : 2025 Maker's Asylum | makersasylum.com
License : MIT

Summary of what this code does:
   Project: Potentiometer-Driven OLED Menu (6-Item Version, Optimized Layout)
   Display: SH1106 128x64 OLED (I2C)
   Library: U8glib

   ---------------------------------------------------------------------------
   PURPOSE:
   Displays a scrollable 6-item menu on an SH1106 OLED display.
   A potentiometer connected to A0 allows the user to move a “>” cursor
   to highlight one of the menu options.

   ---------------------------------------------------------------------------
   HARDWARE SETUP:
     • SH1106 OLED (I2C)
         VCC → 5V
         GND → GND
         SDA → A4  (Arduino I2C SDA)
         SCL → A5  (Arduino I2C SCL)

     • Potentiometer:
         Left pin  → 5V
         Right pin → GND
         Middle pin → A0 (wiper output to Arduino analog input)

   ---------------------------------------------------------------------------
   OPERATION:
     1. The potentiometer’s position (0–1023) is read on A0.
     2. This analog value is divided into six equal “bands,”
        one for each menu option.
     3. The current band determines which menu item is highlighted.
     4. The menu automatically updates in real time.

   NOTES:
     - Uses U8glib’s “paged drawing” method to refresh displays efficiently.
     - Spacing between menu lines has been increased to 8 pixels
       for optimal readability on 128×64 OLED screens.
===============================================================================
*/

#include <U8glib.h>  // Include U8glib library for OLED display control

// ---------------------------
// Pin Definitions
// ---------------------------
#define POT_PIN A0  // Potentiometer connected to analog input pin A0

// ---------------------------
// OLED Initialization
// ---------------------------
// Create a U8glib object for a 128x64 SH1106 OLED using I2C communication
U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);

// ---------------------------
// Menu Configuration
// ---------------------------
// Define an array of six menu items
const char* menuItems[] = {
  "1. Button",
  "2. Buzzer",
  "3. LDR",
  "4. Potentiometer",
  "5. Ultrasonic",
  "6. Haptic"
};

// Calculate total number of items automatically
const int totalItems = sizeof(menuItems) / sizeof(menuItems[0]);

// Variable to store the index (0–5) of the currently selected item
int selectedItem = 0;

// ---------------------------
// drawMenu()
// Renders the menu on the OLED screen.
// ---------------------------
void drawMenu() {
  u8g.setFont(u8g_font_6x10);               // Select readable 6×10 font
  u8g.drawStr(0, 10, "Select Component:");  // Display menu title at top

  // Loop through all menu items and draw them line by line
  for (int i = 0; i < totalItems; i++) {
    int y = 22 + (i * 8);  // Increased line spacing from 7→8 pixels for clarity

    // If this item is selected, draw a “>” cursor before it
    if (i == selectedItem) {
      u8g.drawStr(0, y, ">");
    }

    // Draw the label for the menu item slightly to the right
    u8g.drawStr(10, y, menuItems[i]);
  }
}

// ---------------------------
// updateSelection()
// Reads the potentiometer and maps its value to one of the menu indices.
// ---------------------------
void updateSelection() {
  // Read the potentiometer’s analog value (range: 0–1023)
  int potValue = 1023 - analogRead(POT_PIN);  // Invert direction for natural feel

  // Divide the range into equal “bands” per menu item
  int bandSize = 1024 / totalItems;

  // Convert potentiometer position into a menu index (0–5)
  selectedItem = potValue / bandSize;

  // Ensure the index never exceeds the number of menu items
  if (selectedItem >= totalItems) {
    selectedItem = totalItems - 1;
  }
}

// ---------------------------
// setup()
// Runs once at startup.
// ---------------------------
void setup() {
  // No additional initialization needed for U8glib or OLED.
  // I2C communication begins automatically with U8glib’s constructor.
}

// ---------------------------
// loop()
// Continuously updates the displayed menu based on potentiometer input.
// ---------------------------
void loop() {
  updateSelection();  // Update which menu item is currently selected

  // U8glib uses paged drawing — required to update the entire OLED screen
  u8g.firstPage();
  do {
    drawMenu();  // Render the current menu frame
  } while (u8g.nextPage());

  // Add a short delay for smooth menu scrolling (~10 frames/sec)
  delay(100);
}
