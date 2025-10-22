/* =============================================================================
   MINI ARCADE - FULL HARDWARE TEST MENU (SCROLLING VERSION)
   ---------------------------------------------------------------------------
   Hardware: Arduino Nano
   Purpose : Allows students to test each circuit component individually
             using a simple OLED menu, with navigation via potentiometer
             and control via LEFT (start) and RIGHT (stop/back) buttons.

   COMPONENTS & CONNECTIONS:
   * OLED (SH1106 128x64)  → I2C SDA/SCL pins (A4/A5)
   * Potentiometer (A0)    → used to scroll menu
   * LEFT BTN (D4)         → Start selected test
   * RIGHT BTN (D2)        → Stop test / Return to menu
   * Buzzer (via NPN)      → D3
   * Haptic Motor          → D5
   * Ultrasonic Sensor     → Trigger D7, Echo D6
   * LDR (with pulldown)   → A1
   ---------------------------------------------------------------------------
   Notes:
     - Buttons use external pull-down resistors so pressed = HIGH.
     - Buzzer negative is connected to NPN transistor collector; base driven from D3.
   =============================================================================
*/

#include <U8glib.h>

#define POT_PIN A0
#define LEFT_BTN 4
#define RIGHT_BTN 2
#define BUZZER_PIN 3
#define HAPTIC_PIN 5
#define ECHO_PIN 6
#define TRIG_PIN 7
#define LDR_PIN A1

U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);

// --- Menu items ---
const char* menuItems[] = {
  "1. OLED Test",
  "2. Buttons Test",
  "3. Buzzer Test",
  "4. LDR Test",
  "5. Potentiometer",
  "6. Ultrasonic",
  "7. Haptic Motor"
};
const int totalItems = sizeof(menuItems) / sizeof(menuItems[0]);

int selectedItem = 0;   // currently highlighted menu index
int scrollOffset = 0;   // top-most visible item index for scrolling window

bool testRunning = false; // true while an individual test is active

// ------------------------- setup()
void setup() {
  pinMode(LEFT_BTN, INPUT);
  pinMode(RIGHT_BTN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(HAPTIC_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  Serial.begin(9600);
}

// ------------------------- main loop
void loop() {
  if (testRunning) return;

  updateSelection();  // read pot and compute selectedItem
  showMenu();         // draw menu

  // Start the selected test when LEFT button is pressed
  if (digitalRead(LEFT_BTN) == HIGH) {
    delay(200); // debounce
    runTest(selectedItem);
  }
}

// ------------------------- updateSelection()
// Reversed scroll direction: turning potentiometer clockwise moves UP (lower index).
void updateSelection() {
  int potValue = analogRead(POT_PIN);

  // Reverse the direction by inverting the value
  potValue = 1023 - potValue;

  selectedItem = map(potValue, 0, 1023, 0, totalItems - 1);

  // Keep selected item within the 5-line window
  if (selectedItem < scrollOffset) scrollOffset = selectedItem;
  if (selectedItem > scrollOffset + 4) scrollOffset = selectedItem - 4;
}

// ------------------------- showMenu()
// Draws up to 5 visible menu items with scrolling
void showMenu() {
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x10);
    u8g.drawStr(0, 10, "Select Test Item:");

    // Display 5 items max (scrollable window)
    for (int i = 0; i < 5; i++) {
      int itemIndex = scrollOffset + i;
      if (itemIndex >= totalItems) break;
      int y = 22 + i * 10;
      if (itemIndex == selectedItem) {
        u8g.drawStr(0, y, ">");
      }
      u8g.drawStr(10, y, menuItems[itemIndex]);
    }

    // Scroll indicators
    if (scrollOffset > 0) u8g.drawStr(116, 8, "^");
    if (scrollOffset + 5 < totalItems) u8g.drawStr(116, 60, "v");

  } while (u8g.nextPage());
}

// ------------------------- runTest()
void runTest(int item) {
  testRunning = true;
  switch (item) {
    case 0: oledTest(); break;
    case 1: buttonsTest(); break;
    case 2: buzzerTest(); break;
    case 3: ldrTest(); break;
    case 4: potTest(); break;
    case 5: ultrasonicTest(); break;
    case 6: hapticTest(); break;
  }
  testRunning = false;
}

