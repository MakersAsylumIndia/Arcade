/*
 * CRYPT BLASTER - an original first-person raycasting shooter
 * for the "TheThing" Arduino Nano board.
 *
 *   Potentiometer A0  -> look / turn (the knob is your heading: one sweep = full circle)
 *   BUTTON2 D2 (hold) -> walk forward
 *   BUTTON3 D4        -> fire
 *   Buzzer D3, haptic motor D5, SH1106 128x64 OLED on I2C (0x3C)
 *
 * Hunt down every gloop in the crypt to clear the level. They sniff you
 * out and bite when close; each level brings more and tougher ones.
 *
 * Libraries: "Adafruit SH110X" and "Adafruit GFX Library".
 */
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

const uint8_t PIN_POT = A0, PIN_WALK = 2, PIN_FIRE = 4, PIN_BUZZER = 3, PIN_HAPTIC = 5;

Adafruit_SH1106G display(128, 64, &Wire, -1);
#define WHITE SH110X_WHITE
#define BLACK SH110X_BLACK

const uint8_t VIEW_H = 56;     // 3D view height; HUD below
const uint8_t COLS = 64;       // rays per frame (each drawn 2px wide)

// ---------- Map: 16x16, bit 15 = x0 ----------
const uint16_t PROGMEM worldMap[16] = {
  0b1111111111111111,
  0b1000000010000001,
  0b1011110010111101,
  0b1010000000000101,
  0b1010111011100101,
  0b1000100000100001,
  0b1110101110101111,
  0b1000001010000001,
  0b1011100000111101,
  0b1000000100000001,
  0b1011110101111101,
  0b1000010100000001,
  0b1110010111110111,
  0b1000000000000001,
  0b1011101101011101,
  0b1111111111111111,
};

bool wallAt(int8_t x, int8_t y) {
  if (x < 0 || y < 0 || x > 15 || y > 15) return true;
  return (pgm_read_word(&worldMap[y]) >> (15 - x)) & 1;
}

// ---------- Enemy sprite (8x8): body is white, mask is black backing ----------
const uint8_t PROGMEM gloopBody[2][8] = {
  {0x42, 0x7E, 0xFF, 0xDB, 0xFF, 0xC3, 0x7E, 0x5A},
  {0x42, 0x7E, 0xFF, 0xDB, 0xFF, 0xC3, 0x7E, 0xA5}};
const uint8_t PROGMEM gloopMask[8] = {0x42, 0x7E, 0xFF, 0xFF, 0xFF, 0xFF, 0x7E, 0xFF};

// ---------- State ----------
float plX, plY, plA;
float dirX, dirY, plnX, plnY;
int8_t health;
uint8_t level = 1;
uint16_t score = 0, hiScore = 0;
uint8_t zbuf[COLS];   // wall distance per column, 1/16 tile units

const uint8_t MAX_MOBS = 8;
struct Mob { float x, y; int8_t hp; uint8_t hurt; unsigned long nextBite; };
Mob mobs[MAX_MOBS];
uint8_t mobCount, mobsAlive;
// Explicit prototypes stop the Arduino builder placing them above `Mob`
bool projectMob(const Mob &m, float &screenX, float &depth);
void tryMove(float &x, float &y, float dx, float dy, float r);

unsigned long lastFrame, fireCooldownUntil = 0, flashUntil = 0, hapticOff = 0, hurtFlashUntil = 0;
bool fireSample = false, fireEdge = false;
float bobPhase = 0;
uint8_t frameNo = 0;

enum State { TITLE, PLAYING, CLEARED, DEAD };
State state = TITLE;

// ---------- Helpers ----------
void beep(unsigned int f, unsigned long ms) { tone(PIN_BUZZER, f, ms); }
void rumble(unsigned long ms) { digitalWrite(PIN_HAPTIC, HIGH); hapticOff = millis() + ms; }
bool walkHeld() { return digitalRead(PIN_WALK) == HIGH; }
bool fireHeld() { return digitalRead(PIN_FIRE) == HIGH; }
bool anyHeld() { return walkHeld() || fireHeld(); }

void sampleButtons() {
  bool f = fireHeld();
  if (f && !fireSample) fireEdge = true;
  fireSample = f;
}

uint8_t rngByte() { return random(256); }

// ---------- Level setup ----------
void spawnLevel() {
  plX = 1.5; plY = 1.5;
  mobCount = min((int)MAX_MOBS, 2 + level);
  mobsAlive = mobCount;
  for (uint8_t i = 0; i < mobCount; i++) {
    int8_t x, y;
    do {
      x = 1 + random(14);
      y = 1 + random(14);
    } while (wallAt(x, y) || (x + 0.5 - plX) * (x + 0.5 - plX) + (y + 0.5 - plY) * (y + 0.5 - plY) < 25);
    mobs[i].x = x + 0.5;
    mobs[i].y = y + 0.5;
    mobs[i].hp = 2 + level / 3;
    mobs[i].hurt = 0;
    mobs[i].nextBite = 0;
  }
}

