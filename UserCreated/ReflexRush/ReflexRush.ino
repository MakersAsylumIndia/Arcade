/*
 * REFLEX RUSH - a call-and-respond reaction game for the "TheThing" board.
 *
 * The screen (and a sound cue) tells you what to do. Do it before the bar
 * runs out. Every success makes the next call faster. Wrong move = game over.
 *
 *   PUSH 1!   -> BUTTON2 (SW2 "switch #1") on D2      cue: two high beeps
 *   PUSH 2!   -> BUTTON3 (SW3 "switch #2") on D4      cue: two low beeps
 *   TWIST IT! -> turn the potentiometer (A0)          cue: rising chirp
 *   COVER IT! -> cover the LDR / light sensor (A1)    cue: falling chirp
 *   WAVE IT!  -> wave a hand over the HC-SR04 sonar   cue: warble
 *
 * COVER and WAVE are only used if those sensors are detected at power-up.
 * The schematic swaps TRIGGER/ECHO (D6/D7) between its two pages, so both
 * wirings are tried automatically.
 * Pressing the wrong button or twisting when you shouldn't ends the game
 * (shadows and hands near the sonar are never counted against you).
 * Best score is saved in EEPROM.
 *
 * Libraries: "Adafruit SH110X" and "Adafruit GFX Library".
 */
#include <Wire.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

const uint8_t PIN_POT = A0, PIN_LDR = A1, PIN_B1 = 2, PIN_B2 = 4,
              PIN_BUZZER = 3, PIN_HAPTIC = 5;
uint8_t pinTrig = 7, pinEcho = 6;

Adafruit_SH1106G display(128, 64, &Wire, -1);
#define WHITE SH110X_WHITE
#define BLACK SH110X_BLACK

enum Cmd : uint8_t { PUSH1, PUSH2, TWIST, COVER, WAVE, NUM_CMDS };
const char txt0[] PROGMEM = "PUSH 1!";
const char txt1[] PROGMEM = "PUSH 2!";
const char txt2[] PROGMEM = "TWIST IT!";
const char txt3[] PROGMEM = "COVER IT!";
const char txt4[] PROGMEM = "WAVE IT!";
const char *const cmdText[] PROGMEM = {txt0, txt1, txt2, txt3, txt4};
// Two-note sound cue per command
const uint16_t PROGMEM cueNotes[NUM_CMDS][2] = {
  {1568, 1568}, {392, 392}, {523, 1047}, {1047, 523}, {880, 659}};

bool enabled[NUM_CMDS] = {true, true, true, false, false};
bool hasLdr = false, hasSonar = false;

enum State { TITLE, CALL, SUCCESS, FAIL };
State state = TITLE;

Cmd current, lastCmd = NUM_CMDS;
uint8_t repeatCount = 0;
uint16_t score = 0, best = 0;
uint16_t windowMs;
unsigned long callStart, stateUntil, cueNoteAt = 0, hapticOff = 0;
uint16_t cueSecond = 0;
int potBase, ldrBase, sonarBase;
const __FlashStringHelper *failReason;

// ---------- Inputs ----------
bool b1() { return digitalRead(PIN_B1) == HIGH; }
bool b2() { return digitalRead(PIN_B2) == HIGH; }

int sonarCm() {
  digitalWrite(pinTrig, LOW);
  delayMicroseconds(2);
  digitalWrite(pinTrig, HIGH);
  delayMicroseconds(10);
  digitalWrite(pinTrig, LOW);
  unsigned long us = pulseIn(pinEcho, HIGH, 12000);   // ~2 m max, keeps loop fast
  return us ? us / 58 : 0;                             // 0 = nothing / timeout
}

bool probeSonar(uint8_t trig, uint8_t echo) {
  pinTrig = trig; pinEcho = echo;
  pinMode(pinTrig, OUTPUT);
  pinMode(pinEcho, INPUT);
  uint8_t good = 0;
  for (uint8_t i = 0; i < 6; i++) {
    int d = sonarCm();
    if (d >= 2 && d <= 200) good++;
    delay(30);
  }
  // Needs real echoes, so at power-up point the sonar at something
  // within ~2 m (a wall, the ceiling, your hand).
  return good >= 3;
}

void detectSensors() {
  // LDR: connected readings are stable; a floating pin wanders.
  int lo = 1023, hi = 0;
  for (uint8_t i = 0; i < 16; i++) {
    int v = analogRead(PIN_LDR);
    lo = min(lo, v); hi = max(hi, v);
    delay(5);
  }
  hasLdr = (hi - lo) < 60 && hi > 40;

  hasSonar = probeSonar(7, 6) || probeSonar(6, 7);
  if (!hasSonar) { pinMode(6, INPUT); pinMode(7, INPUT); }

  enabled[COVER] = hasLdr;
  enabled[WAVE] = hasSonar;
}

