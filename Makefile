# DSi Video Capture - Raspberry Pi Zero
# Bare-metal capture from Nintendo DSi top screen, output via HDMI
#
# Build for SD card boot:  make SD_BOOT=1
# Build release (SD card files):  make release

PROGS := dsi-hdmi.c
COMMON_SRC := gpu-fb.c mbox.c mmu.c mmu-asm.S

OPT_LEVEL = -O2

# SD card boot: no auto-restart, runs indefinitely
ifdef SD_BOOT
CFLAGS_EXTRA += -DSD_BOOT
RUN = 0
endif

# Path to cs140e course repo (contains libpi). Set via env or: make CS140E_2026_PATH=/path/to/cs140e
# Default: sibling directory ../cs140e-26win
ifeq ($(CS140E_2026_PATH),)
CS140E_2026_PATH := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../cs140e-26win)
endif
export CS140E_2026_PATH

# Staff object files for memory allocation
L = $(CS140E_2026_PATH)/libpi/staff-objs
STAFF_OBJS += $(L)/staff-kmalloc.o

# Pull in libgcc for division
LIBGCC  = $(shell $(CC) -print-libgcc-file-name)
LIB_POST += $(LIBGCC)

# Use course build system
include $(CS140E_2026_PATH)/libpi/mk/Makefile.robust-v2

# --- Release / SD card targets ---
RELEASE_DIR := release
KERNEL_IMG := kernel7.img

.PHONY: release release-dir
release: SD_BOOT=1
release: dsi-hdmi.bin
	@mkdir -p $(RELEASE_DIR)
	@cp dsi-hdmi.bin $(RELEASE_DIR)/$(KERNEL_IMG)
	@echo ""
	@echo "=============================================="
	@echo "  SD card files ready in: $(RELEASE_DIR)/"
	@echo "=============================================="
	@echo ""
	@echo "  Copy these files to your SD card (FAT32 boot partition):"
	@echo "    - $(RELEASE_DIR)/$(KERNEL_IMG)"
	@echo "    - bootcode.bin, start.elf, config.txt (from Raspberry Pi firmware)"
	@echo ""
	@echo "  See $(RELEASE_DIR)/README.md for full SD card setup."
	@echo ""

release-dir:
	@echo "$(abspath $(RELEASE_DIR))"
