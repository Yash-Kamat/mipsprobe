# mipsprobe

An STM32 debug probe for a custom MIPS core running on an FPGA. A **NUCLEO-L432KC** talks to the
core's I2C slave and exposes it through a UART command console. It loads programs into the core's
instruction and data memory, starts it, waits for it to finish, and dumps memory and registers back
to your serial terminal.

The STM32 side is bare-metal: no HAL, no RTOS, registers accessed directly against the reference
manual. The core itself, a 5-stage pipelined MIPS-style processor on a ZedBoard, lives in
[`MIPS_proc/`](MIPS_proc/README.md).

This is a fork of [stm32-os](https://github.com/Yash-Kamat/stm32-os) (see the `upstream` remote).

## What's here

| Path | What it is |
|---|---|
| `MIPS_proc/` | The MIPS core: RTL, ZedBoard constraints, and [its README](MIPS_proc/README.md) with the I2C memory map, instruction set and conventions |
| `mips_assembly_test/` | Self-checking MIPS test program, plus the macros and Makefile that assemble programs for this core |
| `src/` | Probe firmware: `rcc`, `gpio`, `uart`, `i2c`; `console` with `console_commands/` (`led`, `fpga`); `fpga_mips` (the I2C memory-map driver); `mips_programs` (embedded program table); `mips_full_test.h` (generated) |
| `examples/` | `mips_console_demo.c` (the probe) and `led_console_demo.c` (console self-test) |
| `tools/bin2c.py` | Turns a raw `.bin` into a C byte array; used by `mips_assembly_test/Makefile` |
| `startup/`, `linker/` | Cortex-M startup file (ST's CMSIS file, unmodified) and the L432KC linker script |
| `vendor/` | ARM CMSIS and ST device headers, as git submodules (see [Setup](#setup)) |

A few other drivers and demos inherited from stm32-os are still in the tree (`src/sched.*`,
`src/timer.*`, `src/usb/`, and the `scheduler_demo`, `timer_pwm_demo`, `usb_demo` and
`systick_blink_demo` examples). Nothing in the probe uses them, so they aren't documented here; their
documentation lives in the [original stm32-os repository](https://github.com/Yash-Kamat/stm32-os).

## Demos

`examples/` holds one `main()` per demo, selected with the `DEMO` make variable:

| `DEMO=` | What it does |
|---|---|
| `mips_console_demo` (default) | The probe: console with the `led` and `fpga` commands |
| `led_console_demo` | Console with only the `led` command and no I2C. Use it to check the board, UART and console without the FPGA. |

## Hardware setup

| Connection | Details |
|---|---|
| Nucleo USB → laptop | The ST-LINK's virtual COM port carries USART2 (PA2/PA15), 115200 8N1. Open it with any terminal, e.g. `minicom -D /dev/ttyACM0 -b 115200`, with hardware flow control **off**. |
| PB6 → ZedBoard JA1 | I2C SCL |
| PB7 → ZedBoard JA2 | I2C SDA |
| GND ↔ GND | common ground, required |
| LED on PB3 (optional) | for `led on`/`led off`: anode to 3V3, cathode to PB3 through a series resistor (e.g. 330 Ω). Active low. |

Both boards run at 3.3 V, and both enable internal pull-ups on the I2C lines. The FPGA side (bitstream,
SW0 reset, LD0 "done" LED) is described in [`MIPS_proc/README.md`](MIPS_proc/README.md).

## Using the console

| Command | What it does |
|---|---|
| `help` | List commands |
| `led on` / `led off` | Drive the PB3 LED |
| `fpga list` | List the MIPS programs embedded in the firmware |
| `fpga load <n>` | Write program *n*'s instructions, data and TARGET_PC to the FPGA. It refuses if the core has already been started, so reset the FPGA first. |
| `fpga run` | Set RUN, starting the core |
| `fpga status` | Show running/done, the live PC and TARGET_PC |
| `fpga wait` | Poll until DONE sets, or give up after a few seconds, then show the status |
| `fpga dump <imem\|reg\|dmem> [start] [count]` | Read words back from instruction memory, the registers or data memory |

A typical run of the built-in test program:

```
(flip SW0 on the ZedBoard up, then down, to reset the core)
> fpga load 0
Loading 'full_test'... OK
> fpga run
Starting core... OK
> fpga wait
done: running=1  done=1  PC=0x000001B4  TARGET_PC=0x000001B4
> fpga dump dmem 254 2
  dmem[254] @0x23F8 = 0x00000000
  dmem[255] @0x23FC = 0x00000001
```

`DM[255] = 1` means every check passed. [`MIPS_proc/README.md`](MIPS_proc/README.md#what-full_test-checks)
explains every check and how to read a failure.

## Setup

### 1. Prerequisites

- `arm-none-eabi-gcc` / `-objcopy` / `-size` (the GNU Arm Embedded Toolchain)
- `st-flash` (part of [stlink-tools](https://github.com/stlink-org/stlink)) to flash over the Nucleo's built-in ST-LINK
- `make`, `git`
- Only to rebuild MIPS programs: `clang` with the MIPS target, `llvm-objcopy`, and `python3`

On Debian/Ubuntu:

```bash
sudo apt install gcc-arm-none-eabi stlink-tools make git
sudo apt install clang llvm python3     # only for mips_assembly_test/
```

### 2. Clone the repo and pull in the vendor SDKs

This repo does not vendor ARM's CMSIS or ST's device headers directly —
they're pulled in as **git submodules**, pinned to the exact commits this
project was built against. Clone with `--recurse-submodules` to get
everything in one step:

```bash
git clone --recurse-submodules <this-repo-url>
cd mipsprobe
```

If you already cloned without that flag:

```bash
git submodule update --init --recursive
```

`vendor/CMSIS` and `vendor/STM32L4_Device` will show up on GitHub as linked
sub-repositories pointing at their upstream projects
([ARM-software/CMSIS_5](https://github.com/ARM-software/CMSIS_5) and
[STMicroelectronics/cmsis-device-l4](https://github.com/STMicroelectronics/cmsis-device-l4))
— anyone can open them there, and can point their local checkout at a newer
commit later with:

```bash
cd vendor/CMSIS && git checkout <newer-commit-or-tag> && cd ../..
git add vendor/CMSIS
git commit -m "Bump CMSIS"
```

### 3. Build

```bash
make                          # builds examples/mips_console_demo.c (the default)
make DEMO=led_console_demo    # the console self-test instead
make -C mips_assembly_test    # only after editing MIPS assembly: regenerates src/mips_full_test.h
```

Output lands in `build/firmware.elf` / `build/firmware.bin`. Switching `DEMO` always relinks, so
the image never silently keeps the previous demo.

### 4. Flash

With the Nucleo board plugged in over USB:

```bash
make flash                          # builds and flashes the default demo
make DEMO=led_console_demo flash    # or pick one explicitly
```

This calls `st-flash` under the hood and writes to `0x08000000`.

### 5. FPGA

Build and program the bitstream from `MIPS_proc/` in Vivado. See [`MIPS_proc/README.md`](MIPS_proc/README.md)
for the board connections and which RTL files to copy into the Vivado project.

### 6. Reference documents

The ST datasheet and reference manual (RM0394) aren't in this repo — they're
large, ST-copyrighted PDFs. Grab them directly from ST if you need them:

- [STM32L432KC datasheet](https://www.st.com/en/microcontrollers-microprocessors/stm32l432kc.html)
- [RM0394 reference manual](https://www.st.com/resource/en/reference_manual/rm0394-stm32l41xxx42xxx43xxx44xxx45xxx46xxx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)

## Licensing

Original code (`src/`, `examples/`, `linker/`, `Makefile`) is MIT-licensed —
see [LICENSE](LICENSE). The vendor SDKs and the ST-authored startup file are
under their own Apache License 2.0 terms; details are in that same file.
