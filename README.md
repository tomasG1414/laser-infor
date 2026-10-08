# laser-infor

Proof of concept: sending an **analog** signal with a laser beam onto a photoresistor (LDR), using Arduino.

The transmitter varies the laser intensity (e.g. `analogWrite(laserPin, 0..255)`);
the receiver measures the light and reconstructs the value (0..255).

## Receiver (`receiver/receiver.ino`)

- Wiring: `5V — LDR — A0 — 10k — GND`; optional LED + 220 Ω on pin 9 replays the received signal.
- Serial at 115200 baud; open the **Serial Plotter** to see `raw`, `dark`, `bright` and `value`.

### Calibration
1. Laser **off** while the receiver boots → that light level maps to `0`.
2. Transmitter goes to **full intensity** once → that level maps to `255`.
3. Send `c` over Serial (laser off) to recalibrate.

### Limitations
- LDRs take tens of ms to react, so the signal bandwidth is only a few Hz.
- The LDR response is not linear with light, so the received value follows
  the sent one but is curved; a lookup table can correct it if needed.
- Shield the LDR from room light (a small tube) — ambient changes shift the zero.
