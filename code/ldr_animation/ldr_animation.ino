#include <U8glib.h>

#define LDR_PIN A1
#define BUTTON1_PIN 4
#define BUTTON2_PIN 2

U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);

void setup() {
  pinMode(BUTTON1_PIN, INPUT);
  pinMode(BUTTON2_PIN, INPUT);
  pinMode(LDR_PIN, INPUT);
}

void loop() {
  drawLDRTest();
  delay(100);
}

void drawLDRTest() {
  int ldrValue = analogRead(LDR_PIN);
  int glowLevel = map(ldrValue, 0, 1023, 0, 10);

  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x10);
    u8g.drawStr(35, 10, "LDR Sensor");

    int cx = 64, cy = 50;
    u8g.drawCircle(cx, cy, 8);
    u8g.drawLine(cx, cy + 8, cx, cy + 14);
    u8g.drawLine(cx - 5, cy + 14, cx + 5, cy + 14);

    for (int i = 0; i < glowLevel; i++) {
      int y = cy - 5 - i * 2;
      int w = 4 + i * 2;
      u8g.drawLine(cx - w, y, cx + w, y);
    }

  } while (u8g.nextPage());
}
