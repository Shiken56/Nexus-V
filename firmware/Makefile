PREFIX = riscv-none-elf-
CC = $(PREFIX)gcc
OBJCOPY = $(PREFIX)objcopy

CFLAGS_BASE = -march=rv32i -mabi=ilp32 -nostartfiles -Os

# --- BOOTLOADER DEFINITIONS ---
BOOT_SRCS = boot_start.s boot.c
BOOT_CFLAGS = $(CFLAGS_BASE) -T boot.ld

# --- APPLICATION DEFINITIONS ---
# If no APP is specified in the terminal, default to app.c
APP ?= app.c
APP_SRCS = app_start.s $(APP)
APP_CFLAGS = $(CFLAGS_BASE) -T app.ld

all: bootloader.hex firmware.bin

# Build Bootloader (For Vivado synthesis)
bootloader.elf: $(BOOT_SRCS) boot.ld
	$(CC) $(BOOT_CFLAGS) $(BOOT_SRCS) -o bootloader.elf

bootloader.hex: bootloader.elf
	$(OBJCOPY) -O verilog --verilog-data-width=4 bootloader.elf bootloader.hex

# Build Application (For UART upload)
firmware.elf: $(APP_SRCS) app.ld
	$(CC) $(APP_CFLAGS) $(APP_SRCS) -o firmware.elf 

firmware.bin: firmware.elf
	$(OBJCOPY) -O binary firmware.elf firmware.bin

clean:
	del /Q /F *.elf *.hex *.bin