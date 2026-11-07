/*
 * Space Invaders for "TheThing" (Maker's Asylum) Arduino Nano board
 *
 * Hardware, from TheThing.kicad_sch rev v8 + add_on_board rev v5:
 *   OLED1   SH1106 I2C    SDA=A4, SCL=A5   (addr 0x3C)
 *   RV1     Potentiometer A0                 -> move ship left/right
 *   SW2     BUTTON2       D2  (10k pull-down, pressed = HIGH) -> fire
 *   SW3     BUTTON3       D4  (10k pull-down, pressed = HIGH) -> fire
 *   BZ1     Buzzer        D3  (via BC547)    -> sound effects
 *   J50     Haptic motor  D5                 -> rumble when you're hit
 *
 * Libraries (Library Manager): "Adafruit SH110X" and "Adafruit GFX Library".
 * Board: Arduino Nano (ATmega328P; pick "Old Bootloader" if upload fails).
 */
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// ---------- Pins ----------
const uint8_t PIN_POT    = A0;
const uint8_t PIN_FIRE1  = 2;
const uint8_t PIN_FIRE2  = 4;
const uint8_t PIN_BUZZER = 3;
const uint8_t PIN_HAPTIC = 5;

// ---------- Display ----------
// The schematic says SSD1306 64x64, but this kit ships a 128x64 SH1106
// (1.3" module). If yours really is narrower, change SCREEN_W.
const int16_t SCREEN_W = 128;
const int16_t SCREEN_H = 64;
const uint8_t OLED_ADDR = 0x3C;   // try 0x3D if the screen stays blank
Adafruit_SH1106G display(SCREEN_W, SCREEN_H, &Wire, -1);

// ---------- Sprites (8x6) ----------
const uint8_t PROGMEM invaderA[2][6] = {
  {0x18, 0x3C, 0x7E, 0xDB, 0xFF, 0x24},
  {0x18, 0x3C, 0x7E, 0xDB, 0xFF, 0x5A}};
const uint8_t PROGMEM invaderB[2][6] = {
  {0x24, 0x7E, 0xDB, 0xFF, 0xA5, 0x24},
  {0x24, 0xBD, 0xDB, 0xFF, 0x7E, 0x42}};
const uint8_t PROGMEM shipBmp[5] = {0x18, 0x3C, 0x7E, 0xFF, 0xFF};  // 8x5

// ---------- Game constants ----------
const uint8_t ROWS = 3, COLS = 7;
const uint8_t INV_W = 8, INV_H = 6, INV_GAP_X = 4, INV_GAP_Y = 3;
const uint8_t SHIP_W = 8, SHIP_H = 5;
const int16_t SHIP_Y = SCREEN_H - SHIP_H - 1;
const uint8_t MAX_ENEMY_SHOTS = 3;
const uint8_t HUD_H = 8;

// ---------- Game state ----------
bool    alive[ROWS][COLS];
uint8_t aliveCount;
int16_t gridX, gridY;
int8_t  gridDir = 1;
uint8_t animFrame = 0;
unsigned long lastStep = 0;
uint16_t stepInterval;

int16_t shipX;
bool    shotActive = false;
int16_t shotX, shotY;
bool    lastFireState = false;

struct Shot { bool active; int16_t x, y; };
Shot enemyShots[MAX_ENEMY_SHOTS];

uint16_t score = 0;
uint16_t hiScore = 0;
uint8_t  lives = 3;
uint8_t  level = 1;
unsigned long hapticOff = 0;
unsigned long invulnUntil = 0;

enum State { TITLE, PLAYING, GAME_OVER };
State state = TITLE;

// ---------- Helpers ----------
bool firePressed() {
  return digitalRead(PIN_FIRE1) == HIGH || digitalRead(PIN_FIRE2) == HIGH;
}

// Rising-edge fire so holding the button doesn't machine-gun
bool fireEdge() {
  bool now = firePressed();
  bool edge = now && !lastFireState;
  lastFireState = now;
  return edge;
}

void beep(unsigned int freq, unsigned long ms) { tone(PIN_BUZZER, freq, ms); }

void rumble(unsigned long ms) {
  digitalWrite(PIN_HAPTIC, HIGH);
  hapticOff = millis() + ms;
}

