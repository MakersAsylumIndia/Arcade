/* 
===============================================================================
This code is part of the ARCADE project for the "Electronics and 
Microcontrollers" skill of INNOVATION SCHOOL at MAKERS ASYLUM

Copyright : 2025 Maker's Asylum | makersasylum.com
License : MIT

Summary of what this code does:
   Project Title : Simple Static Text Display on SH1106 OLED
   Description   :
     This minimal Arduino sketch demonstrates drawing static text
     lines on a 128x64 SH1106 OLED using the U8glib graphics library.
     The sketch repeatedly renders three text rows ("row number 1",
     "row number 2", "row number 3") and refreshes the display once
     per second.

   Hardware Setup :
     - OLED (SH1106 128x64) connected via I2C:
         VCC -> 5V
         GND -> GND
         SDA -> A4 (on UNO/Nano)
         SCL -> A5 (on UNO/Nano)
     - No other peripherals required.

   Operation Summary :
     - U8glib uses a paged drawing model to update displays with
       limited RAM. Each frame is drawn page-by-page between calls
       to firstPage() and nextPage().
     - The code sets a font and draws three text strings at fixed
       vertical positions on the screen. After finishing the page
       sequence the sketch waits 1 second and repeats.
   Libraries Required :
     - U8glib (by Oliver Kraus) for SH1106/SSD displays
===============================================================================
*/

#include <U8glib.h>                               // Include U8glib (graphics library) for OLED control

// Create a U8glib display object for the SH1106 128x64 OLED.
// The constructor argument U8G_I2C_OPT_NONE uses default I2C options.
// Variable name 'display' is used below to call drawing functions.
U8GLIB_SH1106_128X64 display(U8G_I2C_OPT_NONE); 

// -------------------------
// setup() - runs once
// -------------------------
void setup() {
  // Nothing required here specifically for U8glib: the display object
  // initializes itself when first used. If you use other I2C devices or
  // custom I2C settings you might call Wire.begin() here — but for the
  // default U8glib I2C usage this empty setup is acceptable.
}

// -------------------------
// loop() - runs repeatedly
// -------------------------
void loop() {
  // U8glib uses a paged drawing approach to update the display buffer.
  // firstPage() begins a new frame. The do/while block is executed once
  // per page; nextPage() advances to the next page until the frame is complete.
  display.firstPage();
  do {
    // Set the font for subsequent text drawing operations.
    // u8g_font_6x13 is a fixed-size bitmap font provided by U8glib.
    // Different fonts change text size and spacing; choose one that fits.
    display.setFont(u8g_font_6x13);

    // drawStr(x, y, "text") draws a string at the given coordinates.
    // Coordinates: x = pixels from left edge, y = baseline (not top) of text.
    // The chosen y values are spaced to avoid overlapping the font's baseline.
    display.drawStr(0, 12, "row number 1");  // Draw the first line near the top
    display.drawStr(0, 26, "row number 2");  // Draw the second line a bit lower
    display.drawStr(0, 40, "row number 3");  // Draw the third line further down

    // Notes:
    // - Because y is the baseline for the font, values are chosen (12,26,40)
    //   to provide comfortable vertical spacing for the 6x13 font.
    // - If you change the font, you'll likely need to adjust these y positions.
    // - drawStr does not wrap text; ensure strings fit within 128 pixels width.
  } while (display.nextPage()); // Continue drawing pages until the entire frame is rendered

  // Pause so the displayed text remains visible for 1 second before redrawing.
  // Redrawing every second is fine for static content; for animations choose
  // a shorter delay or a timed loop using millis() for non-blocking timing.
  delay(1000);
}
