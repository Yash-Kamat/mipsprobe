/*
 * LED console demo -- a UART command console driving an LED wired between 3V3
 * and PB3 (anode 3V3, cathode PB3, so it's active-low).
 *
 * Deliberately links no I2C/FPGA code, so this runs on a bare NUCLEO-L432KC
 * with nothing attached but the LED. Use it to sanity-check the console itself.
 *
 * Build + flash: make DEMO=led_console_demo flash
 * Then open a serial terminal on USART2 (115200 8N1) and type 'help'.
 */
#include "rcc.h"
#include "uart.h"
#include "console.h"
#include "led.h"

static const ConsoleCommand *const commands[] = {
    &console_cmd_help,
    &console_cmd_led,
};

int main(void)
{
    SysClockConfig();

    UART_config(115200);

    Console_run(commands, sizeof(commands) / sizeof(commands[0])); // never returns

    while(1);
}
