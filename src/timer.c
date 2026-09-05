#include "timer.h"
#include "gpio.h"

inline void TIM2_enable()
{
    TIM2->CR1 |= TIM_CR1_CEN;
}

inline void TIM2_disable()
{
    TIM2->CR1 &= ~TIM_CR1_CEN;
}

void TIM2_config(uint8_t mode, uint8_t direction, uint32_t arr)
{
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;

    TIM2_disable();

    TIM2->CR1 &= ~(TIM_CR1_CMS_Msk | TIM_CR1_DIR_Msk | TIM_CR1_ARPE_Msk);

    TIM2->CR1 |= (mode << TIM_CR1_CMS_Pos) | (direction << TIM_CR1_DIR_Pos);

    TIM2->PSC = (uint16_t) (80000 - 1);       //Use internel clock freq (80MHz) / 80000 i.e. 1ms clock

    TIM2->ARR = (uint32_t) (arr?arr:6000);            //Count to 6000 ms and reload by default

    TIM2->CR1 |= (TIM_CR1_ARPE);

    TIM2->CNT = (uint32_t) 0;

    TIM2_enable();
}
void TIM2_set_chan_pwm(uint8_t channel, uint16_t prescaler, uint32_t arr, uint32_t duty)
{
    TIM2_disable();

    switch (channel)
    {
    case 1:
        TIM2->CCR1 = duty;
        break;
    case 2:
        TIM2->CCR2 = duty;
        break;
    case 3:
        TIM2->CCR3 = duty;
        TIM2->CCMR2 = (6 << TIM_CCMR2_OC3M_Pos) |
                      (TIM_CCMR2_OC3PE);
        TIM2->CCER |= (TIM_CCER_CC3E);
        break;
    case 4:
        TIM2->CCR4 = duty;
        TIM2->CCMR2 = (6 << TIM_CCMR2_OC4M_Pos) |
                      (TIM_CCMR2_OC4PE);
        TIM2->CCER |= (TIM_CCER_CC4E);
        break;
        
    default:
        return;
        break;
    }

    TIM2->PSC = prescaler;
    TIM2->ARR = arr;

    TIM2->EGR = (TIM_EGR_UG);
    GPIO_manual_Config(GPIOA, 
                       GPIO_PIN_3, 
                       GPIO_MODER_Alternate, 
                       GPIO_OTYPER_Default, 
                       GPIO_OSPEEDR_Very_High, 
                       GPIO_PUPDR_Default, 
                       GPIO_AFR_AF1);

    TIM2_enable();
}
