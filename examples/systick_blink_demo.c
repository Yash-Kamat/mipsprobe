/*
 * SysTick delay demo — blinks the Nucleo user LED (PA5) using a
 * SysTick-driven tick counter instead of a busy-wait loop.
 *
 * Build + flash: make DEMO=systick_blink_demo flash
 */
#include "rcc.h"
#include "gpio.h"
#include "uart.h"
#include "interrupts.h"

volatile uint32_t ticks = 0;

int main(void)
{
    SysClockConfig();
    GPIO_Config();
    SysTick_Config(80000);              //1 tick after 80000 clock cycles
    UART_config(115200);

    uint32_t last = 0;
    while(1)
    {
        if(ticks - last >= 125)
        {
            last = ticks;
            GPIOA->ODR ^= (1U<<5);
            UART_send_buffer("SystemCoreClock is: ");
            UART_send_uint32(SystemCoreClock); UART_send_buffer("\r\n");
            UART_send_buffer("Time is: ");
            UART_send_uint32(last);            UART_send_buffer("\r\n");
        }
    }
}