// ---------- Rendering ----------
void drawWallColumn(uint8_t x0, int8_t top, uint8_t h, uint8_t shade) {
  for (uint8_t x = x0; x < x0 + 2; x++) {
    if (shade == 0) { display.drawFastVLine(x, top, h, WHITE); continue; }
    display.drawPixel(x, top, WHITE);
    display.drawPixel(x, top + h - 1, WHITE);
    for (uint8_t y = top + 1; y < top + h - 1; y++) {
      bool on;
      if (shade == 1)      on = ((x + y) & 1) == 0;
      else if (shade == 2) on = ((x + y) & 3) == 0;
      else                 on = false;
      if (on) display.drawPixel(x, y, WHITE);
    }
  }
}

void castWalls() {
  for (uint8_t c = 0; c < COLS; c++) {
    float camX = 2.0f * c / COLS - 1.0f;
    float rdx = dirX + plnX * camX, rdy = dirY + plnY * camX;
    int8_t mx = (int8_t)plX, my = (int8_t)plY;
    float ddx = (rdx == 0) ? 1e6 : fabs(1.0f / rdx);
    float ddy = (rdy == 0) ? 1e6 : fabs(1.0f / rdy);
    int8_t sx, sy;
    float sdx, sdy;
    if (rdx < 0) { sx = -1; sdx = (plX - mx) * ddx; } else { sx = 1; sdx = (mx + 1.0f - plX) * ddx; }
    if (rdy < 0) { sy = -1; sdy = (plY - my) * ddy; } else { sy = 1; sdy = (my + 1.0f - plY) * ddy; }
    uint8_t side = 0;
    for (uint8_t i = 0; i < 40; i++) {
      if (sdx < sdy) { sdx += ddx; mx += sx; side = 0; }
      else           { sdy += ddy; my += sy; side = 1; }
      if (wallAt(mx, my)) break;
    }
    float d = side == 0 ? sdx - ddx : sdy - ddy;
    if (d < 0.05f) d = 0.05f;
    zbuf[c] = (d * 16 > 255) ? 255 : (uint8_t)(d * 16);

    int16_t h = (int16_t)(VIEW_H / d);
    if (h > VIEW_H) h = VIEW_H;
    if (h < 2) h = 2;
    uint8_t shade = side + (d > 4 ? 1 : 0) + (d > 8 ? 1 : 0);
    drawWallColumn(c * 2, (VIEW_H - h) / 2, h, shade > 3 ? 3 : shade);
  }
}

// Project a mob to the screen. Returns false if behind the camera.
bool projectMob(const Mob &m, float &screenX, float &depth) {
  float rx = m.x - plX, ry = m.y - plY;
  float invDet = 1.0f / (plnX * dirY - dirX * plnY);
  float tx = invDet * (dirY * rx - dirX * ry);
  depth = invDet * (-plnY * rx + plnX * ry);
  if (depth < 0.2f) return false;
  screenX = 64.0f * (1.0f + tx / depth);   // pixel x of sprite centre
  return true;
}

void drawMobs() {
  // Sort alive mobs far -> near
  uint8_t order[MAX_MOBS], n = 0;
  float depthOf[MAX_MOBS], sxOf[MAX_MOBS];
  for (uint8_t i = 0; i < mobCount; i++) {
    if (mobs[i].hp <= 0) continue;
    float sx, dp;
    if (!projectMob(mobs[i], sx, dp)) continue;
    depthOf[i] = dp; sxOf[i] = sx;
    uint8_t j = n++;
    while (j > 0 && depthOf[order[j - 1]] < dp) { order[j] = order[j - 1]; j--; }
    order[j] = i;
  }

  for (uint8_t k = 0; k < n; k++) {
    uint8_t i = order[k];
    float dp = depthOf[i];
    int16_t size = (int16_t)(VIEW_H / dp);
    if (size > 90) size = 90;
    if (size < 3) size = 3;
    int16_t h = size * 3 / 4, w = h;
    int16_t floorY = VIEW_H / 2 + size / 2;
    int16_t top = floorY - h;
    int16_t left = (int16_t)sxOf[i] - w / 2;
    uint8_t zd = dp * 16 > 255 ? 255 : (uint8_t)(dp * 16);
    bool flashWhite = mobs[i].hurt > 0;
    const uint8_t *body = gloopBody[(frameNo >> 2) & 1];

    for (int16_t px = left; px < left + w; px++) {
      if (px < 0 || px > 127) continue;
      if (zbuf[px >> 1] < zd) continue;          // hidden behind a wall
      uint8_t u = (px - left) * 8 / w;
      for (int16_t py = top; py < floorY; py++) {
        if (py < 0 || py >= VIEW_H) continue;
        uint8_t v = (py - top) * 8 / h;
        uint8_t bit = 0x80 >> u;
        bool m = pgm_read_byte(&gloopMask[v]) & bit;
        if (!m) continue;
        bool b = flashWhite || (pgm_read_byte(&body[v]) & bit);
        display.drawPixel(px, py, b ? WHITE : BLACK);
      }
    }
  }
}

