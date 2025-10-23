/*******************************************************
 *  MINI ARCADE - GAME 1: BUTTON DUEL (Reaction Game)
 *  Hardware: Arduino Nano
 *  ---------------------------------------------------
 *  Description:
 *  A two-player reaction game for learning Arduino basics.
 *  When the display shows “GO!”, both players try to press
 *  their buttons as fast as possible. The first one to press
 *  wins and gets feedback via buzzer and haptic motor.
 *  The game can be restarted easily after each round.
 *
 *  ---------------------------------------------------
 *  HARDWARE CONNECTIONS:
 *  - OLED (SH1106 128x64, I2C): SDA → A4, SCL → A5
 *  - LEFT BTN (Player A): D4  → Pulldown resistor to GND
 *  - RIGHT BTN (Player B): D2 → Pulldown resistor to GND
 *  - BUZZER: D3 → NPN transistor → buzzer → +5V
 *  - HAPTIC MOTOR: D5 → transistor driver → motor → +5V
 *  - Optional: A0 used for randomSeed() noise
 *******************************************************/

#include <U8glib.h>

#define LEFT_BTN 4
#define RIGHT_BTN 2
#define BUZZER 3
#define HAPTIC 5

U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);

bool gameStarted = false;     // True after player starts the game
bool waitingForReaction = false; // True after "GO!" appears
bool resultDisplayed = false; // True after someone wins

unsigned long startTime = 0;  // When "GO!" appeared
unsigned long delayBeforeGo;  // Random wait before "GO!"

void setup() {
  pinMode(LEFT_BTN, INPUT);
  pinMode(RIGHT_BTN, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(HAPTIC, OUTPUT);
  digitalWrite(BUZZER, LOW);
  digitalWrite(HAPTIC, LOW);

  randomSeed(analogRead(A0)); // Add some randomness
}

void loop() {

  // --- Main menu screen ---
  if (!gameStarted && !waitingForReaction && !resultDisplayed) {
    drawTitleScreen();

    // If LEFT button pressed, start game
    if (digitalRead(LEFT_BTN) == HIGH) {
      startGameAnimation();
      startReactionRound();
    }
  }

  // --- Waiting to show "GO!" ---
  else if (waitingForReaction) {
    if (millis() - startTime >= delayBeforeGo) {
      showGoSignal();
      waitingForReaction = false;
      resultDisplayed = false;
    }
  }

  // --- Reaction phase ---
  else if (!waitingForReaction && !resultDisplayed && gameStarted) {
    if (digitalRead(LEFT_BTN) == HIGH) {
      announceWinner("LEFT WINS!");
    } else if (digitalRead(RIGHT_BTN) == HIGH) {
      announceWinner("RIGHT WINS!");
    }
  }

  // --- Result displayed, wait for restart ---
  else if (resultDisplayed) {
    // Both buttons pressed → return to title
    if (digitalRead(LEFT_BTN) == HIGH && digitalRead(RIGHT_BTN) == HIGH) {
      resetGameToTitle();
    }
    // LEFT button pressed → new round
    else if (digitalRead(LEFT_BTN) == HIGH) {
      startGameAnimation();
      startReactionRound();
    }
  }
}

/*-------------------------------------------------------
   FUNCTION DEFINITIONS
-------------------------------------------------------*/

// Draws the title screen
void drawTitleScreen() {
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x13B);
    u8g.drawStr(25, 20, "BUTTON DUEL");
    u8g.setFont(u8g_font_6x10);
    u8g.drawStr(0, 45, "Press LEFT to Start");
    u8g.drawStr(0, 60, "Both BTN: Exit Round");
  } while (u8g.nextPage());
}

// Simple "Starting..." animation
void startGameAnimation() {
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x13B);
    u8g.drawStr(25, 35, "Starting");
  } while (u8g.nextPage());

  for (int i = 0; i < 3; i++) {
    delay(400);
    u8g.firstPage();
    do {
      u8g.setFont(u8g_font_6x13B);
      u8g.drawStr(25, 35, "Starting");
      for (int j = 0; j <= i; j++) {
        u8g.drawStr(90 + j * 5, 35, ".");
      }
    } while (u8g.nextPage());
  }
  delay(500);
}

// Starts the reaction timer with random delay
void startReactionRound() {
  delayBeforeGo = random(1000, 4000); // Random wait 1-4 sec
  startTime = millis();
  waitingForReaction = true;
  gameStarted = true;

  // Display "Get Ready"
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x13B);
    u8g.drawStr(30, 35, "Get Ready...");
  } while (u8g.nextPage());
}

// Displays "GO!" on screen
void showGoSignal() {
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_8x13B);
    u8g.drawStr(55, 40, "GO!");
  } while (u8g.nextPage());
}

// Display winner message and give feedback
void announceWinner(const char* message) {
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x13B);
    u8g.drawStr(20, 35, message);
  } while (u8g.nextPage());

  // Two short buzz/haptic pulses
  for (int i = 0; i < 2; i++) {
    digitalWrite(BUZZER, HIGH);
    digitalWrite(HAPTIC, HIGH);
    delay(150);
    digitalWrite(BUZZER, LOW);
    digitalWrite(HAPTIC, LOW);
    delay(150);
  }

  resultDisplayed = true;
  gameStarted = false;
}

// Reset to initial title screen
void resetGameToTitle() {
  gameStarted = false;
  waitingForReaction = false;
  resultDisplayed = false;
  delay(400);
}
