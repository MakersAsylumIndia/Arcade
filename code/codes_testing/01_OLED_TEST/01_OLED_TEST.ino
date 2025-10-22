/* =============================================================================
   OLED TEST
   Hardware: 1.3" SH1106 128x64 OLED (I2C)
   Purpose : Verify the OLED display connection and basic text rendering.
   =============================================================================
*/

#include <U8glib.h>

U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);

void setup() {
  // No special setup needed for U8glib
}

void loop() {
  // Draw text on the OLED
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x13B);
    u8g.drawStr(10, 30, "OLED Display Test");
  } while (u8g.nextPage());

  delay(1000);
}
