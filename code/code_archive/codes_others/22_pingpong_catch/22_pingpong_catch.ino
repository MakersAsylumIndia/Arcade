#include <U8glib.h>

U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);

// === Pins ===
const int leftButtonPin = 4;
const int rightButtonPin = 2;
const int potPin = A0;
const int buzzerPin = 3;
const int startButtonPin = 5;
const int ldrPin = A1;

// === Global Game States ===
enum AppState { MENU, GAME_PINGPONG, GAME_CATCH, GAME_LDR, GAME_OVER_PINGPONG, GAME_OVER_CATCH };
AppState appState = MENU;

// === General Score ===
int score = 0;

// === Shared Buzzer Functions ===
void toneBounce() { tone(buzzerPin, 1000, 100); }
void toneCatch() { tone(buzzerPin, 1500, 100); }
void toneStart() { tone(buzzerPin, 1000, 200); }
void toneGameOver() {
  int melody[] = { 262, 294, 330, 262 };
  int duration[] = { 200, 200, 200, 400 };
  for (int i = 0; i < 4; i++) {
    tone(buzzerPin, melody[i], duration[i]);
    delay(duration[i] + 50);
  }
  noTone(buzzerPin);
}

// === Menu ===
int selectedGame = 0; // 0 = Ping Pong, 1 = Catch, 2 = LDR

void drawMenu() {
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x13B);
    u8g.drawStr(30, 12, "Select Game:");
    u8g.setFont(u8g_font_6x10);
    u8g.drawStr(20, 30, selectedGame == 0 ? "> Ping Pong" : "  Ping Pong");
    u8g.drawStr(20, 45, selectedGame == 1 ? "> Catch" : "  Catch");
    u8g.drawStr(20, 60, selectedGame == 2 ? "> LDR" : "  LDR");
  } while (u8g.nextPage());
}

// === Ping Pong Game Variables ===
const int paddleW = 2, paddleH = 20;
int leftY = 22, rightY = 22;
int paddleSpeed = 4, leftDir = -1, rightDir = 1;
int ballX = 64, ballY = 32, ballSize = 3;
float ballSpeedX = 4, ballSpeedY = 4;

void resetPingPong() {
  leftY = 22; rightY = 22;
  leftDir = -1; rightDir = 1;
  ballX = 64; ballY = 32;
  ballSpeedX = 4; ballSpeedY = 4;
  paddleSpeed = 4;
  score = 0;
  toneStart();
}

void drawPingPong() {
  u8g.firstPage();
  do {
    char buf[10];
    sprintf(buf, "Score:%d", score);
    u8g.setFont(u8g_font_6x10);
    u8g.drawStr(45, 10, buf);
    u8g.drawBox(2, leftY, paddleW, paddleH);
    u8g.drawBox(124, rightY, paddleW, paddleH);
    u8g.drawBox((int)ballX, (int)ballY, ballSize, ballSize);
  } while (u8g.nextPage());
}

void updatePingPong() {
  if (digitalRead(leftButtonPin) == HIGH) {
    leftY += leftDir * paddleSpeed;
    if (leftY <= 0) { leftY = 0; leftDir = 1; }
    else if (leftY >= 64 - paddleH) { leftY = 64 - paddleH; leftDir = -1; }
  }

  if (digitalRead(rightButtonPin) == HIGH) {
    rightY += rightDir * paddleSpeed;
    if (rightY <= 0) { rightY = 0; rightDir = 1; }
    else if (rightY >= 64 - paddleH) { rightY = 64 - paddleH; rightDir = -1; }
  }

  ballX += ballSpeedX;
  ballY += ballSpeedY;

  if (ballY <= 0 || ballY >= 64 - ballSize) ballSpeedY *= -1;

  if (ballX <= 5 && ballY + ballSize >= leftY && ballY <= leftY + paddleH) {
    ballX = 5;
    ballSpeedX *= -1;
    score++; toneBounce();
    if (score % 5 == 0) {
      ballSpeedX *= 1.1; ballSpeedY *= 1.1; paddleSpeed += 1;
    }
  }

  if (ballX >= 128 - 5 - ballSize && ballY + ballSize >= rightY && ballY <= rightY + paddleH) {
    ballX = 128 - 5 - ballSize;
    ballSpeedX *= -1;
    score++; toneBounce();
    if (score % 5 == 0) {
      ballSpeedX *= 1.1; ballSpeedY *= 1.1; paddleSpeed += 1;
    }
  }

  if (ballX <= 0 || ballX >= 128 - ballSize) {
    appState = GAME_OVER_PINGPONG;
  }
}

