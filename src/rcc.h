#ifndef RCC_H
#define RCC_H

#include "stm32l4xx.h"

#ifndef PLLM
#define PLLM    0   //Divide by 1 (Refer RM section 6.4.4)
#endif

#ifndef PLLN
#define PLLN    10
#endif

#ifndef PLLR
#define PLLR    0   //Divide by 2 (Refer RM section 6.4.4)
#endif

#ifndef PLLQ
#define PLLQ    2   //Not required
#endif

#ifndef PLLP
#define PLLP    7   //Not required
#endif

void SysClockConfig (void);

#endif