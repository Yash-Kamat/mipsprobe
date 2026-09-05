#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "stm32l4xx.h"

extern volatile uint32_t ticks;

void SysTick_Handler()
{
    ticks++;
}

#endif