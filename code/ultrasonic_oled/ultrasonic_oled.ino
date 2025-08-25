#include <U8glib.h>

// OLED setup
U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);

// Ultrasonic pins
const int trigPin = 8;
const int echoPin = 7;

long duration;
int distance;
int contrast = 0;

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
}

void loop() {
  // Trigger ultrasonic pulse
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Measure distance
  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;

  // Map distance to brightness (screen density simulation)
  int minDist = 3;
  int maxDist = 10;
  distance = constrain(distance, minDist, maxDist);
  contrast = map(distance, minDist, maxDist, 255, 0);

  // Draw glow pattern (horizontal lines to simulate brightness)
  u8g.firstPage();
  do {
    int fillLines = map(contrast, 0, 255, 0, 64); // more lines = brighter
    for (int i = 0; i < fillLines; i += 2) {
      u8g.drawBox(0, i, 128, 1); // draw line
    }
  } while (u8g.nextPage());

  delay(100);
}
