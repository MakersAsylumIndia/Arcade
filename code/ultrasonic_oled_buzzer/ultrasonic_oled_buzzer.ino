#include <U8glib.h>

// OLED setup
U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);

// Ultrasonic pins
#define trigPin 8
#define echoPin 7

// Buzzer pin
#define buzzerPin 3

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
}

void loop() {
  long duration, distance;

  // Trigger ultrasonic pulse
  digitalWrite(trigPin, LOW); 
  delayMicroseconds(2); 
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read echo time
  duration = pulseIn(echoPin, HIGH);
  distance = (duration / 2) / 29.1;  // Convert to cm

  // === OLED Display ===
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x13B);
    u8g.drawStr(0, 15, "Ultrasonic Sensor:");

    u8g.setFont(u8g_font_6x10);
    if (distance >= 50 || distance <= 0) {
      u8g.drawStr(0, 40, "Distance: Out of range");
    } else {
      char buf[20];
      sprintf(buf, "Distance: %ld cm", distance);
      u8g.drawStr(0, 40, buf);
    }
  } while (u8g.nextPage());

  // === Buzzer control ===
  if (distance >= 50 || distance <= 0) {
    digitalWrite(buzzerPin, HIGH);  // Beep
  } else {
    digitalWrite(buzzerPin, LOW);   // Silence
  }

  delay(500);  // Wait before next reading
}
