/*
  Laser -> photoresistor (LDR) ANALOG link  —  RECEIVER

  The transmitter varies the laser intensity (e.g. analogWrite(pin, 0..255),
  driven by a potentiometer, sensor, etc.). This sketch measures the light on
  the photoresistor, maps it back to the transmitted value (0..255) and:
    - prints it on Serial (open the Serial Plotter to see the waveform)
    - reproduces it on a PWM pin (put an LED + 220 ohm on OUTPUT_PIN)

  Wiring (voltage divider):
      5V ---[ LDR ]---+---[ 10k ]--- GND
                      |
                      A0
  More light -> lower LDR resistance -> higher reading on A0.
  (If you wire it the other way round, set LIGHT_RAISES_READING to false.)

  If the transmitter drives the laser with PWM, the LDR is far too slow to
  follow the ~490 Hz switching, so it naturally "sees" the average intensity:
  the PWM duty cycle becomes an analog light level.

  Calibration:
    - Keep the laser OFF while the board boots: that reading becomes 0.
    - Then make the transmitter go to full intensity once: the highest
      reading seen becomes 255.
    - Send 'c' on the Serial Monitor (laser OFF) to recalibrate at any time.
*/

// ---------------- Configuration ----------------
const uint8_t       SENSOR_PIN           = A0;
const uint8_t       OUTPUT_PIN           = 9;     // PWM pin that replays the received signal
const bool          LIGHT_RAISES_READING = true;  // see wiring above
const uint8_t       OVERSAMPLES          = 16;    // ADC readings averaged per sample (removes noise/ripple)
const float         SMOOTHING            = 0.3;   // 0..1, low-pass on the output (1 = no smoothing)
const int           MIN_RANGE            = 30;    // smallest dark->bright span accepted (avoids dividing noise)
const unsigned long SAMPLE_PERIOD_MS     = 10;    // 100 samples/s
const uint32_t      SERIAL_BAUD          = 115200;

// ---------------- State ----------------
int   darkLevel;    // reading with laser OFF    -> value 0
int   brightLevel;  // reading with laser at max -> value 255
float filtered = 0; // smoothed received value, 0..1
unsigned long lastSample = 0;

int readLight() {
  long sum = 0;
  for (uint8_t i = 0; i < OVERSAMPLES; i++) sum += analogRead(SENSOR_PIN);
  int v = sum / OVERSAMPLES;
  return LIGHT_RAISES_READING ? v : 1023 - v;
}

void calibrateDark() {
  long sum = 0;
  for (uint8_t i = 0; i < 20; i++) {
    sum += readLight();
    delay(5);
  }
  darkLevel = sum / 20;
  brightLevel = darkLevel + MIN_RANGE;
  filtered = 0;
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  pinMode(OUTPUT_PIN, OUTPUT);
  calibrateDark();
}

void loop() {
  if (Serial.available() && Serial.read() == 'c') calibrateDark();

  if (millis() - lastSample < SAMPLE_PERIOD_MS) return;
  lastSample += SAMPLE_PERIOD_MS;

  int raw = readLight();

  // Learn the range: the darkest and brightest readings seen so far.
  if (raw < darkLevel)   darkLevel = raw;
  if (raw > brightLevel) brightLevel = raw;

  float level = (float)(raw - darkLevel) / (brightLevel - darkLevel);
  level = constrain(level, 0.0, 1.0);
  filtered += SMOOTHING * (level - filtered);

  uint8_t value = (uint8_t)(filtered * 255 + 0.5);
  analogWrite(OUTPUT_PIN, value);

  // Serial Plotter friendly: label:value pairs
  Serial.print(F("raw:"));      Serial.print(raw);
  Serial.print(F(" dark:"));    Serial.print(darkLevel);
  Serial.print(F(" bright:"));  Serial.print(brightLevel);
  Serial.print(F(" value:"));   Serial.println(value);
}
