/*
 * PIXEL HOP - a side-scrolling platformer for "TheThing" Arduino Nano board
 *
 * Hardware (TheThing rev v8 + add_on_board rev v5):
 *   OLED    SH1106 128x64 I2C  SDA=A4, SCL=A5 (addr 0x3C)
 *   RV1     Potentiometer A0   -> run left / right (centre = stand still)
 *   SW2/SW3 Buttons D2 / D4    -> jump (hold for a higher jump)
 *   BZ1     Buzzer D3          -> sound effects
 *   J50     Haptic motor D5    -> rumble when you lose a life
 *
 * Stomp the beetles, grab coins (20 coins = extra life), and reach the flag.
 * Every level is generated from its number and gets harder as you go.
 *
 * Libraries: "Adafruit SH110X" and "Adafruit GFX Library".
 */
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// ---------- Pins ----------
const uint8_t PIN_POT = A0, PIN_JUMP1 = 2, PIN_JUMP2 = 4, PIN_BUZZER = 3, PIN_HAPTIC = 5;

// ---------- Display ----------
const int16_t SCREEN_W = 128, SCREEN_H = 64;
const uint8_t OLED_ADDR = 0x3C;
Adafruit_SH1106G display(SCREEN_W, SCREEN_H, &Wire, -1);
#define WHITE SH110X_WHITE
#define BLACK SH110X_BLACK

// ---------- Sprites ----------
// Player 6x8 (drawn from an 8-wide bitmap). [facing][frame]
const uint8_t PROGMEM playerBmp[2][2][8] = {
  { {0x78, 0xFC, 0x68, 0x78, 0xFC, 0x78, 0x48, 0xCC},     // right, frame 0
    {0x78, 0xFC, 0x68, 0x78, 0xFC, 0x78, 0x88, 0x84} },   // right, frame 1
  { {0x78, 0xFC, 0x58, 0x78, 0xFC, 0x78, 0x48, 0xCC},     // left, frame 0
    {0x78, 0xFC, 0x58, 0x78, 0xFC, 0x78, 0x44, 0x84} } }; // left, frame 1
// Beetle enemy 8x6 [frame]
const uint8_t PROGMEM beetleBmp[2][6] = {
  {0x3C, 0x7E, 0xDB, 0xFF, 0x7E, 0xA5},
  {0x3C, 0x7E, 0xDB, 0xFF, 0x7E, 0x5A}};

// ---------- Level ----------
// One byte per 8px column: bits0-1 ground height in tiles (0 = pit),
// bits2-4 floating block row (0 = none), bit5 = coin present.
const uint8_t LEN = 160;
const uint8_t GOAL = LEN - 6;
uint8_t lvl[LEN];

const uint8_t MAX_ENEMIES = 8;
struct Enemy { int16_t x; uint8_t y; uint8_t g; int8_t dir; bool alive; };
Enemy enemies[MAX_ENEMIES];
uint8_t enemyCount;

inline uint8_t groundH(int16_t c) { return lvl[c] & 3; }
inline uint8_t blockRow(int16_t c) { return (lvl[c] >> 2) & 7; }
inline bool hasCoin(int16_t c) { return lvl[c] & 0x20; }
inline uint8_t coinRow(int16_t c) {
  return blockRow(c) ? blockRow(c) - 1 : 8 - groundH(c) - 2;
}

bool solid(int16_t tx, int16_t ty) {
  if (tx < 0 || tx >= LEN) return true;
  if (ty < 1 || ty > 7) return false;   // HUD row and below screen are open
  uint8_t g = groundH(tx);
  if (g && ty >= 8 - g) return true;
  uint8_t b = blockRow(tx);
  return b && ty == b;
}

// ---------- Game state ----------
// Player position/velocity in 1/16 pixel units
int16_t px, py, vx, vy;
bool onGround, facingLeft;
uint8_t walkAnim;
int16_t camX;
uint16_t score = 0, hiScore = 0;
uint8_t coins = 0, lives = 3, level = 1;
// Jump input: the buttons are sampled continuously (also while waiting for
// the next frame) so even very quick taps register.
bool jumpEdge = false;      // a press started since the last frame
bool jumpSample = false;    // last sampled button state
uint8_t jumpBuffer = 0;     // frames a press stays "remembered" before landing
uint8_t coyote = 0;         // frames you can still jump after leaving a ledge
unsigned long hapticOff = 0;
uint8_t frameCount = 0;

// Deterministic RNG so each level number always builds the same map
uint16_t rngState;
uint8_t rnd(uint8_t n) {
  rngState = rngState * 25173u + 13849u;
  return (rngState >> 8) % n;
}

