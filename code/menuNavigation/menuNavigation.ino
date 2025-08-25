#include <U8glib.h>

#define POT_PIN A0

U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);  // For SH1106

const char* menuItems[] = {
  "1. Button",
  "2. Buzzer",
  "3. LDR",
  "4. Potentiometer",
  "5. Ultrasonic"
};
const int totalItems = sizeof(menuItems) / sizeof(menuItems[0]);

int selectedItem = 0;

void drawMenu() {
  u8g.setFont(u8g_font_6x10);
  u8g.drawStr(0, 10, "Select Component:");

  for (int i = 0; i < totalItems; i++) {
    int y = 22 + (i * 10);
    if (i == selectedItem) {
      u8g.drawStr(0, y, ">");
    }
    u8g.drawStr(10, y, menuItems[i]);
  }
}

void updateSelection() {
  int potValue = 1023 - analogRead(POT_PIN);  // Fix scroll direction

  int bandSize = 1024 / totalItems;
  selectedItem = potValue / bandSize;

  if (selectedItem >= totalItems) {
    selectedItem = totalItems - 1;
  }
}


void setup() {
  // No setup needed for U8glib SH1106
}

void loop() {
  updateSelection();

  u8g.firstPage();
  do {
    drawMenu();
  } while (u8g.nextPage());

  delay(100); // Update rate
}
