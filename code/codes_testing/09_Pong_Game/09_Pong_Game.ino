/* =============================================================================
   MINI ARCADE - SINGLE PLAYER PONG GAME (Restart with Dual Button Press)
   ---------------------------------------------------------------------------
   Hardware: Arduino Nano + 1.3" SH1106 128x64 OLED Display
   Purpose : Fun Pong game using ultrasonic sensor and basic AI.
             Adds restart control via both buttons pressed together.

   HARDWARE CONNECTIONS:
   * OLED (SH1106 128x64) → I2C (A4 = SDA, A5 = SCL)
   * Ultrasonic Sensor:
       - TRIG → D7
       - ECHO → D6
   * Buzzer → D3 (via NPN transistor)
   * Haptic Motor → D5
   * LEFT Button → D4 (with pull-down resistor)
   * RIGHT Button → D2 (with pull-down resistor)

   HOW TO PLAY:
   * Move your hand in front of the ultrasonic sensor to move your paddle.
   * The paddle on the left is you; the right paddle is the AI.
   * The game ends when one player leads by 3 points.
   * After “GAME OVER,” press both buttons together to start a new game.
   =============================================================================
*/

#include <U8glib.h>

#define TRIG_PIN 7
#define ECHO_PIN 6
#define BUZZER_PIN 3
#define HAPTIC_PIN 5
#define BUTTON_LEFT 4
#define BUTTON_RIGHT 2

U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);

// --- Game parameters ---
int paddleHeight = 12;
int paddleWidth = 2;
int ballSize = 3;

int leftPaddleY = 26;
int rightPaddleY = 26;
int ballX = 64;
int ballY = 32;
int ballSpeedX = 2;
int ballSpeedY = 1;
int baseSpeed = 2;

int playerScore = 0;
int aiScore = 0;
bool gameOver = false;

// -------------------- Ultrasonic distance reading --------------------
long readDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 20000);  // Timeout = 20 ms
  long distance = duration * 0.034 / 2;            // Convert to cm

  if (distance < 5) distance = 5;
  if (distance > 30) distance = 30;
  return distance;
}

// -------------------- Setup --------------------
void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(HAPTIC_PIN, OUTPUT);
  pinMode(BUTTON_LEFT, INPUT);
  pinMode(BUTTON_RIGHT, INPUT);

  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(HAPTIC_PIN, LOW);
  randomSeed(analogRead(0));

  resetBall(-1);
}

// -------------------- Main Loop --------------------
void loop() {
  if (!gameOver) {
    // === 1. Player paddle control via ultrasonic sensor ===
    long distance = readDistance();
    leftPaddleY = map(distance, 5, 30, 0, 52);

    // === 2. AI control with handicap ===
    int aiError = random(-3, 4);
    if (random(0, 10) < 8) {
      if (ballY > rightPaddleY + paddleHeight / 2 + aiError) rightPaddleY += 1;
      if (ballY < rightPaddleY + paddleHeight / 2 + aiError) rightPaddleY -= 1;
    }
    rightPaddleY = constrain(rightPaddleY, 0, 64 - paddleHeight);

    // === 3. Ball movement ===
    ballX += ballSpeedX;
    ballY += ballSpeedY;

    // Bounce off top/bottom
    if (ballY <= 0 || ballY >= 64 - ballSize) {
      ballSpeedY = -ballSpeedY;
      playBounceSound();
    }

    // === 4. Player paddle collision ===
    if (ballX <= 4 &&
        ballY + ballSize >= leftPaddleY &&
        ballY <= leftPaddleY + paddleHeight) {
      ballSpeedX = abs(ballSpeedX);
      accelerateBall();
      playBounceSound();
      pulseHaptic(50);
    }

    // === 5. AI paddle collision ===
    if (ballX >= 128 - 4 - paddleWidth &&
        ballY + ballSize >= rightPaddleY &&
        ballY <= rightPaddleY + paddleHeight) {
      ballSpeedX = -abs(ballSpeedX);
      accelerateBall();
      playBounceSound();
    }

    // === 6. Scoring ===
    if (ballX < 0) {
      aiScore++;
      resetBall(1);
    } else if (ballX > 128) {
      playerScore++;
      resetBall(-1);
    }

    // === 7. Game-over condition ===
    if (abs(playerScore - aiScore) >= 3) {
      gameOver = true;
      tone(BUZZER_PIN, 500, 400);
      pulseHaptic(200);
    }

    // === 8. Draw game ===
    drawGame();

    delay(40);
  } else {
    // === Game Over Screen ===
    drawGameOver();

    // Wait for both buttons to be pressed together to restart
    if (digitalRead(BUTTON_LEFT) == HIGH && digitalRead(BUTTON_RIGHT) == HIGH) {
      delay(200);  // Debounce
      resetGame();
    }
  }
}