// ---------- Helpers ----------
bool jumpHeld() { return digitalRead(PIN_JUMP1) == HIGH || digitalRead(PIN_JUMP2) == HIGH; }
void beep(unsigned int f, unsigned long ms) { tone(PIN_BUZZER, f, ms); }
void rumble(unsigned long ms) { digitalWrite(PIN_HAPTIC, HIGH); hapticOff = millis() + ms; }

void playNotes(const uint16_t *notes, uint8_t n, uint16_t ms) {
  for (uint8_t i = 0; i < n; i++) { tone(PIN_BUZZER, notes[i], ms); delay(ms + 15); }
}

// ---------- Level generation ----------
void setCol(uint8_t &c, uint8_t g, uint8_t blk = 0, bool coin = false) {
  if (c >= LEN) return;
  lvl[c++] = (g & 3) | ((blk & 7) << 2) | (coin ? 0x20 : 0);
}

void addEnemy(uint8_t col, uint8_t g) {
  if (enemyCount >= MAX_ENEMIES || g == 0) return;
  Enemy &e = enemies[enemyCount++];
  e.x = col * 8 * 16;
  e.y = (8 - g) * 8 - 6;
  e.g = g;
  e.dir = -1;
  e.alive = true;
}

void genLevel() {
  rngState = 0x1234 + level * 7919u;
  enemyCount = 0;
  uint8_t c = 0, g = 2;
  while (c < 8) setCol(c, g);

  const uint8_t endZone = LEN - 12;
  uint8_t maxPit = min(3, 1 + level / 2);
  while (c < endZone) {
    uint8_t r = rnd(10);
    if (r < 3) {                       // flat run, maybe with coins / beetle
      uint8_t n = 3 + rnd(4);
      bool coinRun = rnd(3) == 0;
      uint8_t start = c;
      for (uint8_t i = 0; i < n && c < endZone; i++) setCol(c, g, 0, coinRun);
      if (rnd(4) < 1 + level / 2) addEnemy(start + n / 2, g);
    } else if (r < 5) {                // pit
      uint8_t w = 1 + rnd(maxPit);
      for (uint8_t i = 0; i < w && c < endZone; i++) setCol(c, 0);
      for (uint8_t i = 0; i < 2 && c < endZone; i++) setCol(c, g);  // landing
    } else if (r < 7) {                // step up / down
      g = constrain(g + (rnd(2) ? 1 : -1), 1, 3);
      uint8_t n = 2 + rnd(2);
      for (uint8_t i = 0; i < n && c < endZone; i++) setCol(c, g);
    } else {                           // floating blocks with coins on top
      uint8_t n = 3 + rnd(3);
      uint8_t blk = 8 - g - 2;
      setCol(c, g);
      for (uint8_t i = 0; i < n && c < endZone; i++) setCol(c, g, blk, true);
      setCol(c, g);
      if (rnd(3) == 0) addEnemy(c - 2, g);
    }
  }
  while (c < LEN) setCol(c, g);

  px = 16 * 16;
  py = ((8 - groundH(2)) * 8 - 8) * 16;
  vx = vy = 0;
  camX = 0;
  onGround = false;
}

// ---------- Player ----------
void loseLife();

bool boxHits(int16_t x, int16_t y, int16_t w, int16_t h) {
  for (int16_t tx = x >> 3; tx <= (x + w - 1) >> 3; tx++)
    for (int16_t ty = y >> 3; ty <= (y + h - 1) >> 3; ty++)
      if (solid(tx, ty)) return true;
  return false;
}

