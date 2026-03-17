# DSi HDMI — Nintendo DSi Video Output for Raspberry Pi Zero

Bare-metal Raspberry Pi Zero firmware that captures video from a Nintendo DSi top screen and outputs it via HDMI.

## Demo

[YouTube Short demo](https://www.youtube.com/shorts/CB0fu9gu2rI)

## Context

This is a **CS140e (Stanford) final project** repository.

- **About this repo**: it was generated/packaged with the help of AI to clean up the code layout/style and to make it easy to build + run as a standalone release repo.
- **Support**: if the build or SD-card boot process doesn’t work on your setup, please open an issue with your OS/toolchain version and the full build output.
- **Credit**: this project heavily relies on the excellent bare-metal Raspberry Pi infrastructure provided by the **CS140e staff** (vendored here as `libpi/`, plus boot/firmware conventions). Any mistakes in integration are on this repo, not on the staff code.

## Features

- Real-time capture from DSi LCD (256×192, 6-bit RGB)
- HDMI output at 320×240 (2× scale)
- DMA-based GPIO sampling
- MMU + cache for performance

## Supported Boards

- **Raspberry Pi Zero**
- **Raspberry Pi Zero W**

Only these boards are supported (BCM2835). Pi Zero 2, Pi 1 Model B+, and other variants are not tested.

## Hardware Installation

### DSi motherboard test points

`assets/Twl_back.jpg` is a photo of the DSi motherboard (back side) with the relevant test points labeled.

![Nintendo DSi motherboard test points](assets/Twl_back.jpg)

Image credit: [DSiBrew — File:Twl_back.jpg](https://dsibrew.org/wiki/File:Twl_back.jpg)

#### Color signal label format

The labeled color test points follow the format **`LD<C><S><B>`**:

- **`C`**: color channel — `R`, `G`, or `B`
- **`S`**: screen number — `1` = bottom screen, `2` = top screen
- **`B`**: bit index — `0` through `5`

In addition, **`LS`**, **`GSP`**, and **`DCLK`** are labeled on the image.

#### Power / ground notes

- **Ground**: `VGND` near the charging port works well for ground.
- **Optional DSi power from the Pi**: you can connect **5V** to **`VIN`** (also near the charging port) if you want the Pi to power/charge the DSi.

### Known limitations (artifacting / performance)

- **Artifacting**: due to Raspberry Pi Zero capture limits, some lines may be consistently copied from the line directly above.
- **Performance**: runs at ~**24 FPS** on a Pi Zero and is not perfectly pixel-accurate. The pixel differences are usually subtle unless you’re looking for them.

### Pinout

GPIO 0–3 are avoided (hardware 1.8kΩ pull-ups on PCB). GPIO 14/15 are reserved for UART TX/RX.

> **Note:** Pin 31 (GPIO 6) is physically wired to **LS** (line sync) on the DSi. The code refers to this signal as GCK — they are the same signal.

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

### Pixel decode (from GPLEV0 sample)

```c
uint8_t r = ((cur >>  7) & 0x3F) << 2;   // GPIO 7-12
uint8_t g = ((cur >> 16) & 0x3F) << 2;   // GPIO 16-21
uint8_t b = ((cur >> 22) & 0x3F) << 2;   // GPIO 22-27
```

## Quick Start — SD Card (No Build)

1. Copy all files from the `release/` folder to a FAT32 SD card (or download from [releases](https://github.com/Gymnast544/dsi-hdmi-rpi-zero/releases)).
2. Boot the Pi Zero.

See [release/README.md](release/README.md) for detailed SD card setup.

## Building from Source

### Prerequisites

- **arm-none-eabi-gcc** toolchain (e.g. `arm-none-eabi-gcc` from [Arm GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) or your distro’s `gcc-arm-none-eabi` package)

No other repos or external dependencies — libpi is vendored in this repo.

### Build

```bash
git clone https://github.com/Gymnast544/dsi-hdmi-rpi-zero.git
cd dsi-hdmi-rpi-zero

# Build for SD card (runs indefinitely):
make SD_BOOT=1

# Or build and prepare release folder:
make release
```

### Output

| Target | Output |
|--------|--------|
| `make SD_BOOT=1` | `src/dsi-hdmi.bin` |
| `make release` | `release/kernel7.img` — ready to copy to SD card |

## Project Structure

```
dsi-hdmi-rpi-zero/
├── src/
│   ├── dsi-hdmi.c       # Main capture + display loop
│   ├── gpu-fb.c/h       # Framebuffer / HDMI via mailbox
│   ├── mbox.c/h         # GPU mailbox
│   ├── mmu.c, mmu-asm.S # MMU + cache
│   ├── vm-cache.h       # Cache setup for DMA
│   └── armv6-*.h        # ARM coprocessor definitions
├── libpi/           # Vendored Raspberry Pi bare-metal library
├── release/         # SD card files (after make release)
│   ├── kernel7.img  # Built kernel — copy to SD card
│   ├── bootcode.bin, start.elf, config.txt
│   └── README.md    # SD card setup instructions
└── Makefile
```

## License

MIT (or as per cs140e course materials).
