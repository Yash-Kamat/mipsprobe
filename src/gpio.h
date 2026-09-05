#ifndef GPIO_H
#define GPIO_H

#include "stm32l4xx.h"

/* ====================== PINS ===================== */

#define GPIO_PIN_0     0U
#define GPIO_PIN_1     1U
#define GPIO_PIN_2     2U
#define GPIO_PIN_3     3U
#define GPIO_PIN_4     4U
#define GPIO_PIN_5     5U
#define GPIO_PIN_6     6U
#define GPIO_PIN_7     7U
#define GPIO_PIN_8     8U
#define GPIO_PIN_9     9U
#define GPIO_PIN_10    10U
#define GPIO_PIN_11    11U
#define GPIO_PIN_12    12U
#define GPIO_PIN_13    13U
#define GPIO_PIN_14    14U
#define GPIO_PIN_15    15U

#define GPIO_MODER_Msk 0x3U
#define GPIO_MODER_Input 0x0U
#define GPIO_MODER_Output 0x1U
#define GPIO_MODER_Alternate 0x2U
#define GPIO_MODER_Analog 0x3U
#define GPIO_MODER_Default 0x3U

#define GPIO_OTYPER_Msk 0x1U
#define GPIO_OTYPER_Push_Pull 0x0U
#define GPIO_OTYPER_Open_Drain 0x1U
#define GPIO_OTYPER_Default 0x0U

/* ==================== OSPEEDR ==================== */

#define GPIO_OSPEEDR_Msk            0x3U
#define GPIO_OSPEEDR_Low            0x0U
#define GPIO_OSPEEDR_Medium         0x1U
#define GPIO_OSPEEDR_High           0x2U
#define GPIO_OSPEEDR_Very_High      0x3U
#define GPIO_OSPEEDR_Default        0x0U


/* ===================== PUPDR ===================== */

#define GPIO_PUPDR_Msk              0x3U
#define GPIO_PUPDR_None             0x0U
#define GPIO_PUPDR_Pull_Up          0x1U
#define GPIO_PUPDR_Pull_Down        0x2U
#define GPIO_PUPDR_Reserved         0x3U
#define GPIO_PUPDR_Default          0x0U


/* ====================== AFR ====================== */
/* AFRL -> Pins 0-7
 * AFRH -> Pins 8-15
 */

#define GPIO_AFR_Msk                0xFU

#define GPIO_AFR_AF0                    0x0U
#define GPIO_AFR_AF1                    0x1U
#define GPIO_AFR_AF2                    0x2U
#define GPIO_AFR_AF3                    0x3U
#define GPIO_AFR_AF4                    0x4U
#define GPIO_AFR_AF5                    0x5U
#define GPIO_AFR_AF6                    0x6U
#define GPIO_AFR_AF7                    0x7U
#define GPIO_AFR_AF8                    0x8U
#define GPIO_AFR_AF9                    0x9U
#define GPIO_AFR_AF10                   0xAU
#define GPIO_AFR_AF11                   0xBU
#define GPIO_AFR_AF12                   0xCU
#define GPIO_AFR_AF13                   0xDU
#define GPIO_AFR_AF14                   0xEU
#define GPIO_AFR_AF15                   0xFU

#define GPIO_AFR_Default             GPIO_AFR_AF0

void GPIO_manual_Config(GPIO_TypeDef * gpio, 
                                uint8_t pin, 
                                uint8_t mode,
                                uint8_t otype,
                                uint8_t ospeed,
                                uint8_t pupd,
                                uint8_t alternate_func);
void GPIO_Config (void);

#endif