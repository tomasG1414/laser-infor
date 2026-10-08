/*
  Laser -> photoresistor (LDR) data link  —  RECEIVER

  Decodes an on/off-keyed laser beam hitting a photoresistor and prints the
  received bytes on the Serial Monitor.

  Wiring (voltage divider):
      5V ---[ LDR ]---+---[ 10k ]--- GND
                      |
                      A0
  More light -> lower LDR resistance -> higher reading on A0.
  (If you wire it the other way round, set LIGHT_RAISES_READING to false.)

  Protocol (the transmitter must match it), UART-like, one frame per byte:
      idle      : laser OFF
      start bit : laser ON  for 1 bit time
      8 data    : LSB first, 1 = laser ON, 0 = laser OFF
      stop bit  : laser OFF for at least 1 bit time (2 recommended)

  Photoresistors are slow (tens of ms to react, slower to go dark than to
  go bright), so the bit time must be long. Start with 50-100 ms and lower
  it until errors appear.

  The dark/lit threshold calibrates itself: keep the laser OFF while the
  board boots, then it keeps tracking ambient light and laser brightness.
*/

// ---------------- Configuration ----------------
const uint8_t  SENSOR_PIN           = A0;
const uint32_t BIT_US               = 50000UL;  // bit time in us, MUST match transmitter (50 ms = 20 bit/s)
const bool     LIGHT_RAISES_READING = true;     // see wiring above
const int      MIN_CONTRAST         = 40;       // smallest dark->lit ADC jump accepted as "laser on"
const int      INITIAL_CONTRAST     = 150;      // first guess of the jump until a real one is measured
const bool     RAW_PLOT_MODE        = false;    // true: stream readings for the Serial Plotter (to aim/tune)
const uint32_t SERIAL_BAUD          = 115200;

// ---------------- State ----------------
int darkLevel;    // running estimate of the reading with laser OFF
int brightLevel;  // running estimate of the reading with laser ON
unsigned long bytesOk = 0;
unsigned long framingErrors = 0;

int readLight() {
  int v = analogRead(SENSOR_PIN);
  return LIGHT_RAISES_READING ? v : 1023 - v;
}

int threshold() {
  return (darkLevel + brightLevel) / 2;
}

// Exponential moving average: level moves 1/2^shift of the way towards v.
void track(int &level, int v, uint8_t shift) {
  level += (v - level) / (1 << shift);
  if (brightLevel < darkLevel + MIN_CONTRAST) brightLevel = darkLevel + MIN_CONTRAST;
}

void waitUntil(unsigned long t) {
  while ((long)(micros() - t) < 0) {}
}

// Reads one bit centered at `center` (micros timestamp): three samples spread
// over the middle of the bit, majority vote. Also updates the level estimates.
bool sampleBit(unsigned long center) {
  const long spread = BIT_US / 6;
  int th = threshold();
  int sum = 0;
  uint8_t votes = 0;
  for (int8_t k = -1; k <= 1; k++) {
    waitUntil(center + k * spread);
    int v = readLight();
    sum += v;
    if (v > th) votes++;
  }
  bool bit = votes >= 2;
  track(bit ? brightLevel : darkLevel, sum / 3, 2);
  return bit;
}

// After a framing error: wait for one full bit time of darkness to resync.
void waitForIdle() {
  unsigned long darkSince = micros();
  while ((long)(micros() - darkSince) < (long)BIT_US) {
    if (readLight() > threshold()) darkSince = micros();
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD);

  // Calibrate ambient light (laser must be OFF now).
  long sum = 0;
  for (int i = 0; i < 200; i++) {
    sum += readLight();
    delay(2);
  }
  darkLevel = sum / 200;
  brightLevel = darkLevel + INITIAL_CONTRAST;

  if (!RAW_PLOT_MODE) {
    Serial.print(F("Laser receiver ready. Dark level: "));
    Serial.print(darkLevel);
    Serial.print(F("  bit time: "));
    Serial.print(BIT_US / 1000);
    Serial.println(F(" ms"));
  }
}

void loop() {
  int v = readLight();

  if (RAW_PLOT_MODE) {
    if (v <= threshold()) track(darkLevel, v, 6);
    else                  track(brightLevel, v, 4);
    Serial.print(F("reading:"));    Serial.print(v);
    Serial.print(F(" dark:"));      Serial.print(darkLevel);
    Serial.print(F(" bright:"));    Serial.print(brightLevel);
    Serial.print(F(" threshold:")); Serial.println(threshold());
    delay(5);
    return;
  }

  // Idle: follow slow ambient-light changes.
  if (v <= threshold()) {
    track(darkLevel, v, 6);
    return;
  }

  // Light detected: possible start bit.
  unsigned long edge = micros();
  if (!sampleBit(edge + BIT_US / 2)) return;  // too short, it was a glitch

  uint8_t data = 0;
  for (uint8_t i = 0; i < 8; i++) {
    if (sampleBit(edge + (i + 1) * BIT_US + BIT_US / 2)) data |= (1 << i);
  }

  if (sampleBit(edge + 9 * BIT_US + BIT_US / 2)) {  // stop bit must be dark
    framingErrors++;
    Serial.print(F("\n[framing error #"));
    Serial.print(framingErrors);
    Serial.println(F("]"));
    waitForIdle();
    return;
  }

  bytesOk++;
  Serial.write(data);
}