void drawGun(bool walking) {
  int8_t bob = walking ? (int8_t)(sin(bobPhase) * 2) : 0;
  int8_t gy = 44 + bob;
  if (millis() < flashUntil) {
    display.fillCircle(64, gy - 4, 5, WHITE);
    display.fillCircle(64, gy - 4, 2, BLACK);
  }
  display.fillRect(58, gy + 4, 13, VIEW_H - gy - 4, BLACK);
  display.drawRect(58, gy + 4, 13, VIEW_H - gy - 4, WHITE);
  display.fillRect(61, gy, 7, 5, BLACK);
  display.drawRect(61, gy, 7, 5, WHITE);
  display.drawFastVLine(64, gy - 1, 2, WHITE);
  // Crosshair
  display.drawPixel(64, VIEW_H / 2, WHITE);
  display.drawPixel(62, VIEW_H / 2, WHITE);
  display.drawPixel(66, VIEW_H / 2, WHITE);
}

void drawHUD() {
  display.fillRect(0, VIEW_H, 128, 64 - VIEW_H, BLACK);
  display.drawFastHLine(0, VIEW_H, 128, WHITE);
  display.setTextSize(1);
  display.setCursor(0, VIEW_H + 1);
  display.print(F("HP"));
  display.drawRect(14, VIEW_H + 2, 32, 5, WHITE);
  display.fillRect(15, VIEW_H + 3, max(0, (int)health) * 30 / 100, 3, WHITE);
  display.setCursor(52, VIEW_H + 1);
  display.print(F("G:"));
  display.print(mobsAlive);
  display.setCursor(78, VIEW_H + 1);
  display.print(F("L"));
  display.print(level);
  display.setCursor(98, VIEW_H + 1);
  display.print(score);
}

// ---------- Game logic ----------
void readHeading() {
  static float smooth = -1;
  int raw = analogRead(PIN_POT);
  smooth = (smooth < 0) ? raw : smooth * 0.6f + raw * 0.4f;
  plA = smooth * (2.0f * PI / 1024.0f);
  dirX = cos(plA); dirY = sin(plA);
  plnX = -dirY * 0.66f; plnY = dirX * 0.66f;
}

void tryMove(float &x, float &y, float dx, float dy, float r) {
  float nx = x + dx;
  if (!wallAt((int8_t)(nx + (dx > 0 ? r : -r)), (int8_t)y)) x = nx;
  float ny = y + dy;
  if (!wallAt((int8_t)x, (int8_t)(ny + (dy > 0 ? r : -r)))) y = ny;
}

void shoot() {
  fireCooldownUntil = millis() + 280;
  flashUntil = millis() + 70;
  beep(1200, 30);
  int8_t best = -1;
  float bestDepth = 99;
  for (uint8_t i = 0; i < mobCount; i++) {
    if (mobs[i].hp <= 0) continue;
    float sx, dp;
    if (!projectMob(mobs[i], sx, dp)) continue;
    float halfW = (VIEW_H / dp) * 3 / 8 + 1;
    if (fabs(sx - 64) > halfW) continue;
    if (dp * 16 > zbuf[COLS / 2]) continue;     // wall in the way
    if (dp < bestDepth) { bestDepth = dp; best = i; }
  }
  if (best < 0) return;
  Mob &m = mobs[best];
  m.hp--;
  m.hurt = 3;
  // Knockback
  m.x += dirX * 0.15f;
  m.y += dirY * 0.15f;
  if (wallAt((int8_t)m.x, (int8_t)m.y)) { m.x -= dirX * 0.15f; m.y -= dirY * 0.15f; }
  if (m.hp <= 0) {
    mobsAlive--;
    score += 25 * level;
    beep(90, 180);
  } else {
    beep(500, 40);
  }
}

