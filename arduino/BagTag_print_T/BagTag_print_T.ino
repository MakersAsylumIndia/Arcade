#include <Wire.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

Adafruit_8x8matrix matrix = Adafruit_8x8matrix();

void setup() {
  Serial.begin(9600);
  Serial.println("8x8 LED Matrix Test");

  // pass in the address
  matrix.begin(0x70);  
}

void loop() {   
  // scroll some text across the matrix
  matrix.setTextSize(1);
  // Set wrap to false for smooth scrollling
  matrix.setTextWrap(false);  
  matrix.setTextColor(LED_ON);
  for (int8_t x=0; x>=-36; x--) {
    matrix.clear();
    matrix.setCursor(x,0);
    matrix.print(" Maker");
    matrix.writeDisplay();
    delay(200);
  }
}
