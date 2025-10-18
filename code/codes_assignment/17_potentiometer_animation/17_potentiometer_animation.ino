#include <U8glib.h>

#define POT_PIN A0
#define BUTTON1_PIN 4
#define BUTTON2_PIN 2

U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);

void setup() {
  pinMode(BUTTON1_PIN, INPUT);
  pinMode(BUTTON2_PIN, INPUT);
  pinMode(POT_PIN, INPUT);
}

void loop() {
  drawPotTest();
  delay(100);
}

void drawPotTest() {
  int potValue = analogRead(POT_PIN);
  float t = map(potValue, 0, 1023, 0, 180);  // Ball moves deeper into spiral

  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x10);
    u8g.drawStr(35, 10, "Pot Spiral");

    int cx = 64, cy = 35;      // Center of spiral (below text)
    float angleStep = 0.08;    // Smaller step = smoother curve
    float a = 1.8;             // Bigger = faster radius increase (tighter spiral)

    // Draw spiral (mirrored)
    for (float theta = 0; theta < angleStep * 180; theta += angleStep) {
      float r = a * theta;
      int x = cx - r * cos(theta);  // Mirrored spiral
      int y = cy + r * sin(theta);
      u8g.drawPixel(x, y);
    }

    // Ball tracing the spiral
    float ballTheta = angleStep * t;
    float rBall = a * ballTheta;
    int ballX = cx - rBall * cos(ballTheta);  // Mirrored
    int ballY = cy + rBall * sin(ballTheta);
    u8g.drawCircle(ballX, ballY, 2);  // Tracing ball

  } while (u8g.nextPage());
}
