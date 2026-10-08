# laser-infor

Proof of concept: sending data with a laser beam onto a photoresistor (LDR), using Arduino.

## Receiver (`receiver/receiver.ino`)

Wiring: `5V — LDR — A0 — 10k — GND`. Open the Serial Monitor at 115200 baud.
Keep the laser **off** while the receiver boots (it measures ambient light).

Set `RAW_PLOT_MODE = true` and open the Serial Plotter to aim the laser and
check the dark/lit contrast before trying to send data.

## Protocol (transmitter must match)

UART-like on/off keying, one frame per byte:

| Part      | Laser                                  |
|-----------|----------------------------------------|
| Idle      | OFF                                    |
| Start bit | ON for 1 bit time                      |
| 8 data    | LSB first, `1` = ON, `0` = OFF         |
| Stop bit  | OFF for ≥ 1 bit time (2 recommended)   |

Default bit time: **50 ms** (`BIT_US` in the receiver). LDRs react in tens of
milliseconds, so start slow and decrease until framing errors appear.
