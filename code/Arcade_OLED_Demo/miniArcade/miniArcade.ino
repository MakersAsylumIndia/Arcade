#include <U8glib.h>

// OLED setup
U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);

// Button pins
const int leftButtonPin = 4;
const int rightButtonPin = 2;

// Paddle settings
const int paddleWidth = 2;
const int paddleHeight = 20;
int leftPaddleY = 22;
int rightPaddleY = 22;
int paddleSpeed = 4;  // Slightly faster

// Paddle directions
int leftDir = -1;
int rightDir = 1;

// Ball settings
int ballX = 64;
int ballY = 32;
float ballSpeedX = 4;
float ballSpeedY = 4;
const int ballSize = 3;

// Game state
enum GameState { START, PLAYING, GAME_OVER };
GameState state = START;

int score = 0;

void drawStartScreen() {
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_fub14);  // Slightly bigger
    u8g.drawStr(8, 28, "PING PONG");

    u8g.setFont(u8g_font_6x10);
    u8g.drawStr(12, 48, "Press both buttons");
    u8g.drawStr(40, 58, "to start");  // moved below
  } while (u8g.nextPage());
}

void drawGameOver() {
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x13B);
    u8g.drawStr(40, 25, "GAME OVER");
    u8g.setFont(u8g_font_6x10);
    u8g.drawStr(30, 45, "Score: ");
    char buf[4];
    itoa(score, buf, 10);
    u8g.drawStr(80, 45, buf);
  } while (u8g.nextPage());
}

void drawGame() {
  u8g.firstPage();
  do {
    // Score
    u8g.setFont(u8g_font_6x10);
    char buf[10];
    sprintf(buf, "Score:%d", score);
    u8g.drawStr(45, 10, buf);

    // Paddles and ball
    u8g.drawBox(2, leftPaddleY, paddleWidth, paddleHeight);
    u8g.drawBox(124, rightPaddleY, paddleWidth, paddleHeight);
    u8g.drawBox((int)ballX, (int)ballY, ballSize, ballSize);
  } while (u8g.nextPage());
}

void resetGame() {
  leftPaddleY = 22;
  rightPaddleY = 22;
  leftDir = -1;
  rightDir = 1;
  ballX = 64;
  ballY = 32;
  ballSpeedX = 4;
  ballSpeedY = 4;
  paddleSpeed = 6;
  score = 0;
}

void increaseBallSpeed() {
  if (score % 5 == 0) {
    ballSpeedX *= 1.1;
    ballSpeedY *= 1.1;
    paddleSpeed += 1;  // make paddles faster too
  }
}

void setup() {
  pinMode(leftButtonPin, INPUT_PULLUP);
  pinMode(rightButtonPin, INPUT_PULLUP);
}

void loop() {
  switch (state) {
    case START:
      drawStartScreen();
      if (digitalRead(leftButtonPin) == HIGH && digitalRead(rightButtonPin) == HIGH) {
        resetGame();
        state = PLAYING;
        delay(300); // debounce delay
      }
      break;

    case GAME_OVER:
      drawGameOver();
      delay(2000);  // Show game over for 2 seconds
      state = START;
      break;

    case PLAYING:
      // === Left Paddle ===
      if (digitalRead(leftButtonPin) == HIGH) {
        leftPaddleY += leftDir * paddleSpeed;
        if (leftPaddleY <= 0) { leftPaddleY = 0; leftDir = 1; }
        else if (leftPaddleY >= 64 - paddleHeight) { leftPaddleY = 64 - paddleHeight; leftDir = -1; }
      }

      // === Right Paddle ===
      if (digitalRead(rightButtonPin) == HIGH) {
        rightPaddleY += rightDir * paddleSpeed;
        if (rightPaddleY <= 0) { rightPaddleY = 0; rightDir = 1; }
        else if (rightPaddleY >= 64 - paddleHeight) { rightPaddleY = 64 - paddleHeight; rightDir = -1; }
      }

      // === Ball Movement ===
      ballX += ballSpeedX;
      ballY += ballSpeedY;

      if (ballY <= 0 || ballY >= 64 - ballSize) ballSpeedY *= -1;

      // Left paddle collision with fix
      if (ballX <= 5 && ballY + ballSize >= leftPaddleY && ballY <= leftPaddleY + paddleHeight) {
        ballX = 5;  // reposition ball just outside
        ballSpeedX *= -1;
        score++;
        increaseBallSpeed();
      }

      // Right paddle collision with fix
      if (ballX >= 128 - 5 - ballSize && ballY + ballSize >= rightPaddleY && ballY <= rightPaddleY + paddleHeight) {
        ballX = 128 - 5 - ballSize;  // reposition ball just outside
        ballSpeedX *= -1;
        score++;
        increaseBallSpeed();
      }

      // Out of bounds = game over
      if (ballX <= 0 || ballX >= 128 - ballSize) {
        state = GAME_OVER;
      }

      drawGame();
      delay(10);
      break;
  }
}
