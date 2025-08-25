#include <U8glib.h>

#define BUTTON1_PIN 4
#define BUTTON2_PIN 2

U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);

void setup() {
  pinMode(BUTTON1_PIN, INPUT);
  pinMode(BUTTON2_PIN, INPUT);
}

void loop() {
  drawButtonTest();
  delay(100);
}

void drawButtonTest() {
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x10);
    u8g.drawStr(20, 10, "Press any Button");

    int x1 = 40, y1 = 40;
    int x2 = 90, y2 = 40;

    if (digitalRead(BUTTON1_PIN) == HIGH)
      u8g.drawDisc(x1, y1, 12);
    else
      u8g.drawDisc(x1, y1, 5);

    if (digitalRead(BUTTON2_PIN) == HIGH)
      u8g.drawDisc(x2, y2, 12);
    else
      u8g.drawDisc(x2, y2, 5);

  } while (u8g.nextPage());
}
