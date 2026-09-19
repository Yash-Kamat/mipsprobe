#include "led.h"
#include "uart.h"
#include "gpio.h"

/*
 * LED wired from 3V3 to PB3: anode at 3V3, cathode at PB3. The pin therefore
 * SINKS the current and the logic is active-low -- driving PB3 low lights the
 * LED, driving it high puts both ends at 3V3 and the LED goes dark.
 */
#define LED_PORT GPIOB
#define LED_PIN  GPIO_PIN_3

// BSRR sets a pin from its low half-word and clears it from the high half, in
// one atomic write -- no read-modify-write on ODR.
static void led_write(uint8_t lit)
{
    LED_PORT->BSRR = lit ? (1U << (LED_PIN + 16)) : (1U << LED_PIN);
}

static void led_init(void)
{
    // Park the output high (LED dark) *before* claiming the pin, so it doesn't
    // flash on at boot -- ODR is writable no matter what MODER currently says.
    // PB3 is JTDO-TRACESWO out of reset, but st-flash talks SWD over PA13/PA14
    // only, so taking it as a plain GPIO costs us nothing.
    led_write(0);
    GPIO_manual_Config(LED_PORT, LED_PIN, GPIO_MODER_Output, GPIO_OTYPER_Push_Pull,
                       GPIO_OSPEEDR_Low, GPIO_PUPDR_None, GPIO_AFR_Default);
}

static void led_on(char *args)
{
    (void) args;
    led_write(1);
    UART_send_buffer("LED on\r\n");
}

static void led_off(char *args)
{
    (void) args;
    led_write(0);
    UART_send_buffer("LED off\r\n");
}

// Sub-commands. Add blink/fade here later: write the handler, add a descriptor,
// add it to subcommands[] -- nothing outside this file needs to change.
static const ConsoleCommand sub_on  = { "on",  "drive PB3 low  -- LED lit",  0, led_on  };
static const ConsoleCommand sub_off = { "off", "drive PB3 high -- LED dark", 0, led_off };

static const ConsoleCommand *const subcommands[] = { &sub_on, &sub_off };
#define SUBCOMMAND_COUNT (sizeof(subcommands) / sizeof(subcommands[0]))

static void led_run(char *args)
{
    if (!Console_dispatch(subcommands, SUBCOMMAND_COUNT, args))
    {
        UART_send_buffer("usage: led <sub-command>\r\n");
        Console_print_help(subcommands, SUBCOMMAND_COUNT);
    }
}

const ConsoleCommand console_cmd_led = {
    "led", "LED on PB3 -- type 'led' for sub-commands", led_init, led_run
};