// ------------------------- TESTS -------------------------

// --- OLED TEST ---
void oledTest() {
  while (digitalRead(RIGHT_BTN) == LOW) {
    u8g.firstPage();
    do {
      u8g.setFont(u8g_font_6x13B);
      u8g.drawStr(8, 20, "OLED DISPLAY TEST");
      u8g.setFont(u8g_font_6x10);
      u8g.drawStr(12, 38, "Graphics & Text OK");
      u8g.drawStr(8, 58, "Press RIGHT to exit");
    } while (u8g.nextPage());
    delay(200);
  }
}

// --- BUTTONS TEST ---
void buttonsTest() {
  while (digitalRead(RIGHT_BTN) == LOW) {
    bool leftState = digitalRead(LEFT_BTN);
    bool rightState = digitalRead(RIGHT_BTN);

    u8g.firstPage();
    do {
      u8g.setFont(u8g_font_6x10);
      u8g.drawStr(8, 12, "Buttons Test");
      u8g.drawStr(8, 26, "Press LEFT/RIGHT to test");

      // Left Button status
      u8g.drawStr(8, 46, "LEFT:");
      u8g.drawStr(48, 46, leftState ? "PRESSED" : "released");

      // Right Button status
      u8g.drawStr(8, 58, "RIGHT:");
      u8g.drawStr(48, 58, rightState ? "PRESSED" : "released");
    } while (u8g.nextPage());

    delay(100);
  }
}

// --- BUZZER TEST ---
void buzzerTest() {
  while (digitalRead(RIGHT_BTN) == LOW) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  }
}

// --- LDR TEST ---
void ldrTest() {
  while (digitalRead(RIGHT_BTN) == LOW) {
    int value = analogRead(LDR_PIN);
    int barLength = map(value, 0, 1023, 0, 120);

    u8g.firstPage();
    do {
      u8g.setFont(u8g_font_6x10);
      u8g.drawStr(12, 18, "LDR Test");
      u8g.drawFrame(4, 30, 120, 10);
      u8g.drawBox(4, 30, barLength, 10);
      u8g.drawStr(8, 58, "Press RIGHT to exit");
    } while (u8g.nextPage());

    Serial.print("LDR: ");
    Serial.println(value);
    delay(200);
  }
}

// --- POTENTIOMETER TEST ---
void potTest() {
  while (digitalRead(RIGHT_BTN) == LOW) {
    int value = analogRead(POT_PIN);
    int barLength = map(value, 0, 1023, 0, 120);

    u8g.firstPage();
    do {
      u8g.setFont(u8g_font_6x10);
      u8g.drawStr(6, 18, "Potentiometer");
      u8g.drawFrame(4, 30, 120, 10);
      u8g.drawBox(4, 30, barLength, 10);
      u8g.drawStr(8, 58, "Turn pot; RIGHT to exit");
    } while (u8g.nextPage());

    Serial.print("Pot: ");
    Serial.println(value);
    delay(100);
  }
}

// --- ULTRASONIC TEST ---
void ultrasonicTest() {
  while (digitalRead(RIGHT_BTN) == LOW) {
    // Trigger ultrasonic pulse
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    // Measure echo duration
    long duration = pulseIn(ECHO_PIN, HIGH);
    int distance = duration * 0.034 / 2; // Convert to cm

    u8g.firstPage();
    do {
      u8g.setFont(u8g_font_6x13);
      char buf[20];
      sprintf(buf, "Dist: %d cm", distance);
      u8g.drawStr(10, 20, "Ultrasonic Test");
      u8g.drawStr(10, 40, buf);
      u8g.setFont(u8g_font_6x10);
      u8g.drawStr(8, 58, "Press RIGHT to exit");
    } while (u8g.nextPage());

    Serial.print("Distance: ");
    Serial.println(distance);
    delay(250);
  }
}

// --- HAPTIC MOTOR TEST ---
void hapticTest() {
  while (digitalRead(RIGHT_BTN) == LOW) {
    digitalWrite(HAPTIC_PIN, HIGH);
    delay(200);
    digitalWrite(HAPTIC_PIN, LOW);
    delay(800);
  }
}
