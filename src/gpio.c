#include "gpio.h"

void GPIO_Config (void)
{
    /************* STEPS FOLLOWED ***************
     * 1. Enable the GPIO CLOCK
     * 2. Set the Pin as OUTPUT
     * 3. Configure the OUTPUT MODE
     * ******************************************/
    //1. Enable the GPIO CLOCK
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    while(!(RCC->AHB2ENR & RCC_AHB2ENR_GPIOAEN_Msk));

    //2. Set the Pin as OUTPUT
    GPIOA->MODER &= ~(3U<<(2*5));
    GPIOA->MODER |= (1U<<(2*5));              // pin PA5(bits 11:10) as Output (01)

    //3. Configure the OUTPUT MODE
    GPIOA->OTYPER &= ~(1U << 5);

    GPIOA->OSPEEDR &= ~(3U << (5 * 2));

    GPIOA->PUPDR &= ~(3U << (5 * 2));

    //     //1. Enable the GPIO CLOCK
    // RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    // while(!(RCC->AHB2ENR & RCC_AHB2ENR_GPIOBEN_Msk));

    // //2. Set the Pin as OUTPUT
    // GPIOB->MODER &= ~(3U<<(2*3));
    // GPIOB->MODER |= (1U<<(2*3));              // pin PB3(bits 7:6) as Output (01)

    // //3. Configure the OUTPUT MODE
    // GPIOB->OTYPER &= ~(1U << 3);
    // GPIOB->OSPEEDR &= ~(3U << (3 * 2));
    // GPIOB->PUPDR &= ~(3U << (3 * 2));
}

void GPIO_manual_Config(GPIO_TypeDef * gpio, 
                                uint8_t pin, 
                                uint8_t mode,
                                uint8_t otype,
                                uint8_t ospeed,
                                uint8_t pupd,
                                uint8_t alternate_func)
{
    //enable the gpio port clock
    switch((uint32_t)gpio)
    {
        case (uint32_t)GPIOA:
        {
                RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
                while(!(RCC->AHB2ENR & RCC_AHB2ENR_GPIOAEN_Msk));
                break;
        }
        case (uint32_t)GPIOB:
        {
                RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
                while(!(RCC->AHB2ENR & RCC_AHB2ENR_GPIOBEN_Msk));
                break;
        }
        default: return;
    }

    //configure the gpio port mode
    gpio->MODER &= ~(GPIO_MODER_Msk << (pin * 2));
    gpio->MODER |= (mode << (pin * 2));

    //configure the output type register
    gpio->OTYPER &= ~(GPIO_OTYPER_Msk << pin);
    gpio->OTYPER |= (otype << pin);

    //configure the output speed register
    gpio->OSPEEDR &= ~(GPIO_OSPEEDR_Msk << (pin * 2));
    gpio->OSPEEDR |= (ospeed << (pin * 2));

    //configure the pull up pull down resistor
    gpio->PUPDR &= ~(GPIO_PUPDR_Msk << (pin * 2));
    gpio->PUPDR |= (pupd << (pin * 2));

    if(mode == GPIO_MODER_Alternate)
    {
        //configure the alternate function
        uint8_t afr_index = pin >> 3;      // 0 for pins 0-7, 1 for pins 8-15
        uint8_t afr_shift = (pin & 0x7) * 4;
        
        gpio->AFR[afr_index] &= ~(GPIO_AFR_Msk << afr_shift);
        gpio->AFR[afr_index] |=  (alternate_func << afr_shift);
    }
}                                