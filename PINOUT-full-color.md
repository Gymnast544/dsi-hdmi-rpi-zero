# Full Color DSi Capture Pinout
# Date: 2026-03-07

GPIO 0–3 avoided (hardware 1.8kΩ pull-ups on PCB).
GPIO 14/15 reserved for UART TX/RX.

> Note: Pin 31 (GPIO 6) is physically wired to **LS** (line sync) on the DSi.
> The code refers to this signal as GCK — these are the same signal.

| Physical Pin | BCM GPIO | DSi Signal  |
|-------------|----------|-------------|
| Pin 7  | GPIO 4  | DCLK        |
| Pin 8  | GPIO 14 | *(UART TX)* |
| Pin 10 | GPIO 15 | *(UART RX)* |
| Pin 11 | GPIO 17 | G[1]        |
| Pin 12 | GPIO 18 | G[2]        |
| Pin 13 | GPIO 27 | B[5]        |
| Pin 15 | GPIO 22 | B[0]        |
| Pin 16 | GPIO 23 | B[1]        |
| Pin 18 | GPIO 24 | B[2]        |
| Pin 19 | GPIO 10 | R[3]        |
| Pin 21 | GPIO 9  | R[2]        |
| Pin 22 | GPIO 25 | B[3]        |
| Pin 23 | GPIO 11 | R[4]        |
| Pin 24 | GPIO 8  | R[1]        |
| Pin 26 | GPIO 7  | R[0]        |
| Pin 29 | GPIO 5  | GSP         |
| Pin 31 | GPIO 6  | LS (= GCK in code) |
| Pin 32 | GPIO 12 | R[5]        |
| Pin 33 | GPIO 13 | *(spare)*   |
| Pin 35 | GPIO 19 | G[3]        |
| Pin 36 | GPIO 16 | G[0]        |
| Pin 37 | GPIO 26 | B[4]        |
| Pin 38 | GPIO 20 | G[4]        |
| Pin 40 | GPIO 21 | G[5]        |

## Pixel decode (from GPLEV0 sample)

```c
uint8_t r = ((cur >>  7) & 0x3F) << 2;   // GPIO 7-12
uint8_t g = ((cur >> 16) & 0x3F) << 2;   // GPIO 16-21
uint8_t b = ((cur >> 22) & 0x3F) << 2;   // GPIO 22-27
```
