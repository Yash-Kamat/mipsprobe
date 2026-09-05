/*
 * Scheduler demo — two round-robin tasks that cooperatively yield.
 *
 * Demonstrates OS_create_task() / OS_start_tasks() / OS_yield() from
 * src/sched.c, backed by the PendSV/SVC context-switch mechanism.
 *
 * Build + flash: make DEMO=scheduler_demo flash
 */
#include "rcc.h"
#include "gpio.h"
#include "uart.h"
#include "sched.h"

void task1(void)
{
    uint32_t counter_t1 = 0;

    while(1)
    {
        UART_send_buffer("[Task 1] counter = ");UART_send_uint32(counter_t1++);

        for(volatile uint32_t i=0;i<1000000;i++);

        OS_yield();
    }
}

void task2(void)
{
    uint32_t counter_t2 = 0;
    while(1)
    {
        UART_send_buffer("[Task 2] counter = ");UART_send_uint32(counter_t2++);

        for(volatile uint32_t i=0;i<1000000;i++);

        OS_yield();
    }
}

int main(void)
{
    SysClockConfig();

    GPIO_Config();

    UART_config(115200);

    OS_Init();

    OS_create_task(task1);

    OS_create_task(task2);

    OS_start_tasks();

    while(1);
}
