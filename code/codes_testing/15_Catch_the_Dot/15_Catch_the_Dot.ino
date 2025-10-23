/************************************************************
 *  MINI ARCADE - GAME 2: CATCH THE DOT
 *  ---------------------------------------------------------
 *  Hardware: Arduino Nano
 *  Display: SH1106 128x64 OLED (I2C)
 *
 *  GAMEPLAY:
 *  - Move paddle using potentiometer (A0)
 *  - Catch the falling dot to earn +1 point
 *  - Miss it → lose 1 point
 *  - Game ends at +5 (win) or -3 (lose)
 *  - LEFT BTN restarts the game
 *
 *  HARDWARE:
 *   OLED (SH1106): SDA → A4, SCL → A5
 *   POTENTIOMETER: Wiper → A0
 *   LEFT BTN: D4 (with pulldown)
 *   BUZZER: D3 → transistor → buzzer → +5V
 *   HAPTIC MOTOR: D5 → transistor → motor → +5V
 ************************************************************/

#include <U8glib.h>

#define POT_PIN A0
#define LEFT_BTN 4
#define BUZZER 3
#define HAPTIC 5

U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);

int score = 0;
int dotX, dotY;
int paddleX;
int dotSpeed = 3;
bool gameOver = false;

void setup() {
  pinMode(POT_PIN, INPUT);
  pinMode(LEFT_BTN, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(HAPTIC, OUTPUT);
  digitalWrite(BUZZER, LOW);
  digitalWrite(HAPTIC, LOW);
  
  randomSeed(analogRead(1));
  resetDot();
}

void loop() {
  if (!gameOver) {
    updateGame();
    drawGame();
    delay(80);
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

// Update dot position and paddle movement
void updateGame() {
  // Smooth potentiometer input → paddle position (0 to 118)
  static float smoothedValue = 0;
  int potValue = analogRead(POT_PIN);
  smoothedValue = 0.8 * smoothedValue + 0.2 * potValue; // smoothing
  paddleX = map(smoothedValue, 0, 1023, 0, 118);

  // Move dot downward
  dotY += dotSpeed;

  // Check if dot reached paddle area
  if (dotY >= 56) {
    if (dotX > paddleX && dotX < paddleX + 10) {
      score++;
      feedbackSuccess();
    } else {
      score--;
      feedbackFail();
    }

    // Reset for next drop
    resetDot();
  }

  // End game conditions
  if (score >= 5 || score <= -3) {
    gameOver = true;
  }
}

// Reset dot to top at random position
void resetDot() {
  dotX = random(5, 120);
  dotY = 0;
}

// Give feedback for successful catch
void feedbackSuccess() {
  digitalWrite(BUZZER, HIGH);
  digitalWrite(HAPTIC, HIGH);
  delay(100);
  digitalWrite(BUZZER, LOW);
  digitalWrite(HAPTIC, LOW);
}

// Feedback for missed dot
void feedbackFail() {
  digitalWrite(BUZZER, HIGH);
  delay(250);
  digitalWrite(BUZZER, LOW);
}

// Restart the entire game
void resetGame() {
  score = 0;
  gameOver = false;
  resetDot();
}

/*----------------------------------------------------------
   GRAPHICS
----------------------------------------------------------*/

void drawGame() {
  u8g.firstPage();
  do {
    // Title
    u8g.setFont(u8g_font_6x10);
    u8g.drawStr(0, 10, "CATCH THE DOT");
    
    // Draw score
    u8g.setPrintPos(95, 10);
    u8g.print("S:");
    u8g.print(score);

    // Draw paddle
    u8g.drawBox(paddleX, 58, 10, 3);

    // Draw falling dot
    u8g.drawDisc(dotX, dotY, 2);
    
  } while (u8g.nextPage());
}

void drawGameOver() {
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x13B);
    if (score >= 5) {
      u8g.drawStr(35, 30, "YOU WIN!");
    } else {
      u8g.drawStr(35, 30, "YOU LOSE!");
    }

    u8g.setFont(u8g_font_6x10);
    u8g.drawStr(20, 50, "Press LEFT to Restart");

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