int16_t readShipX() {
  static int smooth = -1;
  int raw = analogRead(PIN_POT);
  smooth = (smooth < 0) ? raw : (smooth * 3 + raw) / 4;  // light smoothing
  return map(smooth, 0, 1023, 0, SCREEN_W - SHIP_W);
}

int16_t invX(uint8_t c) { return gridX + c * (INV_W + INV_GAP_X); }
int16_t invY(uint8_t r) { return gridY + r * (INV_H + INV_GAP_Y); }

void setupWave() {
  for (uint8_t r = 0; r < ROWS; r++)
    for (uint8_t c = 0; c < COLS; c++) alive[r][c] = true;
  aliveCount = ROWS * COLS;
  gridX = 4;
  gridY = HUD_H + 2 + min(level - 1, 3) * 3;   // each wave starts a bit lower
  gridDir = 1;
  shotActive = false;
  for (auto &s : enemyShots) s.active = false;
  stepInterval = max(120, 500 - (level - 1) * 60);
}

void newGame() {
  score = 0;
  lives = 3;
  level = 1;
  setupWave();
  state = PLAYING;
}

// ---------- Update ----------
void stepInvaders() {
  // Find horizontal extent of live invaders
  int8_t minC = COLS, maxC = -1;
  for (uint8_t c = 0; c < COLS; c++)
    for (uint8_t r = 0; r < ROWS; r++)
      if (alive[r][c]) { if (c < minC) minC = c; if (c > maxC) maxC = c; }

  int16_t left  = invX(minC) + gridDir * 2;
  int16_t right = invX(maxC) + INV_W + gridDir * 2;
  if (left < 0 || right > SCREEN_W) {
    gridDir = -gridDir;
    gridY += 3;
  } else {
    gridX += gridDir * 2;
  }
  animFrame ^= 1;
  beep(animFrame ? 110 : 90, 30);

  // Speed up as invaders are destroyed
  uint16_t base = max(120, 500 - (level - 1) * 60);
  stepInterval = max(40, (int)(base * aliveCount / (ROWS * COLS)));

  // Invaders reached the ship row -> game over
  for (int8_t r = ROWS - 1; r >= 0; r--)
    for (uint8_t c = 0; c < COLS; c++)
      if (alive[r][c] && invY(r) + INV_H >= SHIP_Y) { lives = 0; return; }
}

void enemyFire() {
  // Chance per frame grows with level
  if (random(100) >= 3 + level) return;
  for (auto &s : enemyShots) {
    if (s.active) continue;
    uint8_t c = random(COLS);
    for (int8_t r = ROWS - 1; r >= 0; r--) {   // lowest live invader fires
      if (alive[r][c]) {
        s.active = true;
        s.x = invX(c) + INV_W / 2;
        s.y = invY(r) + INV_H;
        return;
      }
    }
    return;
  }
}

void updateGame() {
  unsigned long now = millis();
  shipX = readShipX();

  if (fireEdge() && !shotActive) {
    shotActive = true;
    shotX = shipX + SHIP_W / 2;
    shotY = SHIP_Y - 1;
    beep(880, 40);
  }

  // Player shot
  if (shotActive) {
    shotY -= 3;
    if (shotY < HUD_H) shotActive = false;
    for (uint8_t r = 0; r < ROWS && shotActive; r++)
      for (uint8_t c = 0; c < COLS && shotActive; c++) {
        if (!alive[r][c]) continue;
        int16_t x = invX(c), y = invY(r);
        if (shotX >= x && shotX < x + INV_W && shotY >= y && shotY < y + INV_H) {
          alive[r][c] = false;
          aliveCount--;
          shotActive = false;
          score += (ROWS - r) * 10;
          beep(220, 60);
        }
      }
  }

  // Invader march
  if (aliveCount > 0 && now - lastStep >= stepInterval) {
    lastStep = now;
    stepInvaders();
  }

  enemyFire();

  // Enemy shots
  for (auto &s : enemyShots) {
    if (!s.active) continue;
    s.y += 2;
    if (s.y >= SCREEN_H) { s.active = false; continue; }
    if (now >= invulnUntil && s.y >= SHIP_Y && s.y < SHIP_Y + SHIP_H &&
        s.x >= shipX && s.x < shipX + SHIP_W) {
      s.active = false;
      lives--;
      invulnUntil = now + 1500;
      beep(60, 300);
      rumble(250);
    }
  }

  if (lives == 0) {
    if (score > hiScore) hiScore = score;
    state = GAME_OVER;
    beep(50, 600);
    rumble(500);
    delay(300);
    lastFireState = true;  // require a fresh press to restart
    return;
  }

  if (aliveCount == 0) {
    level++;
    beep(1320, 150);
    delay(400);
    setupWave();
  }
}

