/* =============================================================================
   MINI ARCADE PROJECT – SPACE INVADERS (Potentiometer Control)
   ---------------------------------------------------------------------------
   Hardware: Arduino Nano + SH1106 128x64 OLED Display
   Purpose : Demonstrate simple animation, player control, and collision
             detection using basic Arduino logic.

   HARDWARE CONNECTIONS:
   * OLED Display (SH1106 128x64) → I2C (A4 = SDA, A5 = SCL)
   * Potentiometer (for movement) → A0
   * LEFT Button (Fire)           → D4  (with pull-down resistor)
   * RIGHT Button (Reset Game)    → D2  (with pull-down resistor)
   * Buzzer                       → D3  (via NPN transistor)
   * Haptic Motor                 → D5
   * Ultrasonic Sensor, LDR, etc. → Not used in this sketch

   HOW TO PLAY:
   * Move your ship left/right using the potentiometer.
   * Press LEFT button to shoot.
   * Invaders move slowly downward; shoot them before they reach the base.
   * When all invaders are destroyed or reach the bottom, the game ends.
   * Press RIGHT button to restart.
   =============================================================================
*/

#include <U8glib.h>

#define POT_PIN A0
#define BUTTON_FIRE 4
#define BUTTON_RESET 2
#define BUZZER_PIN 3
#define HAPTIC_PIN 5

U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);

// Player (ship) properties
int playerX = 60;       
int playerY = 56;       
int playerWidth = 8;

// Bullet properties
bool bulletActive = false;
int bulletX, bulletY;

// Invader grid
const int INV_ROWS = 2;
const int INV_COLS = 5;
bool invaderAlive[INV_ROWS][INV_COLS];
int invaderX[INV_ROWS][INV_COLS];
int invaderY[INV_ROWS][INV_COLS];

// Movement control
int invaderDir = 1;  
unsigned long lastInvaderMove = 0;
int invaderSpeed = 300;

// Game state
bool gameOver = false;
bool gameOverBeeped = false; // NEW: ensures the 3 beeps play only once

