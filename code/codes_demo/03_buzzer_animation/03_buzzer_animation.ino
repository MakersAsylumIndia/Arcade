#include <U8glib.h>

#define BUTTON1_PIN 4
#define BUTTON2_PIN 2
#define BUZZER_PIN 3

U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);

void setup() {
  pinMode(BUTTON1_PIN, INPUT);
  pinMode(BUTTON2_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
}

void loop() {
  drawBuzzerTest();
  delay(100);
}

void drawBuzzerTest() {
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x10);
    u8g.drawStr(15, 10, "Press any Button");

    u8g.drawTriangle(30, 30, 30, 50, 45, 40);

    if (digitalRead(BUTTON1_PIN) == HIGH || digitalRead(BUTTON2_PIN) == HIGH) {
      digitalWrite(BUZZER_PIN, HIGH);
      u8g.drawCircle(50, 40, 5);
      u8g.drawCircle(55, 40, 8);
      u8g.drawCircle(60, 40, 11);
    } else {
      digitalWrite(BUZZER_PIN, LOW);
      u8g.drawLine(48, 30, 60, 50);
      u8g.drawLine(48, 50, 60, 30);
    }

  } while (u8g.nextPage());
}
