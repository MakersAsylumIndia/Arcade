#include <U8glib.h>

#define POT_PIN A0
#define BUTTON1_PIN 4
#define BUTTON2_PIN 2
#define BUZZER_PIN 3
#define LDR_PIN A1

U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);

const char* menuItems[] = {"1. Button", "2. Buzzer", "3. LDR", "4. Pot", "5. Ultra"};
const int totalItems = 5;
int selectedItem = 0;
bool inMenu = true;
bool inButtonTest = false;
bool inBuzzerTest = false;
bool inLDRTest = false;
bool inPotTest = false; // Added flag for Pot Test mode

void setup() {
  pinMode(BUTTON1_PIN, INPUT);
  pinMode(BUTTON2_PIN, INPUT);
  pinMode(LDR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
}

void loop() {
  if (inMenu) {
    updateSelection();
    u8g.firstPage();
    do {
      drawMenu();
    } while (u8g.nextPage());

    if (buttonPressedOnce()) {
      if (selectedItem == 0) {
        inMenu = false;
        inButtonTest = true;
      } else if (selectedItem == 1) {
        inMenu = false;
        inBuzzerTest = true;
      } else if (selectedItem == 2) {
        inMenu = false;
        inLDRTest = true;
      } else if (selectedItem == 3) {
        inMenu = false;
        inPotTest = true;
      }
    }
  } 
  else if (inButtonTest) {
    drawButtonTest();
    if (digitalRead(BUTTON1_PIN) == HIGH && digitalRead(BUTTON2_PIN) == HIGH) {
      delay(300);
      while (digitalRead(BUTTON1_PIN) == HIGH || digitalRead(BUTTON2_PIN) == HIGH);
      inMenu = true;
      inButtonTest = false;
    }
  } 
  else if (inBuzzerTest) {
    drawBuzzerTest();
    if (digitalRead(BUTTON1_PIN) == HIGH || digitalRead(BUTTON2_PIN) == HIGH) {
      digitalWrite(BUZZER_PIN, HIGH);
    } else {
      digitalWrite(BUZZER_PIN, LOW);
    }
    if (digitalRead(BUTTON1_PIN) == HIGH && digitalRead(BUTTON2_PIN) == HIGH) {
      delay(300);
      while (digitalRead(BUTTON1_PIN) == HIGH || digitalRead(BUTTON2_PIN) == HIGH);
      digitalWrite(BUZZER_PIN, LOW);
      inMenu = true;
      inBuzzerTest = false;
    }
  }
  else if (inLDRTest) {
    drawLDRTest();
    if (digitalRead(BUTTON1_PIN) == HIGH && digitalRead(BUTTON2_PIN) == HIGH) {
      delay(300);
      while (digitalRead(BUTTON1_PIN) == HIGH || digitalRead(BUTTON2_PIN) == HIGH);
      inMenu = true;
      inLDRTest = false;
    }
  }
else if (inPotTest) {
  drawPotTest();
  if (digitalRead(BUTTON1_PIN) == HIGH && digitalRead(BUTTON2_PIN) == HIGH) {
    delay(300);
    while (digitalRead(BUTTON1_PIN) == HIGH || digitalRead(BUTTON2_PIN) == HIGH);
    inMenu = true;
    inPotTest = false;
  }
}


  delay(100);
}

void updateSelection() {
  int potValue = 1023 - analogRead(POT_PIN);
  int bandSize = 1024 / totalItems;
  selectedItem = potValue / bandSize;
  if (selectedItem >= totalItems) selectedItem = totalItems - 1;
}

void drawMenu() {
  u8g.setFont(u8g_font_6x10);
  u8g.drawStr(0, 10, "Select Component:");
  for (int i = 0; i < totalItems; i++) {
    if (i == selectedItem) {
      u8g.drawStr(0, 20 + i * 10, ">");
    }
    u8g.drawStr(10, 20 + i * 10, menuItems[i]);
  }
}

bool buttonPressedOnce() {
  static bool lastState = LOW;
  bool current = (digitalRead(BUTTON1_PIN) == HIGH || digitalRead(BUTTON2_PIN) == HIGH);

  if (current && !lastState) {
    lastState = true;
    delay(20);
    return true;
  } else if (!current) {
    lastState = false;
  }
  return false;
}

void drawButtonTest() {
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x10);
    u8g.drawStr(20, 10, "Press any Button");

    int x1 = 40, y1 = 40;
    int x2 = 90, y2 = 40;

    if (digitalRead(BUTTON1_PIN) == HIGH)
      u8g.drawDisc(x1, y1, 12);
    else
      u8g.drawDisc(x1, y1, 5);

    if (digitalRead(BUTTON2_PIN) == HIGH)
      u8g.drawDisc(x2, y2, 12);
    else
      u8g.drawDisc(x2, y2, 5);

  } while (u8g.nextPage());
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