// === Catch Game Variables ===
int playerX = 64, playerY = 56, playerW = 18, playerH = 6;
int objX, objY = 0, objW = 10, objH = 6, objSpeed = 4;

void resetCatch() {
  objX = random(0, 128 - objW);
  objY = 0;
  objSpeed = 4;
  score = 0;
  toneStart();
}

void drawCatch() {
  u8g.firstPage();
  do {
    char buf[12];
    sprintf(buf, "Score: %d", score);
    u8g.setFont(u8g_font_6x10);
    u8g.drawStr(5, 10, buf);
    u8g.drawBox(playerX, playerY, playerW, playerH);
    u8g.drawBox(objX, objY, objW, objH);
  } while (u8g.nextPage());
}

bool checkCatch() {
  return !(playerX + playerW < objX || playerX > objX + objW || playerY > objY + objH);
}

void updateCatch() {
  objY += objSpeed;

  if (objY > 64) {
    appState = GAME_OVER_CATCH;
    tone(buzzerPin, 500, 300);
    return;
  }

  if (checkCatch()) {
    score++;
    objY = 0;
    objX = random(0, 128 - objW);
    if (score % 5 == 0) objSpeed++;
    toneCatch();
  }
}

// === LDR Game Functions ===
int ldrDark = 120;  // Set your own LDR dark value
int ldrBright = 315;  // Set your own LDR bright value

void drawLDR() {
  int rawLDR = analogRead(ldrPin);
  int lightLevel = map(rawLDR, ldrDark, ldrBright, 0, 10);
  lightLevel = constrain(lightLevel, 0, 10);  // Ensure it stays in bounds

  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x13);
    u8g.drawStr(10, 14, "Light Intensity:");
    char buf[10];
    sprintf(buf, "Level: %d", lightLevel);
    u8g.drawStr(10, 30, buf);

    for (int i = 0; i < lightLevel; i++) {
      u8g.drawBox(10 + i * 10, 50, 8, 10);
    }
  } while (u8g.nextPage());
}

void updateLDR() {
  drawLDR();
  if (digitalRead(leftButtonPin) == HIGH && digitalRead(rightButtonPin) == HIGH) {
    delay(500);
    appState = MENU;
  }
}

// === Setup & Loop ===
void setup() {
  pinMode(leftButtonPin, INPUT_PULLUP);
  pinMode(rightButtonPin, INPUT_PULLUP);
  pinMode(startButtonPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  pinMode(ldrPin, INPUT);
  randomSeed(analogRead(0));
}

void loop() {
  switch (appState) {
    case MENU:
      drawMenu();
      if (digitalRead(rightButtonPin) == HIGH) {
        selectedGame = (selectedGame + 1) % 3;
        delay(200);
      }
      if (digitalRead(leftButtonPin) == HIGH) {
        if (selectedGame == 0) {
          resetPingPong();
          appState = GAME_PINGPONG;
        } else if (selectedGame == 1) {
          resetCatch();
          appState = GAME_CATCH;
        } else {
          appState = GAME_LDR;
        }
        delay(300);
      }
      break;

    case GAME_PINGPONG:
      updatePingPong();
      drawPingPong();
      delay(10);
      break;

    case GAME_CATCH:
      playerX = map(analogRead(potPin), 0, 1023, 0, 128 - playerW);
      updateCatch();
      drawCatch();
      delay(10);
      break;

    case GAME_LDR:
      updateLDR();
      delay(500);
      break;

    case GAME_OVER_PINGPONG:
      toneGameOver();
      delay(2000);
      appState = MENU;
      break;

    case GAME_OVER_CATCH:
      toneGameOver();
      delay(2000);
      appState = MENU;
      break;
  }
}
