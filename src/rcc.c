#include "rcc.h"

void SysClockConfig (void)
{
    /*************>>>>>>> STEPS FOLLOWED <<<<<<<<************
	
	1. ENABLE HSI and wait for the HSI to become Ready
	2. Set the POWER ENABLE CLOCK and VOLTAGE REGULATOR
	3. Configure the FLASH PREFETCH and the LATENCY Related Settings
	4. Configure the PRESCALARS HCLK, PCLK1, PCLK2
	5. Configure the MAIN PLL
	6. Enable the PLL and wait for it to become ready
    7. Enable desired PLL outputs by configuring PLLPEN, PLLQEN, PLLREN in PLLCFGR
	8. Select the Clock Source and wait for it to be set
	
	********************************************************/

    //1. ENABLE HSI and wait for the HSI to become Ready
    RCC->CR |= RCC_CR_HSION;
    while(!(RCC->CR & RCC_CR_HSIRDY));

	//2. Set the POWER ENABLE CLOCK and VOLTAGE REGULATOR
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    PWR->CR1 = (PWR->CR1 & ~(PWR_CR1_VOS)) | (1U << PWR_CR1_VOS_Pos);       //Range 1
    while(PWR->SR2 & PWR_SR2_VOSF_Msk);

	//3. Configure the FLASH PREFETCH and the LATENCY Related Settings (RM section 3.3.3)
    FLASH->ACR &= ~FLASH_ACR_LATENCY;    
    FLASH->ACR |= FLASH_ACR_DCEN | FLASH_ACR_ICEN | FLASH_ACR_PRFTEN | FLASH_ACR_LATENCY_4WS;
    while((FLASH->ACR & FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_4WS);

    //4. Configure the PRESCALARS HCLK, PCLK1, PCLK2
	//AHB PR
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE_Msk)) | RCC_CFGR_HPRE_DIV1;

    //APB1 PR
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_PPRE1_Msk)) | RCC_CFGR_PPRE1_DIV1;

    //APB2 PR
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_PPRE2_Msk)) | RCC_CFGR_PPRE2_DIV1;

    //5. Configure the MAIN PLL
    RCC->CR &= ~(RCC_CR_PLLON);
    while((RCC->CR & RCC_CR_PLLRDY));

    RCC->PLLCFGR = (PLLM << RCC_PLLCFGR_PLLM_Pos) | 
                   (PLLN << RCC_PLLCFGR_PLLN_Pos) | 
                   (PLLR << RCC_PLLCFGR_PLLR_Pos) |
                   (RCC_PLLCFGR_PLLSRC_HSI);

    //7. Enable desired PLL outputs by configuring PLLPEN, PLLQEN, PLLREN in PLLCFGR (RM section 6.2.5)
	RCC->PLLCFGR |= (RCC_PLLCFGR_PLLREN);

    //6. Enable the PLL and wait for it to become ready
    RCC->CR |= (RCC_CR_PLLON);
    while(!(RCC->CR & RCC_CR_PLLRDY))
    {
    }

    //8. Select the Clock Source and wait for it to be set
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);

    SystemCoreClockUpdate();
}