// ---------- Output ----------
void beep(unsigned int f, unsigned long ms) { tone(PIN_BUZZER, f, ms); }
void rumble(unsigned long ms) { digitalWrite(PIN_HAPTIC, HIGH); hapticOff = millis() + ms; }

void playCue(Cmd c) {
  uint16_t d = windowMs > 1500 ? 110 : 70;
  beep(pgm_read_word(&cueNotes[c][0]), d);
  cueSecond = pgm_read_word(&cueNotes[c][1]);
  cueNoteAt = millis() + d + 30;
}

// ---------- Drawing ----------
void centerP(const char *pText, int16_t y, uint8_t size) {
  char buf[12];
  strncpy_P(buf, pText, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = 0;
  int16_t x1, y1; uint16_t w, h;
  display.setTextSize(size);
  display.getTextBounds(buf, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((128 - w) / 2, y);
  display.print(buf);
}

void centerF(const __FlashStringHelper *t, int16_t y, uint8_t size) {
  int16_t x1, y1; uint16_t w, h;
  display.setTextSize(size);
  display.getTextBounds(t, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((128 - w) / 2, y);
  display.print(t);
}

void drawIcon(Cmd c, int16_t cx, int16_t cy) {
  switch (c) {
    case PUSH1: case PUSH2:
      display.drawCircle(cx, cy, 7, WHITE);
      display.fillCircle(cx, cy, 4, WHITE);
      display.setTextSize(1);
      display.setCursor(cx + 11, cy - 3);
      display.print(c == PUSH1 ? '1' : '2');
      break;
    case TWIST:
      display.drawCircle(cx, cy, 7, WHITE);
      display.drawLine(cx, cy, cx + 5, cy - 5, WHITE);
      display.fillTriangle(cx + 9, cy - 8, cx + 12, cy - 2, cx + 6, cy - 3, WHITE);
      break;
    case COVER:  // a sun with a hand-ish bar over it
      display.fillCircle(cx, cy + 2, 4, WHITE);
      for (int8_t a = -1; a <= 1; a++) display.drawLine(cx + a * 6, cy - 4, cx + a * 8, cy - 7, WHITE);
      display.fillRoundRect(cx - 10, cy - 1, 20, 4, 2, WHITE);
      break;
    case WAVE:
      for (uint8_t r = 3; r <= 9; r += 3) display.drawCircle(cx - 8, cy, r, WHITE);
      display.fillRect(cx - 18, cy - 10, 10, 21, BLACK);
      display.fillRect(cx - 10, cy - 3, 4, 7, WHITE);
      break;
    default: break;
  }
}

void drawCall(float remaining) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("Score "));
  display.print(score);
  display.setCursor(80, 0);
  display.print(F("Best "));
  display.print(best);
  drawIcon(current, 60, 22);
  centerP((const char *)pgm_read_ptr(&cmdText[current]), 36, 2);
  int16_t w = (int16_t)(124 * remaining);
  display.drawRect(0, 56, 128, 8, WHITE);
  if (w > 0) display.fillRect(2, 58, w, 4, WHITE);
  display.display();
}

// ---------- Game flow ----------
Cmd pickCommand() {
  Cmd c;
  do {
    c = (Cmd)random(NUM_CMDS);
  } while (!enabled[c] || (c == lastCmd && repeatCount >= 1));
  repeatCount = (c == lastCmd) ? repeatCount + 1 : 0;
  lastCmd = c;
  return c;
}

void newCall() {
  current = pickCommand();
  potBase = analogRead(PIN_POT);
  ldrBase = analogRead(PIN_LDR);
  if (hasSonar) { int d = sonarCm(); sonarBase = d ? d : 400; }
  playCue(current);
  callStart = millis();
  state = CALL;
}

void startGame() {
  score = 0;
  windowMs = 3000;
  lastCmd = NUM_CMDS;
  display.clearDisplay();
  centerF(F("GET READY"), 24, 2);
  display.display();
  for (uint8_t i = 0; i < 3; i++) { beep(660, 80); delay(350); }
  newCall();
}

void fail(const __FlashStringHelper *why) {
  failReason = why;
  noTone(PIN_BUZZER);
  rumble(400);
  if (score > best) {
    best = score;
    EEPROM.put(0, (uint16_t)0xB0B1);   // magic marker
    EEPROM.put(2, best);
  }
  static const uint16_t tune[] = {392, 330, 262, 196};
  for (uint8_t i = 0; i < 4; i++) { tone(PIN_BUZZER, tune[i], 140); delay(160); }
  state = FAIL;
  stateUntil = millis() + 800;    // ignore presses briefly
}

void succeed() {
  score++;
  beep(1319, 50);
  // Speed up: 6% faster each time, floor of 700 ms
  windowMs = max(700, (int)(windowMs * 0.94f));
  state = SUCCESS;
  stateUntil = millis() + (windowMs > 1500 ? 350 : 200);
}

// Returns which action (if any) the player is doing right now
// Bit flags: 1=PUSH1 2=PUSH2 4=TWIST 8=COVER 16=WAVE
uint8_t readActions() {
  uint8_t a = 0;
  if (b1()) a |= 1 << PUSH1;
  if (b2()) a |= 1 << PUSH2;
  if (abs(analogRead(PIN_POT) - potBase) > 200) a |= 1 << TWIST;
  if (hasLdr) {
    int v = analogRead(PIN_LDR);
    if (v < ldrBase * 0.6f && ldrBase - v > 40) a |= 1 << COVER;
  }
  if (hasSonar && current == WAVE) {   // only ping when needed (it's slow)
    int d = sonarCm();
    int trig = min(15, sonarBase - 8);
    if (d && d < trig) a |= 1 << WAVE;
  }
  return a;
}

// ---------- Arduino ----------
void setup() {
  pinMode(PIN_B1, INPUT);   // external 10k pull-downs
  pinMode(PIN_B2, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_HAPTIC, OUTPUT);
  digitalWrite(PIN_HAPTIC, LOW);
  if (!display.begin(0x3C, true)) { for (;;) { beep(200, 100); delay(1000); } }
  display.setTextColor(WHITE);

  uint16_t magic;
  EEPROM.get(0, magic);
  if (magic == 0xB0B1) EEPROM.get(2, best);

  display.clearDisplay();
  centerF(F("Checking"), 16, 1);
  centerF(F("sensors..."), 28, 1);
  display.display();
  detectSensors();
  randomSeed(analogRead(A6) ^ (analogRead(A7) << 3) ^ micros());
}

void loop() {
  unsigned long now = millis();
  if (hapticOff && now >= hapticOff) { digitalWrite(PIN_HAPTIC, LOW); hapticOff = 0; }
  if (cueNoteAt && now >= cueNoteAt) { beep(cueSecond, 70); cueNoteAt = 0; }

  switch (state) {
    case TITLE: {
      display.clearDisplay();
      centerF(F("REFLEX"), 2, 2);
      centerF(F("RUSH"), 20, 2);
      display.setTextSize(1);
      display.setCursor(4, 40);
      display.print(F("Light:"));
      display.print(hasLdr ? F("yes") : F("no"));
      display.setCursor(68, 40);
      display.print(F("Sonar:"));
      display.print(hasSonar ? F("yes") : F("no"));
      centerF(F("Press a button"), 54, 1);
      display.display();
      static bool armed = false;
      if (!b1() && !b2()) armed = true;
      else if (armed) { armed = false; while (b1() || b2()) delay(5); startGame(); }
      break;
    }

    case CALL: {
      float remaining = 1.0f - (float)(now - callStart) / windowMs;
      if (remaining <= 0) { fail(F("Too slow!")); break; }
      drawCall(remaining);
      uint8_t acts = readActions();
      uint8_t want = 1 << current;
      // Only buttons and twisting count as a "wrong move"
      uint8_t wrong = acts & ~want & ((1 << PUSH1) | (1 << PUSH2) | (1 << TWIST));
      if (wrong) fail(F("Wrong move!"));
      else if (acts & want) succeed();
      break;
    }

    case SUCCESS:
      // Wait until buttons are released and the pause has passed
      if (now >= stateUntil && !b1() && !b2()) newCall();
      break;

    case FAIL: {
      display.clearDisplay();
      centerF(F("GAME OVER"), 2, 2);
      centerF(failReason, 22, 1);
      display.setTextSize(1);
      display.setCursor(30, 34); display.print(F("Score: ")); display.print(score);
      display.setCursor(30, 44); display.print(F("Best:  ")); display.print(best);
      centerF(F("Press a button"), 56, 1);
      display.display();
      static bool armed = false;
      if (now < stateUntil) { armed = false; break; }
      if (!b1() && !b2()) armed = true;
      else if (armed) { armed = false; while (b1() || b2()) delay(5); startGame(); }
      break;
    }
  }
}
