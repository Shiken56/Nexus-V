# 1. Define your toolchain prefix 
PREFIX = riscv64-unknown-elf-
CC = $(PREFIX)gcc
OBJCOPY = $(PREFIX)objcopy

# 2. Define the compiler flags
CFLAGS = -march=rv32i -mabi=ilp32 -nostartfiles -T linker.ld

# 3. Define the source files
SRCS = start.S main.c

# 4. The default target that runs when you just type 'make'
all: firmware.hex

# 5. Rule to build the ELF binary
firmware.elf: $(SRCS) linker.ld
	$(CC) $(CFLAGS) $(SRCS) -o firmware.elf

# 6. Rule to convert the ELF into the raw HEX format for Vivado
firmware.hex: firmware.elf
	$(OBJCOPY) -O verilog --verilog-data-width=4 firmware.elf firmware.hex

# 7. A cleanup command to remove generated files (Windows version)
clean:
	del /Q /F firmware.elf firmware.hex
