/*
================================================================================
  MINI ARCADE - SIMPLE SNAKE GAME
  --------------------------------
  Hardware: Arduino Nano + SH1106 128x64 OLED (I2C) + Buzzer via NPN transistor +
             Potentiometer (A0) + LEFT button (D4)

  HOW TO PLAY:
    - Rotate the potentiometer to choose direction sector (LEFT / UP / RIGHT / DOWN)
    - Press the LEFT button (D4) to confirm and change direction.
    - Avoid hitting the walls or yourself!
    - When the snake eats food, it grows.
    - When you lose, the buzzer beeps 3 times. Press LEFT again to start a new game.

  SMOOTHING:
    - Potentiometer input is filtered using Exponential Moving Average (EMA)
      to avoid jitter when selecting direction.

  HARDWARE CONNECTIONS:
    * OLED (SH1106 128x64): I2C -> SDA, SCL
    * Potentiometer wiper: A0
    * LEFT Button (with pulldown): D4
    * Buzzer via NPN transistor: D3 (base resistor required)
================================================================================
*/

#include <U8glib.h>

#define POT_PIN A0
#define LEFT_BTN 4
#define BUZZER_PIN 3

// === OLED Display Setup ===
U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);

// === Game parameters ===
#define GRID_SIZE 4         // Each grid cell = 4x4 pixels
#define GRID_WIDTH  (128 / GRID_SIZE)
#define GRID_HEIGHT (64 / GRID_SIZE)
#define MAX_SNAKE_LENGTH 100

// === Direction definitions ===
#define DIR_UP    0
#define DIR_RIGHT 1
#define DIR_DOWN  2
#define DIR_LEFT  3

// === Potentiometer smoothing ===
const float POT_ALPHA = 0.12;  // EMA smoothing factor
float potEMA = 0;

// === Button debounce ===
unsigned long lastLeftPress = 0;
const unsigned long DEBOUNCE_MS = 200;

// === Game state ===
int snakeX[MAX_SNAKE_LENGTH];
int snakeY[MAX_SNAKE_LENGTH];
int snakeLength;
int direction;
int foodX, foodY;
bool gameOver = false;

// === Function prototypes ===
void startGame();
void updateGame();
void drawGame();
void moveSnake();
void generateFood();
void beep(int count);
void checkCollision();
void readInput();

// ---------------------------------------------------------------------------

void setup() {
  pinMode(LEFT_BTN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  potEMA = analogRead(POT_PIN);
  startGame();
}

// ---------------------------------------------------------------------------

void loop() {
  readInput();    // Check pot and button for direction updates

  if (!gameOver) {
    moveSnake();
    checkCollision();
    updateGame();
  } else {
    // Wait for button press to restart
    if (digitalRead(LEFT_BTN) == HIGH && (millis() - lastLeftPress) > DEBOUNCE_MS) {
      lastLeftPress = millis();
      startGame();
    }
  }

  delay(150); // Game speed
}

// ---------------------------------------------------------------------------

void startGame() {
  snakeLength = 3;
  direction = DIR_RIGHT;
  gameOver = false;

  // Initialize snake in the center
  int startX = GRID_WIDTH / 2;
  int startY = GRID_HEIGHT / 2;
  for (int i = 0; i < snakeLength; i++) {
    snakeX[i] = startX - i;
    snakeY[i] = startY;
  }

  generateFood();
}

// ---------------------------------------------------------------------------

void updateGame() {
  // Draw everything
  u8g.firstPage();
  do {
    drawGame();
  } while (u8g.nextPage());
}

// ---------------------------------------------------------------------------

void drawGame() {
  // Draw border
  u8g.drawFrame(0, 0, 128, 64);

  // Draw food
  u8g.drawBox(foodX * GRID_SIZE, foodY * GRID_SIZE, GRID_SIZE, GRID_SIZE);

  // Draw snake
  for (int i = 0; i < snakeLength; i++) {
    u8g.drawBox(snakeX[i] * GRID_SIZE, snakeY[i] * GRID_SIZE, GRID_SIZE, GRID_SIZE);
  }

  // If game over, overlay text
  if (gameOver) {
    u8g.setFont(u8g_font_6x13);
    u8g.drawStr(30, 32, "GAME OVER!");
    u8g.setFont(u8g_font_6x10);
    u8g.drawStr(15, 48, "Press LEFT to restart");
  }
}

// ---------------------------------------------------------------------------

void moveSnake() {
  // Move body
  for (int i = snakeLength - 1; i > 0; i--) {
    snakeX[i] = snakeX[i - 1];
    snakeY[i] = snakeY[i - 1];
  }

  // Move head
  if (direction == DIR_UP) snakeY[0]--;
  if (direction == DIR_DOWN) snakeY[0]++;
  if (direction == DIR_LEFT) snakeX[0]--;
  if (direction == DIR_RIGHT) snakeX[0]++;

  // Wrap around (optional)
  // snakeX[0] = (snakeX[0] + GRID_WIDTH) % GRID_WIDTH;
  // snakeY[0] = (snakeY[0] + GRID_HEIGHT) % GRID_HEIGHT;

  // Check if snake ate food
  if (snakeX[0] == foodX && snakeY[0] == foodY) {
    if (snakeLength < MAX_SNAKE_LENGTH) snakeLength++;
    generateFood();
  }
}

// ---------------------------------------------------------------------------

void checkCollision() {
  // Check wall collision
  if (snakeX[0] < 0 || snakeX[0] >= GRID_WIDTH || snakeY[0] < 0 || snakeY[0] >= GRID_HEIGHT) {
    gameOver = true;
    beep(3);
    return;
  }

  // Check self-collision
  for (int i = 1; i < snakeLength; i++) {
    if (snakeX[0] == snakeX[i] && snakeY[0] == snakeY[i]) {
      gameOver = true;
      beep(3);
      return;
    }
  }
}

// ---------------------------------------------------------------------------

void generateFood() {
  foodX = random(1, GRID_WIDTH - 2);
  foodY = random(1, GRID_HEIGHT - 2);
}

// ---------------------------------------------------------------------------

void readInput() {
  // Smooth potentiometer
  int raw = analogRead(POT_PIN);
  potEMA = POT_ALPHA * raw + (1 - POT_ALPHA) * potEMA;
  float norm = potEMA / 1023.0;

  // Determine selected direction
  int selectedDir;
  if (norm < 0.25) selectedDir = DIR_LEFT;
  else if (norm < 0.5) selectedDir = DIR_UP;
  else if (norm < 0.75) selectedDir = DIR_RIGHT;
  else selectedDir = DIR_DOWN;

  // On LEFT button press, apply selected direction
  if (digitalRead(LEFT_BTN) == HIGH && (millis() - lastLeftPress) > DEBOUNCE_MS) {
    lastLeftPress = millis();

    // Prevent reversing direction directly
    if (!((direction == DIR_LEFT && selectedDir == DIR_RIGHT) ||
          (direction == DIR_RIGHT && selectedDir == DIR_LEFT) ||
          (direction == DIR_UP && selectedDir == DIR_DOWN) ||
          (direction == DIR_DOWN && selectedDir == DIR_UP))) {
      direction = selectedDir;
    }
  }
}

// ---------------------------------------------------------------------------

void beep(int count) {
  // Generate 'count' short beeps and then stop
  for (int i = 0; i < count; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }
}
