/*
================================================================================
  MINI ARCADE - SNAKE GAME (Ultrasonic Control Version)
  ------------------------------------------------------
  Hardware: Arduino Nano + SH1106 128x64 OLED (I2C) + Ultrasonic sensor +
            LEFT button (D4) + Buzzer (D3 via NPN transistor)

  HOW TO PLAY:
    - Place your hand in front of the ultrasonic sensor.
    - Hand distance decides the snake’s direction:
        * Very close  (3–6 cm)   → LEFT
        * Medium-close (7–10 cm) → UP
        * Medium-far   (11–14 cm) → RIGHT
        * Far (15–18 cm)          → DOWN
    - Press LEFT button to start or restart the game.
    - When the snake hits a wall or itself → GAME OVER (3 short beeps).

  SMOOTHING:
    - Ultrasonic readings are filtered with a moving average of the last 5 samples
      to provide steady directional control.

  HARDWARE CONNECTIONS:
    * OLED (SH1106 128x64): I2C -> SDA, SCL
    * Ultrasonic Trigger: D7
    * Ultrasonic Echo: D6
    * LEFT Button (with pulldown): D4
    * Buzzer (via NPN transistor): D3 (base resistor recommended)
================================================================================
*/

#include <U8glib.h>

#define TRIG_PIN 7
#define ECHO_PIN 6
#define LEFT_BTN 4
#define BUZZER_PIN 3

// OLED setup
U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);

// === Snake game constants ===
#define GRID_SIZE 4
#define GRID_WIDTH  (128 / GRID_SIZE)
#define GRID_HEIGHT (64 / GRID_SIZE)
#define MAX_SNAKE_LENGTH 100

// === Directions ===
#define DIR_UP    0
#define DIR_RIGHT 1
#define DIR_DOWN  2
#define DIR_LEFT  3

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

// === Ultrasonic smoothing ===
#define FILTER_SIZE 5
int distHistory[FILTER_SIZE];
int distIndex = 0;

// === Function prototypes ===
void startGame();
void updateGame();
void drawGame();
void moveSnake();
void generateFood();
void beep(int count);
void checkCollision();
void readInput();
int getSmoothedDistance();

// ---------------------------------------------------------------------------

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LEFT_BTN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // Initialize filter
  for (int i = 0; i < FILTER_SIZE; i++) distHistory[i] = 10;

  startGame();
}

// ---------------------------------------------------------------------------

void loop() {
  readInput();

  if (!gameOver) {
    moveSnake();
    checkCollision();
    updateGame();
  } else {
    // Wait for LEFT button to restart
    if (digitalRead(LEFT_BTN) == HIGH && (millis() - lastLeftPress) > DEBOUNCE_MS) {
      lastLeftPress = millis();
      startGame();
    }
  }

  delay(150);
}

// ---------------------------------------------------------------------------

void startGame() {
  snakeLength = 3;
  direction = DIR_RIGHT;
  gameOver = false;

  // Initialize snake in center
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
  u8g.firstPage();
  do {
    drawGame();
  } while (u8g.nextPage());
}

// ---------------------------------------------------------------------------

void drawGame() {
  // Draw play border
  u8g.drawFrame(0, 0, 128, 64);

  // Draw food
  u8g.drawBox(foodX * GRID_SIZE, foodY * GRID_SIZE, GRID_SIZE, GRID_SIZE);

  // Draw snake
  for (int i = 0; i < snakeLength; i++) {
    u8g.drawBox(snakeX[i] * GRID_SIZE, snakeY[i] * GRID_SIZE, GRID_SIZE, GRID_SIZE);
  }

  // If game over
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

  // Check for food
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
  int distance = getSmoothedDistance();

  // Map distance to direction ranges
  int newDir = direction;
  if (distance >= 3 && distance <= 6) newDir = DIR_LEFT;
  else if (distance >= 7 && distance <= 10) newDir = DIR_UP;
  else if (distance >= 11 && distance <= 14) newDir = DIR_RIGHT;
  else if (distance >= 15 && distance <= 18) newDir = DIR_DOWN;

  // Avoid direct 180° reversal
  if (!((direction == DIR_LEFT && newDir == DIR_RIGHT) ||
        (direction == DIR_RIGHT && newDir == DIR_LEFT) ||
        (direction == DIR_UP && newDir == DIR_DOWN) ||
        (direction == DIR_DOWN && newDir == DIR_UP))) {
    direction = newDir;
  }
}

// ---------------------------------------------------------------------------

int getSmoothedDistance() {
  // Trigger ultrasonic pulse
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Read echo
  long duration = pulseIn(ECHO_PIN, HIGH, 20000); // timeout ~20ms
  int distance = duration * 0.034 / 2; // in cm
  if (distance == 0) distance = 20; // handle out of range

  // Add to circular buffer
  distHistory[distIndex] = distance;
  distIndex = (distIndex + 1) % FILTER_SIZE;

  // Compute average
  long sum = 0;
  for (int i = 0; i < FILTER_SIZE; i++) sum += distHistory[i];
  return sum / FILTER_SIZE;
}

// ---------------------------------------------------------------------------

void beep(int count) {
  for (int i = 0; i < count; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }
}
