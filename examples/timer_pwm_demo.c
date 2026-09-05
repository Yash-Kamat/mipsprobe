/*
 * Timer/PWM demo — configures TIM2 channel 4 for PWM and ramps the duty
 * cycle up and down in a breathing-LED pattern.
 *
 * Build + flash: make DEMO=timer_pwm_demo flash
 */
#include "rcc.h"
#include "gpio.h"
#include "uart.h"
#include "timer.h"

int main(void)
{
    SysClockConfig();

    GPIO_Config();

    UART_config(115200);

    UART_send_buffer("\r\n");

    UART_send_buffer("===============STM32 Timer Configuration====================\r\n");
    TIM2_config(CMS_EDGE_ALIGNED, DIR_UPCOUNTER, 0);
    TIM2_set_chan_pwm(4,79, 999, 0);

    while (1)
    {
        for(uint32_t duty = 0; duty <= 999; duty += 20)
        {
            TIM2->CCR4 = duty;

            for(volatile uint32_t i = 0; i < 300000; i++);
        }

        for(uint32_t duty = 999; duty > 0; )
        {
            TIM2->CCR4 = duty;

            for(volatile uint32_t i = 0; i < 300000; i++);
            duty -= 20;
            duty = (duty>999)?0:duty;
        }
    }
}
