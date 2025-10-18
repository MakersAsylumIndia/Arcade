#include <U8glib.h>

#define POT_PIN A0
#define BUTTON1_PIN 4
#define BUTTON2_PIN 2

U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);  // Adjust if needed

const char* menuItems[] = {"1. Button", "2. Buzzer", "3. LDR", "4. Pot", "5. Ultra"};
const int totalItems = 5;
int selectedItem = 0;
bool inMenu = true;

void setup() {
  pinMode(BUTTON1_PIN, INPUT);  // No pullup, since you're using external resistors
  pinMode(BUTTON2_PIN, INPUT);
}

void loop() {
  if (inMenu) {
    updateSelection();
    u8g.firstPage();
    do {
      drawMenu();
    } while (u8g.nextPage());

    if (buttonPressedOnce()) {
      if (selectedItem == 0) {
        inMenu = false;
      }
    }
  } else {
    drawButtonTest();
    // Go back to menu only when both buttons are pressed together
    if (digitalRead(BUTTON1_PIN) == HIGH && digitalRead(BUTTON2_PIN) == HIGH) {
      delay(300);
      while (digitalRead(BUTTON1_PIN) == HIGH || digitalRead(BUTTON2_PIN) == HIGH); // wait for release
      inMenu = true;
    }

  }

  delay(100);
}

void updateSelection() {
  int potValue = 1023 - analogRead(POT_PIN);  // Reverse for correct scroll
  int bandSize = 1024 / totalItems;
  selectedItem = potValue / bandSize;
  if (selectedItem >= totalItems) selectedItem = totalItems - 1;
}

void drawMenu() {
  u8g.setFont(u8g_font_6x10);
  u8g.drawStr(0, 10, "Select Component:");
  for (int i = 0; i < totalItems; i++) {
    if (i == selectedItem) {
      u8g.drawStr(0, 20 + i * 10, ">");
    }
    u8g.drawStr(10, 20 + i * 10, menuItems[i]);
  }
}

bool buttonPressedOnce() {
  static bool lastState = LOW;
  bool current = (digitalRead(BUTTON1_PIN) == HIGH || digitalRead(BUTTON2_PIN) == HIGH);

  if (current && !lastState) {
    lastState = true;
    delay(20); // debounce
    return true;
  } else if (!current) {
    lastState = false;
  }
  return false;
}


void drawButtonTest() {
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x10);
    u8g.drawStr(20, 10, "Press any Button");

    int x1 = 40, y1 = 40;
    int x2 = 90, y2 = 40;

    // If button is pressed, draw bigger circle
    if (digitalRead(BUTTON1_PIN) == HIGH)
      u8g.drawDisc(x1, y1, 10);  // Large left circle
    else
      u8g.drawDisc(x1, y1, 5);   // Small left circle

    if (digitalRead(BUTTON2_PIN) == HIGH)
      u8g.drawDisc(x2, y2, 10);  // Large right circle
    else
      u8g.drawDisc(x2, y2, 5);   // Small right circle

  } while (u8g.nextPage());
}
