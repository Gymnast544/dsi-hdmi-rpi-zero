# DSi HDMI — Nintendo DSi Video Output for Raspberry Pi Zero

Bare-metal Raspberry Pi Zero firmware that captures video from a Nintendo DSi top screen and outputs it via HDMI.

## Features

- Real-time capture from DSi LCD (256×192, 6-bit RGB)
- HDMI output at 320×240 (2× scale)
- DMA-based GPIO sampling
- MMU + cache for performance

## Hardware

- **Raspberry Pi Zero** (or Pi 1 — BCM2835)
- Nintendo DSi (top screen signals)
- Pin mapping: see [PINOUT-full-color.md](PINOUT-full-color.md)

## Quick Start — SD Card (No Build)

1. Get `kernel7.img` from the `release/` folder (after `make release`) or build it (see below).
2. Put it on a FAT32 SD card with Raspberry Pi boot files:
   - `kernel7.img` (this firmware)
   - `bootcode.bin`, `start.elf`, `config.txt` — from [Raspberry Pi firmware](https://github.com/raspberrypi/firmware/tree/master/boot)
3. Boot the Pi Zero.

See [release/README.md](release/README.md) for detailed SD card setup.

## Building from Source

### Prerequisites

- `arm-none-eabi-gcc` toolchain
- [cs140e](https://web.stanford.edu/class/cs140e/) course repo (for libpi)

### Build

```bash
# Clone this repo and the cs140e repo as siblings:
#   parent/
#     dsi-hdmi-rpi-zero/   <- this repo
#     cs140e-26win/        <- course repo with libpi

cd dsi-hdmi-rpi-zero

# Build for SD card (runs indefinitely):
make SD_BOOT=1

# Or build and prepare release folder:
make release
```

### Output

| Target | Output |
|--------|--------|
| `make SD_BOOT=1` | `dsi-hdmi.bin` in project root |
| `make release` | `release/kernel7.img` — ready to copy to SD card |

### Custom libpi Path

If cs140e is not in `../cs140e-26win`:

```bash
make CS140E_2026_PATH=/path/to/cs140e-26win SD_BOOT=1
```

## Project Structure

```
dsi-hdmi-rpi-zero/
├── dsi-hdmi.c       # Main capture + display loop
├── gpu-fb.c/h       # Framebuffer / HDMI via mailbox
├── mbox.c/h         # GPU mailbox
├── mmu.c, mmu-asm.S # MMU + cache (from VM lab)
├── vm-cache.h       # Cache setup for DMA
├── armv6-*.h        # ARM coprocessor definitions
├── PINOUT-full-color.md
├── release/         # SD card files (after make release)
│   ├── kernel7.img  # Built kernel — copy to SD card
│   └── README.md    # SD card setup instructions
└── Makefile
```

## License

MIT (or as per cs140e course materials).
