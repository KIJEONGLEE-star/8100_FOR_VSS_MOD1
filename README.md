<<<<<<< HEAD
<<<<<<< HEAD
# TANG
=======
# DANG8100 A2B Secondary Node Emulation

This project implements a firmware emulator for the DANG8100 amplifier which functions as a Secondary node in an A2B (Automotive Audio Bus) network. The implementation follows the AAB Communication Protocol (AABCOP) as described in the specification document.

## Overview

The DANG8100 amplifier is a Secondary node in an A2B network with the UID (Unit Identifier) of 0x60. This firmware runs on a GD32E502KBU7 microcontroller, which communicates with an AD2433 A2B transceiver via SPI.

## Features

- Implements AABCOP protocol for communication with Primary node
- Handles A2B mailbox messages (4-byte packets)
- Processes single and multi-frame messages
- Supports various Command Identifiers (CIDs) for:
  - Network configuration and discovery
  - Audio control (volume, mute)
  - Equalization (bass, treble)
  - Module configuration
  - Status reporting and diagnostics
- Monitors input voltage, current, and temperature
- Controls DAC outputs for audio channels
- LED status indicators

## Architecture

The firmware is organized into several modules:

- **main.c**: Main entry point and application loop
- **a2b.c**: Handles communication with the AD2433 A2B transceiver via SPI
- **aabcop.c**: Implements the AABCOP protocol for A2B network communication
- **peripherals.c**: Manages hardware peripherals (ADC, DAC, GPIO, etc.)

## Building and Flashing

To build the project, use ARM GCC with the following commands:

```bash
# Compile source files
arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -O2 -Wall -fmessage-length=0 -ffunction-sections -fdata-sections -c -MMD \
    -DGDMCU_FIRMWARE -I./inc -I./Firmware/CMSIS -I./Firmware/CMSIS/GD/gd32e502/Include \
    -I./Firmware/gd32e502_standard_peripheral/Include \
    src/*.c Firmware/CMSIS/GD/gd32e502/Source/system_gd32e502.c \
    Firmware/gd32e502_standard_peripheral/Source/*.c

# Link the object files
arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -T gd32e502_flash.ld -nostartfiles -Xlinker --gc-sections \
    -Wl,-Map=output.map -o output.elf *.o gcc_startup/startup_gd32e502.S \
    -lm

# Generate binary file
arm-none-eabi-objcopy -O binary output.elf output.bin
```

To flash the firmware to the GD32E502KBU7 using OpenOCD:

```bash
openocd -f openocd_gdlink.cfg -c "program output.bin 0x08000000 verify reset exit"
```

## Protocol Implementation

The firmware implements the AABCOP protocol as follows:

1. **Initialization**: Configure peripherals and initialize protocol state
2. **Discovery**: Detect and initialize the A2B network
3. **Running**: Process mailbox messages and respond to commands
4. **Error Handling**: Handle protocol errors and reset if necessary

## Supported Commands

The firmware handles various Command Identifiers (CIDs) including:

- **0x00**: Network Startup/Validation
- **0x01**: Response Command
- **0x13**: Zone Volume Absolute
- **0x15**: Mute Zone
- **0x16**: Mute Channels
- **0x32**: Equalizer Bass
- **0x33**: Equalizer Treble
- **0x60**: Channel Slot Assignment
- **0x61**: PLL Lock Status
- **0x70**: Module Enable
- **0x80**: Module Status

## Hardware Connections

The GD32E502KBU7 connects to the AD2433 A2B transceiver via SPI with the following pin assignments:

- **SPI SCK**: PA5
- **SPI MISO**: PA6
- **SPI MOSI**: PA7
- **SPI CS**: PA4
- **INT**: PB0

## Status Indicators

The firmware uses LEDs to indicate the current state:

- **LED1**: Shows the protocol state with different blink patterns
  - Fast blinking: Initialization
  - Double blink: Discovery
  - Slow pulse: Running
  - Rapid blinking: Error
- **LED2**: Can be toggled by pressing the user button

## License
=======
# TANG
>>>>>>> 74dac83 (Initial commit)
# 8100_FOR_VSS