// ---------- Draw ----------
void drawHUD() {
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("S:"));
  display.print(score);
  display.setCursor(SCREEN_W / 2 - 10, 0);
  display.print(F("L"));
  display.print(level);
  for (uint8_t i = 0; i < lives; i++)
    display.fillRect(SCREEN_W - 6 - i * 6, 2, 4, 4, SH110X_WHITE);
  display.drawFastHLine(0, HUD_H, SCREEN_W, SH110X_WHITE);
}

void drawGame() {
  display.clearDisplay();
  drawHUD();

  for (uint8_t r = 0; r < ROWS; r++)
    for (uint8_t c = 0; c < COLS; c++)
      if (alive[r][c])
        display.drawBitmap(invX(c), invY(r),
                           (r == 0 ? invaderB : invaderA)[animFrame],
                           INV_W, INV_H, SH110X_WHITE);

  // Ship blinks while invulnerable
  if (millis() >= invulnUntil || (millis() / 100) % 2)
    display.drawBitmap(shipX, SHIP_Y, shipBmp, SHIP_W, SHIP_H, SH110X_WHITE);

  if (shotActive) display.drawFastVLine(shotX, shotY, 3, SH110X_WHITE);
  for (auto &s : enemyShots)
    if (s.active) display.drawFastVLine(s.x, s.y, 2, SH110X_WHITE);

  display.display();
}

void drawCentered(const __FlashStringHelper *txt, int16_t y, uint8_t size) {
  int16_t x1, y1; uint16_t w, h;
  display.setTextSize(size);
  display.getTextBounds(txt, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_W - w) / 2, y);
  display.print(txt);
}

void drawTitle() {
  display.clearDisplay();
  drawCentered(F("SPACE"), 4, 2);
  drawCentered(F("INVADERS"), 22, 2);
  display.drawBitmap(SCREEN_W / 2 - 4, 42, invaderA[(millis() / 400) % 2],
                     INV_W, INV_H, SH110X_WHITE);
  drawCentered(F("Press fire"), 54, 1);
  display.display();
}

void drawGameOver() {
  display.clearDisplay();
  drawCentered(F("GAME OVER"), 6, 2);
  display.setTextSize(1);
  display.setCursor(20, 30);
  display.print(F("Score: ")); display.print(score);
  display.setCursor(20, 40);
  display.print(F("Best:  ")); display.print(hiScore);
  drawCentered(F("Press fire"), 54, 1);
  display.display();
}

// ---------- Arduino ----------
void setup() {
  pinMode(PIN_FIRE1, INPUT);   // external 10k pull-downs on the add-on board
  pinMode(PIN_FIRE2, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_HAPTIC, OUTPUT);
  digitalWrite(PIN_HAPTIC, LOW);

  if (!display.begin(OLED_ADDR, true)) {
    // No display: beep an error forever
    for (;;) { beep(200, 100); delay(1000); }
  }
  display.setTextColor(SH110X_WHITE);
  randomSeed(analogRead(A1) ^ analogRead(A6));   // LDR + floating pin
}

void loop() {
  unsigned long frameStart = millis();

  if (hapticOff && millis() >= hapticOff) {
    digitalWrite(PIN_HAPTIC, LOW);
    hapticOff = 0;
  }

  switch (state) {
    case TITLE:
      drawTitle();
      if (fireEdge()) newGame();
      break;
    case PLAYING:
      updateGame();
      if (state == PLAYING) drawGame();
      break;
    case GAME_OVER:
      drawGameOver();
      if (fireEdge()) newGame();
      break;
  }

  // ~30 fps
  unsigned long elapsed = millis() - frameStart;
  if (elapsed < 33) delay(33 - elapsed);
}
