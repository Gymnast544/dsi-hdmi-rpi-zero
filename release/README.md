# SD Card Setup for DSi HDMI

This folder contains the built kernel for Raspberry Pi Zero. To run without building from source:

## Quick Setup (pre-built)

1. **Get Raspberry Pi firmware files** (bootcode.bin, start.elf, config.txt):
   - Download from https://github.com/raspberrypi/firmware/raw/master/boot/
   - Or copy from any working Pi SD card

2. **Format an SD card** as FAT32 (first partition)

3. **Copy these files** to the SD card root:
   - `kernel7.img` (from this folder — the built program)
   - `bootcode.bin`
   - `start.elf`
   - `config.txt`

4. **Optional config.txt** for Pi Zero — add if needed:
   ```
   kernel=kernel7.img
   arm_64bit=0
   ```

5. Insert SD card into Pi Zero, connect DSi + HDMI, power on.

## File Locations

| File | Location |
|------|----------|
| Built kernel | `release/kernel7.img` |
| After `make release` | Same — overwrites with fresh build |

## Building from Source

See the main [README](../README.md) for build instructions.
