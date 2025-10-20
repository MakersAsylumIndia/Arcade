/*
This code is part of the ARCADE project for the "Electronics and Microcontrollers" skill of INNOVATION SCHOOL at MAKERS ASYLUM
Copyright : 2025 Maker's Asylum | makersasylum.com
License : MIT

Summary of what this code does:
---------------------------------------------------------------
  Project: Two-Component Test Menu (Button & Buzzer)
  Description:
    This Arduino program provides a simple on-screen menu system 
    using a 128x64 SH1106 OLED display to test two components:
    - Buttons
    - Buzzer

    The user navigates through the menu using a potentiometer and 
    selects an option using either of the two buttons. Each test 
    mode displays interactive graphics on the OLED, reflecting 
    the current input or output state.

  Hardware Setup:
    - OLED Display: SH1106 128x64 (I2C connection)
    - Potentiometer: Connected to analog pin A0 for menu selection
    - Button 1: Digital pin D4 (active HIGH on press)
    - Button 2: Digital pin D2 (active HIGH on press)
    - Buzzer: Digital pin D3 (active HIGH to sound)
    - External resistors used for stable button input levels

  Operation:
    1. The main menu allows selecting between:
       • Button Test – displays visual feedback for button presses.
       • Buzzer Test – activates buzzer and draws sound waves when pressed.
    2. The potentiometer scrolls through menu options.
    3. Pressing either button enters the selected test.
    4. Pressing both buttons together exits a test and returns to the menu.
    5. The OLED updates in real-time to show sensor/output feedback.

  Libraries Required:
    - U8glib (for SH1106 OLED graphics rendering)
   --------------------------------------------------------------- */

#include <U8glib.h>  // Include the U8glib graphics library for SH1106 OLED display support

// --- Pin Definitions ---
#define POT_PIN A0         // Analog pin connected to the potentiometer
#define BUTTON1_PIN 4      // Digital pin connected to Button 1 (active HIGH)
#define BUTTON2_PIN 2      // Digital pin connected to Button 2 (active HIGH)
#define BUZZER_PIN 3       // Digital pin connected to the buzzer (active HIGH)

// --- OLED Display Initialization ---
// Create a display object for SH1106 OLED using I2C communication
U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);

// --- Menu Configuration ---
const char* menuItems[] = {"1. Button", "2. Buzzer"};  // Menu item labels
const int totalItems = 2;                             // Number of menu items

// --- Menu State Variables ---
int selectedItem = 0;      // Tracks the current menu selection
bool inMenu = true;        // True when in main menu mode
bool inButtonTest = false; // True when running Button Test
bool inBuzzerTest = false; // True when running Buzzer Test

// --- Setup Function ---
void setup() {
  // Configure button pins as inputs (using external resistors)
  pinMode(BUTTON1_PIN, INPUT);
  pinMode(BUTTON2_PIN, INPUT);

  // Configure buzzer as output and ensure it's initially OFF
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
}

// --- Main Loop ---
void loop() {
  // --- MENU MODE ---
  if (inMenu) {
    updateSelection();  // Read potentiometer and update menu selection

    // Render the OLED menu page by page
    u8g.firstPage();
    do {
      drawMenu();  // Draw menu items and highlight selection
    } while (u8g.nextPage());

    // Check if a button was pressed once to select a menu item
    if (buttonPressedOnce()) {
      if (selectedItem == 0) {
        // Enter Button Test
        inMenu = false;
        inButtonTest = true;
      } else if (selectedItem == 1) {
        // Enter Buzzer Test
        inMenu = false;
        inBuzzerTest = true;
      }
    }
  }

  // --- BUTTON TEST MODE ---
  else if (inButtonTest) {
    drawButtonTest();  // Continuously draw button feedback graphics

    // If both buttons are pressed together → return to main menu
    if (digitalRead(BUTTON1_PIN) == HIGH && digitalRead(BUTTON2_PIN) == HIGH) {
      delay(300);  // Small delay to prevent accidental triggers
      // Wait until both buttons are released
      while (digitalRead(BUTTON1_PIN) == HIGH || digitalRead(BUTTON2_PIN) == HIGH);
      // Switch states
      inMenu = true;
      inButtonTest = false;
    }
  }

  // --- BUZZER TEST MODE ---
  else if (inBuzzerTest) {
    drawBuzzerTest();  // Display buzzer graphics (speaker + sound waves)

    // Activate buzzer when any button is pressed
    if (digitalRead(BUTTON1_PIN) == HIGH || digitalRead(BUTTON2_PIN) == HIGH) {
      digitalWrite(BUZZER_PIN, HIGH);
    } else {
      digitalWrite(BUZZER_PIN, LOW);
    }

    // If both buttons are pressed together → stop buzzer and return to menu
    if (digitalRead(BUTTON1_PIN) == HIGH && digitalRead(BUTTON2_PIN) == HIGH) {
      delay(300);  // Debounce delay
      // Wait for button release
      while (digitalRead(BUTTON1_PIN) == HIGH || digitalRead(BUTTON2_PIN) == HIGH);
      digitalWrite(BUZZER_PIN, LOW);  // Ensure buzzer is turned off
      inMenu = true;
      inBuzzerTest = false;
    }
  }

  delay(100);  // Small delay to stabilize screen refresh and input reading
}

