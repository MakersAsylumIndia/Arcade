#include <U8glib.h>

#define TRIG_PIN 8
#define ECHO_PIN 7

U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

void loop() {
  // === Measure distance ===
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  int distance = duration * 0.034 / 2;  // convert to cm

  // === Draw animation ===
  drawUltra(distance);

  delay(100);
}

void drawUltra(int distance) {
  int roadY = 50;
  int carX;

  // Only show car if within 3–18 cm, else off-screen
  if (distance >= 3 && distance <= 18) {
    carX = map(distance, 18, 3, 0, 100);
  } else {
    carX = -20;  // off-screen
  }

  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x10);
    u8g.drawStr(30, 10, "Ultra Sensor");

    // Draw road
    u8g.drawLine(0, roadY, 128, roadY);

    // Draw car if in range
    if (distance >= 3 && distance <= 18) {
      // Body
      u8g.drawBox(carX, roadY - 8, 18, 6);
      // Roof
      u8g.drawFrame(carX + 3, roadY - 12, 10, 4);
      u8g.drawLine(carX + 3, roadY - 8, carX + 3, roadY - 12);
      u8g.drawLine(carX + 13, roadY - 8, carX + 13, roadY - 12);
      // Wheels
      u8g.drawDisc(carX + 4, roadY, 2);
      u8g.drawDisc(carX + 14, roadY, 2);
    }

    // Distance label at bottom
    char buf[6];
    sprintf(buf, "%d cm", distance);
    u8g.drawStr(45, 63, buf);

  } while (u8g.nextPage());
}
