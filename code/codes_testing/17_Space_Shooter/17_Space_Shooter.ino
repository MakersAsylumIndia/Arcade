/************************************************************
 *  MINI ARCADE - GAME 4: SPACE SHOOTER (MULTI-BULLET)
 *  ---------------------------------------------------------
 *  Hardware: Arduino Nano
 *  Display: SH1106 128x64 OLED (I2C)
 *
 *  CONTROLS:
 *   - Potentiometer (A0): Move ship left/right
 *   - LEFT BTN (D4): Fire laser
 *   - RIGHT BTN (D2): Restart after game over
 *
 *  GAMEPLAY:
 *   - Shoot enemies descending from top.
 *   - +1 point for every hit.
 *   - Lose 1 life if an enemy reaches bottom.
 *   - Lose all 3 lives = Game Over.
 *   - Press RIGHT BTN to restart.
 *   - Fire rate increased, can shoot multiple bullets!
 ************************************************************/

#include <U8glib.h>

#define POT_PIN A0
#define FIRE_BTN 4
#define RESTART_BTN 2
#define BUZZER 3
#define HAPTIC 5

U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);

// Ship
int shipX;
const int shipY = 58;
const int shipW = 10;
const int shipH = 3;

// Bullet system
#define MAX_BULLETS 5
struct Bullet {
  bool active;
  int x, y;
};
Bullet bullets[MAX_BULLETS];

// Enemy
int enemyX, enemyY;
int enemySpeed = 2;

// Score + Lives
int score = 0;
int lives = 3;
bool gameOver = false;

// Timing
unsigned long lastFireTime = 0;
unsigned long fireCooldown = 150; // Faster fire rate!

void setup() {
  pinMode(POT_PIN, INPUT);
  pinMode(FIRE_BTN, INPUT);
  pinMode(RESTART_BTN, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(HAPTIC, OUTPUT);
  digitalWrite(BUZZER, LOW);
  digitalWrite(HAPTIC, LOW);
  randomSeed(analogRead(1));

  resetEnemy();
  resetBullets();
}

void loop() {
  if (!gameOver) {
    updateGame();
    drawGame();
    delay(40);
  } else {
    drawGameOver();
    if (digitalRead(RESTART_BTN) == HIGH) {
      resetGame();
    }
  }
}

/*----------------------------------------------------------
   GAME LOGIC
----------------------------------------------------------*/

void updateGame() {
  // Smooth potentiometer input for ship movement
  static float smoothVal = 0;
  int rawVal = analogRead(POT_PIN);
  smoothVal = 0.85 * smoothVal + 0.15 * rawVal;
  shipX = map(smoothVal, 0, 1023, 0, 118);

  // Fire button with shorter cooldown and multiple bullets
  if (digitalRead(FIRE_BTN) == HIGH && millis() - lastFireTime > fireCooldown) {
    fireBullet();
    lastFireTime = millis();
  }

  // Update bullet positions
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (bullets[i].active) {
      bullets[i].y -= 5;
      if (bullets[i].y < 0) bullets[i].active = false;
    }
  }

  // Move enemy downward
  enemyY += enemySpeed;

  // Bullet-enemy collision check
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (bullets[i].active &&
        bullets[i].y < enemyY + 6 && bullets[i].y > enemyY &&
        bullets[i].x > enemyX && bullets[i].x < enemyX + 8) {
      bullets[i].active = false;
      score++;
      resetEnemy();
      hitEffect();
      if (score % 5 == 0 && enemySpeed < 5) enemySpeed++; // Increase difficulty
    }
  }

  // Enemy reaches bottom
  if (enemyY > 64) {
    lives--;
    missEffect();
    resetEnemy();
    if (lives <= 0) {
      gameOver = true;
      return;
    }
  }
}

void fireBullet() {
  // Find first available bullet slot
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (!bullets[i].active) {
      bullets[i].active = true;
      bullets[i].x = shipX + shipW / 2;
      bullets[i].y = shipY - 4;
      fireEffect();
      break;
    }
  }
}

void resetBullets() {
  for (int i = 0; i < MAX_BULLETS; i++) {
    bullets[i].active = false;
  }
}

void resetEnemy() {
  enemyX = random(0, 120);
  enemyY = 0;
}

/*----------------------------------------------------------
   SOUND & HAPTIC FEEDBACK
----------------------------------------------------------*/

void fireEffect() {
  digitalWrite(BUZZER, HIGH);
  delay(20);
  digitalWrite(BUZZER, LOW);
}

void hitEffect() {
  digitalWrite(HAPTIC, HIGH);
  digitalWrite(BUZZER, HIGH);
  delay(60);
  digitalWrite(BUZZER, LOW);
  digitalWrite(HAPTIC, LOW);
}

void missEffect() {
  digitalWrite(BUZZER, HIGH);
  delay(100);
  digitalWrite(BUZZER, LOW);
}

/*----------------------------------------------------------
   GRAPHICS
----------------------------------------------------------*/

void drawGame() {
  u8g.firstPage();
  do {
    // Title and stats
    u8g.setFont(u8g_font_6x10);
    u8g.drawStr(0, 10, "SPACE SHOOTER");
    u8g.setPrintPos(90, 10);
    u8g.print("S:");
    u8g.print(score);
    u8g.setPrintPos(60, 10);
    u8g.print("L:");
    u8g.print(lives);

    // Ship
    u8g.drawBox(shipX, shipY, shipW, shipH);
    u8g.drawTriangle(shipX - 2, shipY, shipX + shipW + 2, shipY, shipX + shipW / 2, shipY - 6);

    // Enemy
    u8g.drawBox(enemyX, enemyY, 8, 6);

    // Bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
      if (bullets[i].active) {
        u8g.drawBox(bullets[i].x, bullets[i].y, 2, 4);
      }
    }

  } while (u8g.nextPage());
}

void drawGameOver() {
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x13B);
    u8g.drawStr(30, 30, "GAME OVER");

    u8g.setFont(u8g_font_6x10);
    char buf[20];
    sprintf(buf, "Score: %d", score);
    u8g.drawStr(40, 45, buf);
    u8g.drawStr(10, 60, "Press RIGHT to Restart");
  } while (u8g.nextPage());

  // Three short beeps once
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
   RESET GAME
----------------------------------------------------------*/

void resetGame() {
  score = 0;
  lives = 3;
  enemySpeed = 2;
  gameOver = false;
  resetEnemy();
  resetBullets();
}
