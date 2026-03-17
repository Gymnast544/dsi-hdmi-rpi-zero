# SD Card Setup for DSi HDMI

This folder contains the built kernel for Raspberry Pi Zero. To run without building from source:

## Quick Setup (pre-built)

1. **Format an SD card** as FAT32 (first partition)

2. **Copy all files** from this folder to the SD card root:
   - `kernel7.img` — the DSi capture firmware
   - `bootcode.bin`, `start.elf`, `config.txt` — Pi boot files (included)

3. Insert SD card into Pi Zero, connect DSi + HDMI, power on.

## File Locations

| File | Location |
|------|----------|
| Built kernel | `release/kernel7.img` |
| After `make release` | Same — overwrites with fresh build |

## Building from Source

See the main [README](../README.md) for build instructions.
