/************************************************************
 *  MINI ARCADE - GAME 3: AVOID THE OBSTACLES
 *  ---------------------------------------------------------
 *  Hardware: Arduino Nano
 *  Display: SH1106 128x64 OLED (I2C)
 *
 *  GAMEPLAY:
 *   - Move player left/right using the potentiometer (A0)
 *   - Avoid falling blocks (obstacles)
 *   - Each dodge = +1 point
 *   - Collision = lose 1 life
 *   - Lose 3 lives → Game Over
 *   - Press LEFT BTN to restart
 *
 *  HARDWARE CONNECTIONS:
 *   OLED (SH1106): SDA → A4, SCL → A5
 *   POTENTIOMETER: Wiper → A0
 *   LEFT BTN: D4 (with pulldown)
 *   BUZZER: D3 → NPN transistor → buzzer → +5V
 *   HAPTIC MOTOR: D5 → transistor → motor → +5V
 ************************************************************/

#include <U8glib.h>

#define POT_PIN A0
#define LEFT_BTN 4
#define BUZZER 3
#define HAPTIC 5

U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);

int playerX;
const int playerY = 58;
const int playerW = 10;
const int playerH = 3;

int obsX, obsY;
int obsSpeed = 3;
int score = 0;
int lives = 3;
bool gameOver = false;

void setup() {
  pinMode(POT_PIN, INPUT);
  pinMode(LEFT_BTN, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(HAPTIC, OUTPUT);
  digitalWrite(BUZZER, LOW);
  digitalWrite(HAPTIC, LOW);
  randomSeed(analogRead(1));

  resetObstacle();
}

void loop() {
  if (!gameOver) {
    updateGame();
    drawGame();
    delay(70);
  } else {
    drawGameOver();
    if (digitalRead(LEFT_BTN) == HIGH) {
      resetGame();
    }
  }
}

/*----------------------------------------------------------
   GAME LOGIC
----------------------------------------------------------*/

void updateGame() {
  // Smooth potentiometer → player position
  static float smoothVal = 0;
  int rawVal = analogRead(POT_PIN);
  smoothVal = 0.85 * smoothVal + 0.15 * rawVal;
  playerX = map(smoothVal, 0, 1023, 0, 118);

  // Move obstacle downward
  obsY += obsSpeed;

  // Check for collision
  if (obsY >= playerY - playerH && obsY <= playerY + 2) {
    if (obsX < playerX + playerW && obsX + 6 > playerX) {
      // COLLISION DETECTED
      lives--;
      feedbackCrash();
      resetObstacle();
      if (lives <= 0) {
        gameOver = true;
        return;
      }
    }
  }

  // If obstacle goes off-screen → score +1
  if (obsY > 64) {
    score++;
    resetObstacle();

    // Gradually increase difficulty
    if (score % 3 == 0 && obsSpeed < 6) {
      obsSpeed++;
    }
  }
}

void resetObstacle() {
  obsX = random(0, 122);
  obsY = 0;
}

// Haptic + buzzer feedback on hit
void feedbackCrash() {
  digitalWrite(BUZZER, HIGH);
  digitalWrite(HAPTIC, HIGH);
  delay(150);
  digitalWrite(BUZZER, LOW);
  digitalWrite(HAPTIC, LOW);
}

/*----------------------------------------------------------
   GRAPHICS
----------------------------------------------------------*/

void drawGame() {
  u8g.firstPage();
  do {
    // Title
    u8g.setFont(u8g_font_6x10);
    u8g.drawStr(0, 10, "AVOID THE BLOCKS");

    // Draw score and lives
    u8g.setPrintPos(90, 10);
    u8g.print("S:");
    u8g.print(score);
    u8g.setPrintPos(60, 10);
    u8g.print("L:");
    u8g.print(lives);

    // Draw player
    u8g.drawBox(playerX, playerY, playerW, playerH);

    // Draw obstacle
    u8g.drawBox(obsX, obsY, 6, 6);

  } while (u8g.nextPage());
}

void drawGameOver() {
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x13B);
    u8g.drawStr(30, 30, "GAME OVER");

    u8g.setFont(u8g_font_6x10);
    char buf[20];
    sprintf(buf, "Final Score: %d", score);
    u8g.drawStr(25, 45, buf);
    u8g.drawStr(10, 60, "Press LEFT to Restart");
  } while (u8g.nextPage());

  // Three short beeps (once only)
  static bool beeped = false;
  if (!beeped) {
    for (int i = 0; i < 3; i++) {
      digitalWrite(BUZZER, HIGH);
      delay(100);
      digitalWrite(BUZZER, LOW);
      delay(100);
    }
    beeped = true;
  }
}

/*----------------------------------------------------------
   GAME RESET
----------------------------------------------------------*/
void resetGame() {
  score = 0;
  lives = 3;
  obsSpeed = 3;
  gameOver = false;
  resetObstacle();
}