void updatePlayer() {
  // Potentiometer: centre is a dead zone, further from centre = faster
  int d = analogRead(PIN_POT) - 512;
  int16_t target = 0;
  if (d > 60)       target =  min(24, (d - 60) / 14 + 6);
  else if (d < -60) target = -min(24, (-d - 60) / 14 + 6);
  if (vx < target) vx = min(target, vx + 2);
  else if (vx > target) vx = max(target, vx - 2);
  if (vx > 0) facingLeft = false; else if (vx < 0) facingLeft = true;

  if (jumpEdge) { jumpBuffer = 6; jumpEdge = false; }
  if (jumpBuffer && (onGround || coyote)) {
    vy = -54;
    jumpBuffer = 0;
    coyote = 0;
    beep(660, 40);
  }
  if (jumpBuffer) jumpBuffer--;
  if (!jumpHeld() && vy < -32) vy = -32;   // release early = short hop

  vy = min(64, vy + 4);                // gravity

  // Horizontal move + collision
  px += vx;
  if (px < camX * 16) { px = camX * 16; vx = 0; }
  int16_t x = px >> 4, y = py >> 4;
  if (boxHits(x, y, 6, 8)) {
    if (vx > 0) px = ((((x + 5) >> 3) * 8) - 6) * 16;
    else        px = (((x >> 3) + 1) * 8) * 16;
    vx = 0;
  }

  // Vertical move + collision
  py += vy;
  onGround = false;
  x = px >> 4; y = py >> 4;
  if (y < 8) { py = 8 * 16; vy = 0; y = 8; }
  if (boxHits(x, y, 6, 8)) {
    if (vy > 0) { py = ((((y + 7) >> 3) * 8) - 8) * 16; onGround = true; }
    else        { py = (((y >> 3) + 1) * 8) * 16; beep(150, 20); }
    vy = 0;
  }
  x = px >> 4; y = py >> 4;
  // Standing check: solid ground directly under the feet counts as grounded
  // every frame (gravity's sub-pixel creep otherwise hides it 2 frames in 3).
  if (vy >= 0 && boxHits(x, y + 1, 6, 8)) {
    onGround = true;
    vy = 0;
    py = y * 16;
  }
  if (onGround) coyote = 4; else if (coyote) coyote--;

  if (vx != 0 && onGround && (frameCount & 3) == 0) walkAnim ^= 1;

  // Coins
  for (int16_t tx = x >> 3; tx <= (x + 5) >> 3; tx++) {
    if (tx < 0 || tx >= LEN || !hasCoin(tx)) continue;
    int16_t cy = coinRow(tx) * 8;
    if (y < cy + 7 && y + 8 > cy + 1) {
      lvl[tx] &= ~0x20;
      score += 10;
      beep(1568, 50);
      if (++coins >= 20) { coins = 0; lives++; beep(2093, 120); }
    }
  }

  // Camera follows, never scrolls back
  int16_t want = x - 48;
  if (want > camX) camX = min(want, (int16_t)(LEN * 8 - SCREEN_W));

  if (y > SCREEN_H) loseLife();
}

// ---------- Enemies ----------
void updateEnemies() {
  int16_t x = px >> 4, y = py >> 4;
  for (uint8_t i = 0; i < enemyCount; i++) {
    Enemy &e = enemies[i];
    if (!e.alive) continue;
    int16_t ex = e.x >> 4;
    if (ex < camX - 16 || ex > camX + SCREEN_W + 16) continue;  // sleep off-screen

    // Walk, turning at walls, pits and ledges
    int16_t nx = e.x + e.dir * 8;
    int16_t front = (nx >> 4) + (e.dir > 0 ? 7 : 0);
    int16_t col = front >> 3;
    if (col < 0 || col >= LEN || groundH(col) != e.g) e.dir = -e.dir;
    else e.x = nx;
    ex = e.x >> 4;

    // Player collision
    if (x < ex + 8 && x + 6 > ex && y < e.y + 6 && y + 8 > e.y) {
      if (vy > 0 && y + 8 <= e.y + 4) {       // stomp
        e.alive = false;
        vy = jumpHeld() ? -50 : -32;
        score += 50;
        beep(400, 60);
      } else {
        loseLife();
        return;
      }
    }
  }
}

// ---------- Drawing ----------
void drawGround(int16_t x, int16_t y, uint8_t ty) {
  display.fillRect(x, y, 8, 8, WHITE);
  display.drawFastHLine(x, y + 4, 8, BLACK);
  display.drawFastVLine(x + ((ty & 1) ? 2 : 6), y, 4, BLACK);
  display.drawFastVLine(x + ((ty & 1) ? 6 : 2), y + 5, 3, BLACK);
}

void drawBlock(int16_t x, int16_t y) {
  display.drawRect(x, y, 8, 8, WHITE);
  display.fillRect(x + 3, y + 3, 2, 2, WHITE);
}

void drawHUD() {
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(score);
  display.setCursor(44, 0);
  display.print(F("o"));
  display.print(coins);
  display.setCursor(72, 0);
  display.print(F("W"));
  display.print(level);
  display.setCursor(104, 0);
  display.print(F("x"));
  display.print(lives);
}

void drawWorld() {
  display.clearDisplay();
  drawHUD();

  int16_t first = camX >> 3, off = camX & 7;
  for (int16_t i = 0; i <= SCREEN_W / 8; i++) {
    int16_t col = first + i;
    if (col >= LEN) break;
    int16_t sx = i * 8 - off;
    uint8_t g = groundH(col);
    for (uint8_t ty = 8 - g; ty < 8 && g; ty++) drawGround(sx, ty * 8, ty);
    if (blockRow(col)) drawBlock(sx, blockRow(col) * 8);
    if (hasCoin(col)) {
      int16_t cy = coinRow(col) * 8;
      display.drawCircle(sx + 4, cy + 4, 2, WHITE);
      display.drawPixel(sx + 4, cy + 4, WHITE);
    }
    if (col == GOAL) {
      int16_t top = 12, base = (8 - g) * 8;
      display.drawFastVLine(sx + 1, top, base - top, WHITE);
      display.fillTriangle(sx + 2, top, sx + 9, top + 3, sx + 2, top + 6, WHITE);
    }
  }

  for (uint8_t i = 0; i < enemyCount; i++) {
    Enemy &e = enemies[i];
    int16_t ex = (e.x >> 4) - camX;
    if (e.alive && ex > -8 && ex < SCREEN_W)
      display.drawBitmap(ex, e.y, beetleBmp[(frameCount >> 3) & 1], 8, 6, WHITE);
  }

  display.drawBitmap((px >> 4) - camX, py >> 4,
                     playerBmp[facingLeft][onGround ? walkAnim : 1], 8, 8, WHITE);
  display.display();
}

