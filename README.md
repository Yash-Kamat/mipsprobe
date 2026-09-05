# stm32-os

Bare-metal drivers, a cooperative scheduler, and a hand-rolled Cortex-M4
context switch for the **STM32L432KC** on a **NUCLEO-L432KC** board — no HAL,
no RTOS library, registers accessed directly against the reference manual.

Written from scratch as a learning project: GPIO → RCC/clock config → UART →
timers/PWM → USB (in progress) → a PendSV/SVC-based cooperative task
scheduler.

## What's here

| Path | What it is |
|---|---|
| `src/` | Drivers: `gpio`, `rcc` (clock config), `uart`, `timer`, `sched` (scheduler + context switch), `usb/` |
| `examples/` | One `main()` per demo — see below |
| `startup/` | Cortex-M reset/vector table (ST's CMSIS startup file, unmodified) |
| `linker/` | Linker script for the L432KC's 256K flash / 48K RAM |
| `vendor/` | Third-party SDKs, pulled in as git submodules (see [Setup](#setup)) |

### Status of each driver

- **GPIO / RCC / UART / Timer-PWM**: working, exercised by their demos.
- **Scheduler** (`src/sched.c`): a cooperative round-robin scheduler with a
  PendSV/SVC-based context switch for Cortex-M. The switch routine
  (`PendSV_Handler`, `SVC_Handler`, `OS_start_tasks`) was adapted from the
  FreeRTOS Cortex-M port — it is not fully original, credited in
  [LICENSE](LICENSE).
- **USB** (`src/usb/`): peripheral bring-up and packet-memory-area (PMA)
  access work, but endpoint handling is unfinished —
  `src/usb/usb_ep.c`/`usb_ep.h` are currently empty stubs. The USB demo
  configures the peripheral but does **not** enumerate on a host yet.

### Demos

`examples/` holds four standalone programs, each with its own `main()`,
selected at build time via the `DEMO` make variable:

| `DEMO=` | What it does |
|---|---|
| `scheduler_demo` (default) | Two tasks round-robin over UART, driven by the scheduler |
| `timer_pwm_demo` | TIM2 PWM, breathing-LED duty cycle ramp |
| `usb_demo` | Brings up the USB FS peripheral (no enumeration yet) |
| `systick_blink_demo` | Blinks the Nucleo user LED (PA5) off a SysTick tick counter |

Run `make list-demos` to print this list from the Makefile itself.

All UART output is on USART2 (PA2/PA15) at 115200 baud, which on the
NUCLEO-L432KC comes out over the on-board ST-LINK's virtual COM port — open
it with any serial terminal (e.g. `screen /dev/ttyACM0 115200`).

## Setup

### 1. Prerequisites

- `arm-none-eabi-gcc` / `-objcopy` / `-size` (the GNU Arm Embedded Toolchain)
- `st-flash` (part of [stlink-tools](https://github.com/stlink-org/stlink)) to flash over the Nucleo's built-in ST-LINK
- `make`
- `git`

On Debian/Ubuntu:

```bash
sudo apt install gcc-arm-none-eabi stlink-tools make git
```

### 2. Clone the repo and pull in the vendor SDKs

This repo does not vendor ARM's CMSIS or ST's device headers directly —
they're pulled in as **git submodules**, pinned to the exact commits this
project was built against. Clone with `--recurse-submodules` to get
everything in one step:

```bash
git clone --recurse-submodules <this-repo-url>
cd stm32-os
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
make                          # builds examples/scheduler_demo.c by default
make DEMO=timer_pwm_demo      # or pick a different demo (see table above)
```

Output lands in `build/firmware.elf` / `build/firmware.bin`.

### 4. Flash

With the Nucleo board plugged in over USB:

```bash
make flash                    # flashes whichever DEMO you last built
# or in one line:
make DEMO=usb_demo flash
```

This calls `st-flash` under the hood and writes to `0x08000000`.

### 5. Reference documents

The ST datasheet and reference manual (RM0394) aren't in this repo — they're
large, ST-copyrighted PDFs. Grab them directly from ST if you need them:

- [STM32L432KC datasheet](https://www.st.com/en/microcontrollers-microprocessors/stm32l432kc.html)
- [RM0394 reference manual](https://www.st.com/resource/en/reference_manual/rm0394-stm32l41xxx42xxx43xxx44xxx45xxx46xxx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)

## Licensing

Original code (`src/`, `examples/`, `linker/`, `Makefile`) is MIT-licensed —
see [LICENSE](LICENSE). The vendor SDKs and the ST-authored startup file are
under their own Apache License 2.0 terms; details are in that same file.
