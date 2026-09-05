#ifndef TIMER_H
#define TIMER_H

#include "stm32l4xx.h"

enum Config_mode {
    CMS_EDGE_ALIGNED,
    CMS_CENTER_ALIGNED_MODE1,
    CMS_CENTER_ALIGNED_MODE2,
    CMS_CENTER_ALIGNED_MODE3
};

enum Direction {
    DIR_UPCOUNTER,
    DIR_DOWNCOUNTER
};

extern void TIM2_config(uint8_t mode, uint8_t direction, uint32_t arr);

extern void TIM2_set_chan_pwm(uint8_t channel, uint16_t prescaler, uint32_t arr, uint32_t duty);

extern inline void TIM2_enable();

extern inline void TIM2_disable();
#endif