// -------------------- Reset ball to center --------------------
void resetBall(int direction) {
  ballX = 64;
  ballY = 32;
  ballSpeedX = direction * baseSpeed;
  ballSpeedY = random(-1, 2);
  baseSpeed = 2;
  tone(BUZZER_PIN, 700, 100);
  pulseHaptic(100);
  delay(500);
}

// -------------------- Reset full game --------------------
void resetGame() {
  playerScore = 0;
  aiScore = 0;
  gameOver = false;
  resetBall(-1);
}

// -------------------- Increase ball speed after each hit --------------------
void accelerateBall() {
  if (ballSpeedX > 0)
    ballSpeedX++;
  else
    ballSpeedX--;
  if (ballSpeedY > 0)
    ballSpeedY++;
  else
    ballSpeedY--;
  if (abs(ballSpeedX) > 4) ballSpeedX = (ballSpeedX > 0) ? 4 : -4;
  if (abs(ballSpeedY) > 3) ballSpeedY = (ballSpeedY > 0) ? 3 : -3;
}

// -------------------- Sound based on ball speed --------------------
void playBounceSound() {
  int pitch = 700 + (abs(ballSpeedX) * 150);
  tone(BUZZER_PIN, pitch, 50);
}

// -------------------- Haptic feedback --------------------
void pulseHaptic(int duration) {
  digitalWrite(HAPTIC_PIN, HIGH);
  delay(duration);
  digitalWrite(HAPTIC_PIN, LOW);
}

// -------------------- Draw gameplay --------------------
void drawGame() {
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_5x7);

    // Center divider
    for (int y = 0; y < 64; y += 4)
      u8g.drawLine(64, y, 64, y + 2);

    // Player paddle
    u8g.drawBox(2, leftPaddleY, paddleWidth, paddleHeight);

    // AI paddle
    u8g.drawBox(124, rightPaddleY, paddleWidth, paddleHeight);

    // Ball
    u8g.drawBox(ballX, ballY, ballSize, ballSize);

    // Scores
    char buf[10];
    sprintf(buf, "%d", playerScore);
    u8g.drawStr(54, 10, buf);
    sprintf(buf, "%d", aiScore);
    u8g.drawStr(70, 10, buf);

    // Hint
    u8g.setFont(u8g_font_4x6);
    u8g.drawStr(10, 62, "Move hand to play!");
  } while (u8g.nextPage());
}

// -------------------- Game Over Screen --------------------
void drawGameOver() {
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x13B);
    u8g.drawStr(25, 25, "GAME OVER");

    u8g.setFont(u8g_font_6x10);
    if (playerScore > aiScore)
      u8g.drawStr(30, 45, "You Win!");
    else
      u8g.drawStr(30, 45, "AI Wins!");

    char buf[20];
    sprintf(buf, "Score %d - %d", playerScore, aiScore);
    u8g.drawStr(30, 60, buf);

    // Instruction to restart
    u8g.setFont(u8g_font_5x8);
    u8g.drawStr(10, 12, "Press both buttons to restart");
  } while (u8g.nextPage());
}
