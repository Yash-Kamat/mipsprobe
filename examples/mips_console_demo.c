/*
 * MIPS console demo -- the debug probe: a UART command console that loads,
 * runs and reads back programs on the FPGA-hosted MIPS core over I2C1, plus
 * the LED command for checking the console itself.
 *
 * Wiring: PB6 (SCL) -> ZedBoard JA1, PB7 (SDA) -> JA2, GND -> GND.
 *
 * Each run: toggle SW0 on the ZedBoard (FPGA reset), then
 *   fpga load 0  ->  fpga run  ->  fpga wait  ->  fpga dump dmem ...
 * (MIPS_proc/README.md has the memory map and what the test program checks.)
 *
 * Each command configures its own hardware from Console_run() (fpga brings up
 * I2C1, led claims PB3), so main() only has to set up the clock and UART.
 *
 * Build + flash: make DEMO=mips_console_demo flash
 * Then open a serial terminal on USART2 (115200 8N1) and type 'help'.
 */
#include "rcc.h"
#include "uart.h"
#include "console.h"
#include "led.h"
#include "fpga.h"

static const ConsoleCommand *const commands[] = {
    &console_cmd_help,
    &console_cmd_led,
    &console_cmd_fpga,
};

int main(void)
{
    SysClockConfig();

    UART_config(115200);

    Console_run(commands, sizeof(commands) / sizeof(commands[0])); // never returns

    while(1);
}