void centerText(const __FlashStringHelper *t, int16_t y, uint8_t size) {
  int16_t x1, y1; uint16_t w, h;
  display.setTextSize(size);
  display.getTextBounds(t, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_W - w) / 2, y);
  display.print(t);
}

void showLevelCard() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(28, 14);
  display.print(F("WORLD "));
  display.print(level);
  display.drawBitmap(50, 40, playerBmp[0][0], 8, 8, WHITE);
  display.setTextSize(1);
  display.setCursor(62, 40);
  display.print(F("x "));
  display.print(lives);
  display.display();
  delay(1500);
}

// ---------- Flow ----------
enum State { TITLE, PLAYING, GAME_OVER };
State state = TITLE;

void startLevel() {
  genLevel();
  showLevelCard();
  jumpEdge = false;  // ignore presses made during the level card
  jumpBuffer = 0;
  jumpSample = jumpHeld();
}

void loseLife() {
  static const uint16_t dieTune[] = {494, 392, 330, 262, 196};
  rumble(400);
  drawWorld();
  playNotes(dieTune, 5, 90);
  digitalWrite(PIN_HAPTIC, LOW);
  hapticOff = 0;
  if (--lives == 0) {
    if (score > hiScore) hiScore = score;
    state = GAME_OVER;
    jumpEdge = false;
    jumpSample = jumpHeld();
    return;
  }
  startLevel();
}

void levelClear() {
  static const uint16_t winTune[] = {523, 659, 784, 1047, 784, 1047};
  score += 100 + level * 20;
  playNotes(winTune, 6, 100);
  level++;
  startLevel();
}

void setup() {
  pinMode(PIN_JUMP1, INPUT);      // external 10k pull-downs on the add-on board
  pinMode(PIN_JUMP2, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_HAPTIC, OUTPUT);
  digitalWrite(PIN_HAPTIC, LOW);

  if (!display.begin(OLED_ADDR, true)) {
    for (;;) { beep(200, 100); delay(1000); }
  }
  Wire.setClock(400000);          // faster I2C = smoother frame rate
  display.setTextColor(WHITE);
}

void sampleJump() {
  bool now = jumpHeld();
  if (now && !jumpSample) jumpEdge = true;
  jumpSample = now;
}

bool jumpPressed() {
  bool e = jumpEdge;
  jumpEdge = false;
  return e;
}

void loop() {
  unsigned long t0 = millis();
  frameCount++;
  sampleJump();
  if (hapticOff && millis() >= hapticOff) { digitalWrite(PIN_HAPTIC, LOW); hapticOff = 0; }

  switch (state) {
    case TITLE:
      display.clearDisplay();
      centerText(F("PIXEL HOP"), 8, 2);
      display.drawBitmap(40, 32, playerBmp[0][(frameCount >> 3) & 1], 8, 8, WHITE);
      display.drawBitmap(80, 34, beetleBmp[(frameCount >> 3) & 1], 8, 6, WHITE);
      display.drawFastHLine(0, 40, SCREEN_W, WHITE);
      centerText(F("Press jump"), 52, 1);
      display.display();
      if (jumpPressed()) {
        score = 0; coins = 0; lives = 3; level = 1;
        state = PLAYING;
        startLevel();
      }
      break;

    case PLAYING:
      updatePlayer();
      if (state == PLAYING) updateEnemies();
      if (state == PLAYING && (px >> 4) + 3 >= GOAL * 8) levelClear();
      if (state == PLAYING) drawWorld();
      break;

    case GAME_OVER:
      display.clearDisplay();
      centerText(F("GAME OVER"), 6, 2);
      display.setTextSize(1);
      display.setCursor(24, 30); display.print(F("Score: ")); display.print(score);
      display.setCursor(24, 40); display.print(F("Best:  ")); display.print(hiScore);
      centerText(F("Press jump"), 54, 1);
      display.display();
      if (jumpPressed()) state = TITLE;
      break;
  }

  // Wait out the rest of the frame while watching the buttons
  do { sampleJump(); } while (millis() - t0 < 33);
}
