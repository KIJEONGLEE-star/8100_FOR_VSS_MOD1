# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This repository contains firmware for the GD32E502 microcontroller used in the DANG8100 audio amplifier, which is a Secondary node in an A2B (Automotive Audio Bus) network. The project implements the AABCOP (AAB Communication Protocol) over A2B for audio system control and communication.

## Building and Flashing

### Build Commands

The project uses GCC for ARM to compile and OpenOCD for flashing:

```bash
# Navigate to project directory
cd /path/to/gd32mcu-main

# Compile using ARM GCC
arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -O2 -Wall -fmessage-length=0 -ffunction-sections -fdata-sections -c -MMD \
    -DGDMCU_FIRMWARE -I./inc -I./Firmware/CMSIS -I./Firmware/CMSIS/GD/gd32e502/Include \
    -I./Firmware/gd32e502_standard_peripheral/Include \
    src/*.c Firmware/CMSIS/GD/gd32e502/Source/system_gd32e502.c \
    Firmware/gd32e502_standard_peripheral/Source/*.c

# Link the object files
arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -T gd32e502_flash.ld -nostartfiles -Xlinker \
    --gc-sections -Wl,-Map=output.map -o output.elf *.o gcc_startup/startup_gd32e502.S \
    -lm

# Generate binary file
arm-none-eabi-objcopy -O binary output.elf output.bin
```

### Flash Command

To flash the firmware using OpenOCD:

```bash
# Flash using OpenOCD with the GDLink configuration
openocd -f openocd_gdlink.cfg -c "program output.bin 0x08000000 verify reset exit"
```

## Architecture and Components

### Key Files and Directories

- `inc/`: Header files
- `src/`: Source files including main application code
- `Firmware/`: Standard peripheral drivers and CMSIS
  - `CMSIS/`: ARM CMSIS core and GD32-specific headers
  - `gd32e502_standard_peripheral/`: HAL for GD32E502 peripherals
- `gcc_startup/`: Startup code for GCC
- `AAB_Specification_v1.3.md`: Detailed specification for AABCOP protocol

### Hardware

The project targets the GD32E502KBU7 microcontroller (Cortex-M33) with:
- 256KB Flash memory
- 32KB SRAM
- SPI interface for communication with A2B transceiver (AD2433)
- Up to 48 MHz clock frequency

### Protocol Implementation

The AABCOP protocol implementation includes:
1. A2B network discovery and initialization
2. Mailbox communication (4-byte packets)
3. Single and multi-frame message handling
4. Command handling based on CIDs (Command Identifiers)
5. Audio configuration and control

## Development Workflow

1. Modify code as needed in the src/ directory
2. Build using the GCC ARM toolchain
3. Flash using OpenOCD
4. Debug using GDB or monitor the UART output (configured in main.c)

## Key Protocol Concepts

- **Primary Node**: Main controller in the A2B network
- **Secondary Node**: Nodes like DANG8100 that respond to commands
- **UID**: Unit Identifier (DANG8100 has UID 0x60)
- **CID**: Command Identifier (defines the action to take)
- **Mailbox**: 4-byte buffer for SPI communication with A2B transceiver

These components work together to implement the communication layer in a distributed audio system over an A2B bus.