// -------------------- SETUP --------------------
void setup() {
  pinMode(BUTTON_FIRE, INPUT);
  pinMode(BUTTON_RESET, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(HAPTIC_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(HAPTIC_PIN, LOW);

  resetGame();
}

// -------------------- MAIN LOOP --------------------
void loop() {
  // If game is over, show message and wait for reset
  if (gameOver) {
    drawGameOver();

    // Play 3 short beeps once only
    if (!gameOverBeeped) {
      for (int i = 0; i < 3; i++) {
        tone(BUZZER_PIN, 500, 100);
        delay(150);
      }
      noTone(BUZZER_PIN);      // ensure buzzer is off after beeps
      gameOverBeeped = true;   // prevents repetition
    }

    // Wait for RIGHT button to restart
    if (digitalRead(BUTTON_RESET) == HIGH) {
      delay(200);
      resetGame();
    }
    return; // Stop running rest of loop when game over
  }

  // --- Player movement via potentiometer ---
  int potVal = analogRead(POT_PIN);
  playerX = map(potVal, 0, 1023, 0, 120 - playerWidth);

  // --- Fire bullet with LEFT button ---
  if (digitalRead(BUTTON_FIRE) == HIGH && !bulletActive) {
    bulletActive = true;
    bulletX = playerX + playerWidth / 2;
    bulletY = playerY - 2;
    tone(BUZZER_PIN, 800, 50);
    delay(100);
  }

  // --- Update bullet position ---
  if (bulletActive) {
    bulletY -= 3;
    if (bulletY < 0) bulletActive = false;
  }

  // --- Move invaders ---
  if (millis() - lastInvaderMove > invaderSpeed) {
    lastInvaderMove = millis();
    bool edgeReached = false;

    // Move horizontally
    for (int r = 0; r < INV_ROWS; r++) {
      for (int c = 0; c < INV_COLS; c++) {
        if (invaderAlive[r][c]) {
          invaderX[r][c] += invaderDir * 4;
          if (invaderX[r][c] < 0 || invaderX[r][c] > 120) edgeReached = true;
        }
      }
    }

    // Drop down and reverse direction
    if (edgeReached) {
      invaderDir *= -1;
      for (int r = 0; r < INV_ROWS; r++) {
        for (int c = 0; c < INV_COLS; c++) {
          invaderY[r][c] += 4;
          if (invaderY[r][c] >= 56) {
            gameOver = true;  // Invader reached bottom
          }
        }
      }
    }
  }

  // --- Bullet collision detection ---
  if (bulletActive) {
    for (int r = 0; r < INV_ROWS; r++) {
      for (int c = 0; c < INV_COLS; c++) {
        if (invaderAlive[r][c]) {
          int ix = invaderX[r][c];
          int iy = invaderY[r][c];
          if (bulletX >= ix && bulletX <= ix + 6 &&
              bulletY >= iy && bulletY <= iy + 4) {
            invaderAlive[r][c] = false;
            bulletActive = false;
            tone(BUZZER_PIN, 600, 80);
            pulseHaptic(50);
          }
        }
      }
    }
  }

  // --- Check if all invaders destroyed ---
  bool anyAlive = false;
  for (int r = 0; r < INV_ROWS; r++)
    for (int c = 0; c < INV_COLS; c++)
      if (invaderAlive[r][c]) anyAlive = true;

  if (!anyAlive) gameOver = true;

  // --- Draw everything ---
  drawGame();
  delay(40);
}

// -------------------- Reset Game --------------------
void resetGame() {
  for (int r = 0; r < INV_ROWS; r++) {
    for (int c = 0; c < INV_COLS; c++) {
      invaderAlive[r][c] = true;
      invaderX[r][c] = 10 + c * 22;
      invaderY[r][c] = 8 + r * 10;
    }
  }
  bulletActive = false;
  gameOver = false;
  gameOverBeeped = false; // reset beep flag
  invaderDir = 1;
  tone(BUZZER_PIN, 700, 100);
  pulseHaptic(100);
}

// -------------------- Draw Gameplay --------------------
void drawGame() {
  u8g.firstPage();
  do {
    // Draw player ship
    u8g.drawBox(playerX, playerY, playerWidth, 2);
    u8g.drawTriangle(playerX, playerY, playerX + playerWidth / 2, playerY - 4,
                     playerX + playerWidth, playerY);

    // Draw bullet
    if (bulletActive) {
      u8g.drawBox(bulletX, bulletY, 1, 3);
    }

    // Draw invaders
    for (int r = 0; r < INV_ROWS; r++) {
      for (int c = 0; c < INV_COLS; c++) {
        if (invaderAlive[r][c]) {
          int x = invaderX[r][c];
          int y = invaderY[r][c];
          u8g.drawFrame(x, y, 6, 4);
          u8g.drawPixel(x + 2, y + 1);
          u8g.drawPixel(x + 4, y + 1);
          u8g.drawLine(x, y + 3, x + 6, y + 3);
        }
      }
    }

    // HUD text
    u8g.setFont(u8g_font_5x7);
    u8g.drawStr(2, 10, "SPACE INVADERS");
    u8g.drawStr(90, 10, "FIRE=D4");

  } while (u8g.nextPage());
}

// -------------------- Draw Game Over --------------------
void drawGameOver() {
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x13B);
    u8g.drawStr(30, 30, "GAME OVER");
    u8g.setFont(u8g_font_6x10);
    u8g.drawStr(15, 50, "Press RIGHT BTN to restart");
  } while (u8g.nextPage());
}

// -------------------- Haptic Pulse --------------------
void pulseHaptic(int duration) {
  digitalWrite(HAPTIC_PIN, HIGH);
  delay(duration);
  digitalWrite(HAPTIC_PIN, LOW);
}
