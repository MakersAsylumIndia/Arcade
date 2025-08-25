const int ldrPin = A1;
const int buzzerPin = 3;  // D3

void setup() {
  pinMode(buzzerPin, OUTPUT);
}

void loop() {
  int lightValue = analogRead(ldrPin);  // 0 (dark) to 1023 (bright)

  // Map to PWM range (0–255)
  int buzzerIntensity = map(lightValue, 0, 1023, 0, 255);

  // Optional: invert it if you want sound in dark, silence in bright
  // int buzzerIntensity = map(lightValue, 0, 1023, 255, 0);

  analogWrite(buzzerPin, buzzerIntensity);

  delay(50); // update rate
}