void updateMobs(float dt) {
  float speed = (0.7f + level * 0.12f) * dt;
  unsigned long now = millis();
  for (uint8_t i = 0; i < mobCount; i++) {
    Mob &m = mobs[i];
    if (m.hp <= 0) continue;
    if (m.hurt) m.hurt--;
    float dx = plX - m.x, dy = plY - m.y;
    float d = sqrt(dx * dx + dy * dy);
    if (d > 9) continue;                    // too far to notice you
    if (d > 0.6f) {
      tryMove(m.x, m.y, dx / d * speed, dy / d * speed, 0.3f);
    } else if (now >= m.nextBite) {
      m.nextBite = now + 900;
      health -= 8 + level * 2;
      beep(110, 120);
      rumble(180);
      hurtFlashUntil = now + 120;
    }
  }
}

void showMessage(const __FlashStringHelper *a, const __FlashStringHelper *b) {
  display.clearDisplay();
  display.setTextSize(2);
  int16_t x1, y1; uint16_t w, h;
  display.getTextBounds(a, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((128 - w) / 2, 10);
  display.print(a);
  display.setTextSize(1);
  display.getTextBounds(b, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((128 - w) / 2, 50);
  display.print(b);
}

void startGame() {
  level = 1;
  score = 0;
  health = 100;
  spawnLevel();
  state = PLAYING;
  lastFrame = millis();
}

// ---------- Arduino ----------
void setup() {
  pinMode(PIN_WALK, INPUT);   // external 10k pull-downs
  pinMode(PIN_FIRE, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_HAPTIC, OUTPUT);
  digitalWrite(PIN_HAPTIC, LOW);
  if (!display.begin(0x3C, true)) { for (;;) { beep(200, 100); delay(1000); } }
  Wire.setClock(400000);
  display.setTextColor(WHITE);
  randomSeed(analogRead(A1) ^ (analogRead(A6) << 4) ^ micros());
}

void loop() {
  sampleButtons();
  unsigned long now = millis();
  if (hapticOff && now >= hapticOff) { digitalWrite(PIN_HAPTIC, LOW); hapticOff = 0; }
  frameNo++;

  switch (state) {
    case TITLE:
    case DEAD:
    case CLEARED: {
      if (state == TITLE) {
        showMessage(F("CRYPT"), F("Press a button"));
        display.setTextSize(2);
        display.setCursor(22, 28);
        display.print(F("BLASTER"));
      } else if (state == DEAD) {
        showMessage(F("YOU DIED"), F("Press a button"));
        display.setTextSize(1);
        display.setCursor(28, 30); display.print(F("Score: ")); display.print(score);
        display.setCursor(28, 40); display.print(F("Best:  ")); display.print(hiScore);
      } else {
        showMessage(F("CLEARED!"), F("Press a button"));
        display.setTextSize(1);
        display.setCursor(34, 34); display.print(F("Next: L")); display.print(level);
      }
      display.display();
      static bool armed = false;          // wait for release, then a fresh press
      if (!anyHeld()) armed = true;
      else if (armed) {
        armed = false;
        if (state == CLEARED) { health = min(100, health + 30); spawnLevel(); state = PLAYING; lastFrame = millis(); }
        else startGame();
        fireEdge = false;
      }
      break;
    }

    case PLAYING: {
      float dt = (now - lastFrame) / 1000.0f;
      if (dt > 0.2f) dt = 0.2f;
      lastFrame = now;

      readHeading();
      bool walking = walkHeld();
      if (walking) {
        float v = 2.4f * dt;
        tryMove(plX, plY, dirX * v, dirY * v, 0.25f);
        bobPhase += dt * 12;
      }

      display.clearDisplay();
      castWalls();
      if (fireEdge || (fireHeld() && now >= fireCooldownUntil)) {
        if (now >= fireCooldownUntil) shoot();
      }
      fireEdge = false;
      updateMobs(dt);
      drawMobs();
      drawGun(walking);
      drawHUD();
      display.invertDisplay(millis() < hurtFlashUntil);
      display.display();

      if (health <= 0) {
        display.invertDisplay(false);
        if (score > hiScore) hiScore = score;
        static const uint16_t tune[] = {300, 250, 200, 150, 100};
        rumble(500);
        for (uint8_t i = 0; i < 5; i++) { tone(PIN_BUZZER, tune[i], 120); delay(140); }
        state = DEAD;
      } else if (mobsAlive == 0) {
        display.invertDisplay(false);
        static const uint16_t tune[] = {523, 659, 784, 1047};
        for (uint8_t i = 0; i < 4; i++) { tone(PIN_BUZZER, tune[i], 110); delay(130); }
        score += 100 * level;
        level++;
        state = CLEARED;
      }
      break;
    }
  }
}