// --- Update Menu Selection Function ---
// Reads the potentiometer value and maps it to the menu item index
void updateSelection() {
  int potValue = 1023 - analogRead(POT_PIN);  // Read and invert potentiometer value (0–1023)
  int bandSize = 1024 / totalItems;           // Divide range into equal sections per menu item
  selectedItem = potValue / bandSize;         // Determine which section the knob is in
  if (selectedItem >= totalItems) selectedItem = totalItems - 1;  // Prevent overflow
}

// --- Draw Menu on OLED ---
void drawMenu() {
  u8g.setFont(u8g_font_6x10);                    // Set small readable font
  u8g.drawStr(0, 10, "Select Component:");       // Draw menu title

  // Loop through each menu item and draw it
  for (int i = 0; i < totalItems; i++) {
    if (i == selectedItem) {
      u8g.drawStr(0, 20 + i * 10, ">");          // Draw selection indicator ">"
    }
    u8g.drawStr(10, 20 + i * 10, menuItems[i]);  // Draw menu item text
  }
}

// --- Detect Single Button Press ---
// Returns true when a button is pressed once (with basic debounce)
bool buttonPressedOnce() {
  static bool lastState = LOW;  // Stores the last known button state
  bool current = (digitalRead(BUTTON1_PIN) == HIGH || digitalRead(BUTTON2_PIN) == HIGH);

  if (current && !lastState) {  // Detect rising edge (button just pressed)
    lastState = true;
    delay(20);                  // Debounce delay
    return true;
  } else if (!current) {        // Reset state when no buttons are pressed
    lastState = false;
  }
  return false;
}

// --- Button Test Display Function ---
// Shows visual feedback of each button press using on-screen circles
void drawButtonTest() {
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x10);
    u8g.drawStr(20, 10, "Press any Button");

    // Button positions on screen
    int x1 = 40, y1 = 40;  // Left button circle position
    int x2 = 90, y2 = 40;  // Right button circle position

    // Draw Button 1 circle
    if (digitalRead(BUTTON1_PIN) == HIGH)
      u8g.drawDisc(x1, y1, 12);  // Larger filled circle when pressed
    else
      u8g.drawDisc(x1, y1, 5);   // Small circle when not pressed

    // Draw Button 2 circle
    if (digitalRead(BUTTON2_PIN) == HIGH)
      u8g.drawDisc(x2, y2, 12);
    else
      u8g.drawDisc(x2, y2, 5);

  } while (u8g.nextPage());  // Finish drawing page
}

// --- Buzzer Test Display Function ---
// Visualizes buzzer activity with speaker shape and sound wave animation
void drawBuzzerTest() {
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x10);
    u8g.drawStr(15, 10, "Press any Button");

    // Draw the speaker base shape
    u8g.drawTriangle(30, 30, 30, 50, 45, 40);  // Speaker cone

    // When any button is pressed, turn on buzzer and draw sound waves
    if (digitalRead(BUTTON1_PIN) == HIGH || digitalRead(BUTTON2_PIN) == HIGH) {
      digitalWrite(BUZZER_PIN, HIGH);   // Activate buzzer
      // Draw concentric circles to represent sound waves
      u8g.drawCircle(50, 40, 5);
      u8g.drawCircle(55, 40, 8);
      u8g.drawCircle(60, 40, 11);
    } 
    else {
      digitalWrite(BUZZER_PIN, LOW);    // Turn off buzzer
      // Draw an "X" over the speaker to indicate mute state
      u8g.drawLine(48, 30, 60, 50);
      u8g.drawLine(48, 50, 60, 30);
    }

  } while (u8g.nextPage());  // Complete screen refresh cycle
}
