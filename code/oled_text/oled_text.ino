#include <U8glib.h>

// Use SH1106 OLED with U8glib
U8GLIB_SH1106_128X64 display(U8G_I2C_OPT_NONE); 

void setup() {
  // Nothing needed here for U8glib
}

void loop() {
  display.firstPage();
  do {
    display.setFont(u8g_font_6x13);
    display.drawStr(0, 12, "row number 1");
    display.drawStr(0, 26, "row number 2");
    display.drawStr(0, 40, "row number 3");
  } while (display.nextPage());

  delay(1000);
}
