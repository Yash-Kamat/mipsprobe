#include "usb.h"
#include "gpio.h"
#include "uart.h"
#include "usb_ep0.h"
#include "usb_pma.h"

void USB_FS_Configure()
{
    //==================PHASE 0 ====================//
    //Enable the clock for USB (HSI48)
    RCC->CRRCR |= RCC_CRRCR_HSI48ON;        //CRRCR - Clock Recovery RC Register
    while(!(RCC->CRRCR & RCC_CRRCR_HSI48RDY));

    //Enable clock recovery system peripheral
    RCC->APB1ENR1 |= RCC_APB1ENR1_CRSEN;

    //Configure the CRS
    CRS->CFGR = (0x2U << CRS_CFGR_SYNCSRC_Pos)      //crs_sync_in_2(USB SOF) selected as SYNC signal source
               |(0x0U << CRS_CFGR_SYNCDIV_Pos)      //000: SYNC not divided (default)
               |(0x0U << CRS_CFGR_SYNCPOL_Pos)      //0: SYNC active on rising edge (default)
               |(47999<< CRS_CFGR_RELOAD_Pos)
               |(34   << CRS_CFGR_FELIM_Pos);
    CRS->CR = (32   << CRS_CR_TRIM_Pos);
    CRS->CR |= (CRS_CR_AUTOTRIMEN | CRS_CR_CEN);

    //====================PHASE 1===================//
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    PWR->CR2 |= PWR_CR2_USV;            //Refer RM section 5.1.2

    //====================PHASE 2===================//
    RCC->APB1ENR1 |= RCC_APB1ENR1_USBFSEN;

    RCC->APB1RSTR1|= RCC_APB1RSTR1_USBFSRST;
    RCC->APB1RSTR1&=~RCC_APB1RSTR1_USBFSRST;

    //====================PHASE 3===================//
    GPIO_manual_Config(GPIOA, 
                       GPIO_PIN_11,
                       GPIO_MODER_Alternate,
                       GPIO_OTYPER_Push_Pull,
                       GPIO_OSPEEDR_Very_High,
                       GPIO_PUPDR_None,
                       GPIO_AFR_AF10);
    GPIO_manual_Config(GPIOA, 
                       GPIO_PIN_12,
                       GPIO_MODER_Alternate,
                       GPIO_OTYPER_Push_Pull,
                       GPIO_OSPEEDR_Very_High,
                       GPIO_PUPDR_None,
                       GPIO_AFR_AF10);

    //====================PHASE 4===================//
    USB->CNTR = USB_CNTR_FRES;      // Force USB reset
    for(volatile int i=0;i<1000;i++);   //Slight delay
    USB->CNTR = 0;

    //====================PHASE 5===================//
    USB->ISTR = 0;      //Clear stale interrupt flags

    //====================PHASE 6===================//
    USB->BTABLE = 0;    //Buffer Table starts at PMA address 0

    //====================PHASE 7===================//    
    USB->EP0R = USB_EP_CONTROL;

    //====================PHASE 8===================//
    USB->CNTR |=  USB_CNTR_RESETM
                | USB_CNTR_CTRM;        //Enable only important interrupts (RESET interrupt 
                                        // and Correct Transfer Interrupt)
    NVIC_EnableIRQ(USB_IRQn);

    //====================PHASE 9===================//
    USB->BCDR |= USB_BCDR_DPPU;
}
     
void USB_IRQHandler(void)
{
    UART_send_buffer("ISTR = ");
    UART_send_hex16(USB->ISTR);
    uint16_t istr = USB->ISTR;      /*we copy the register first because interrupt flags can 
                                      change while we're inside the ISR*/

    if (istr & USB_ISTR_RESET)
    {
        UART_send_buffer("EP0R = ");
        UART_send_hex16(USB->EP0R);

        USB_EP0_Init();
        
        UART_send_buffer("EP0R = ");
        UART_send_hex16(USB->EP0R);

        USB->DADDR = USB_DADDR_EF;

        UART_send_buffer("DADDR = ");
        UART_send_hex16(USB->DADDR);

        USB_BTableEntry *btable_local = ((USB_BTableEntry *)(USB_PMA));
        UART_send_buffer("TX_ADDR = ");
        UART_send_hex16(btable_local->TX_ADDR);

        UART_send_buffer("RX_ADDR = ");
        UART_send_hex16(btable_local->RX_ADDR);

        UART_send_buffer("RX_COUNT = ");
        UART_send_hex16(btable_local->RX_COUNT);

        UART_send_buffer("[USB] RESET\r\n");
        USB->ISTR &= ~USB_ISTR_RESET;
    }

    if (istr & USB_ISTR_CTR)
    {
        uint8_t ep = istr & USB_ISTR_EP_ID;

        volatile uint16_t * EndPArr = (uint16_t * ) USB;

        UART_send_buffer("[USB] CTR EP: ");
        UART_send_uint32(ep);

        uint16_t EPR = EndPArr[ep*2];
        if(EPR & USB_EP_CTR_RX) {UART_send_buffer("Received data\r\n");}
        if(EPR & USB_EP_SETUP) {UART_send_buffer("SETUP\r\n");}

        USB_EP0_HandleSetup();
        //USB->ISTR &= ~USB_ISTR_CTR;       //CTR is defined as a read only bit in the reference manual but stm32l432xx.h defines it as 
        /******************  Bits definition for USB_ISTR register  *******************/
// #define USB_ISTR_EP_ID                           ((uint16_t)0x000FU)           /*!< EndPoint IDentifier (read-only bit)  */
// #define USB_ISTR_DIR                             ((uint16_t)0x0010U)           /*!< DIRection of transaction (read-only bit)  */
// #define USB_ISTR_L1REQ                           ((uint16_t)0x0080U)           /*!< LPM L1 state request  */
// #define USB_ISTR_ESOF                            ((uint16_t)0x0100U)           /*!< Expected Start Of Frame (clear-only bit) */
// #define USB_ISTR_SOF                             ((uint16_t)0x0200U)           /*!< Start Of Frame (clear-only bit) */
// #define USB_ISTR_RESET                           ((uint16_t)0x0400U)           /*!< RESET (clear-only bit) */
// #define USB_ISTR_SUSP                            ((uint16_t)0x0800U)           /*!< SUSPend (clear-only bit) */
// #define USB_ISTR_WKUP                            ((uint16_t)0x1000U)           /*!< WaKe UP (clear-only bit) */
// #define USB_ISTR_ERR                             ((uint16_t)0x2000U)           /*!< ERRor (clear-only bit) */
// #define USB_ISTR_PMAOVR                          ((uint16_t)0x4000U)           /*!< DMA OVeR/underrun (clear-only bit) */
// #define USB_ISTR_CTR                             ((uint16_t)0x8000U)           /*!< Correct TRansfer (clear-only bit) */

// #define USB_CLR_L1REQ                        (~USB_ISTR_L1REQ)           /*!< clear LPM L1  bit */
// #define USB_CLR_ESOF                         (~USB_ISTR_ESOF)            /*!< clear Expected Start Of Frame bit */
// #define USB_CLR_SOF                          (~USB_ISTR_SOF)             /*!< clear Start Of Frame bit */
// #define USB_CLR_RESET                        (~USB_ISTR_RESET)           /*!< clear RESET bit */
// #define USB_CLR_SUSP                         (~USB_ISTR_SUSP)            /*!< clear SUSPend bit */
// #define USB_CLR_WKUP                         (~USB_ISTR_WKUP)            /*!< clear WaKe UP bit */
// #define USB_CLR_ERR                          (~USB_ISTR_ERR)             /*!< clear ERRor bit */
// #define USB_CLR_PMAOVR                       (~USB_ISTR_PMAOVR)          /*!< clear DMA OVeR/underrun bit*/
// #define USB_CLR_CTR                          (~USB_ISTR_CTR)             /*!< clear Correct TRansfer bit */
    }
}

/**
  * @brief Universal Serial Bus Full Speed Device
  */

// typedef struct
// {
//   __IO uint16_t EP0R;            /*!< USB Endpoint 0 register,                Address offset: 0x00 */
//   __IO uint16_t RESERVED0;       /*!< Reserved */
//   __IO uint16_t EP1R;            /*!< USB Endpoint 1 register,                Address offset: 0x04 */
//   __IO uint16_t RESERVED1;       /*!< Reserved */
//   __IO uint16_t EP2R;            /*!< USB Endpoint 2 register,                Address offset: 0x08 */
//   __IO uint16_t RESERVED2;       /*!< Reserved */
//   __IO uint16_t EP3R;            /*!< USB Endpoint 3 register,                Address offset: 0x0C */
//   __IO uint16_t RESERVED3;       /*!< Reserved */
//   __IO uint16_t EP4R;            /*!< USB Endpoint 4 register,                Address offset: 0x10 */
//   __IO uint16_t RESERVED4;       /*!< Reserved */
//   __IO uint16_t EP5R;            /*!< USB Endpoint 5 register,                Address offset: 0x14 */
//   __IO uint16_t RESERVED5;       /*!< Reserved */
//   __IO uint16_t EP6R;            /*!< USB Endpoint 6 register,                Address offset: 0x18 */
//   __IO uint16_t RESERVED6;       /*!< Reserved */
//   __IO uint16_t EP7R;            /*!< USB Endpoint 7 register,                Address offset: 0x1C */
//   __IO uint16_t RESERVED7[17];   /*!< Reserved */
//   __IO uint16_t CNTR;            /*!< Control register,                       Address offset: 0x40 */
//   __IO uint16_t RESERVED8;       /*!< Reserved */
//   __IO uint16_t ISTR;            /*!< Interrupt status register,              Address offset: 0x44 */
//   __IO uint16_t RESERVED9;       /*!< Reserved */
//   __IO uint16_t FNR;             /*!< Frame number register,                  Address offset: 0x48 */
//   __IO uint16_t RESERVEDA;       /*!< Reserved */
//   __IO uint16_t DADDR;           /*!< Device address register,                Address offset: 0x4C */
//   __IO uint16_t RESERVEDB;       /*!< Reserved */
//   __IO uint16_t BTABLE;          /*!< Buffer Table address register,          Address offset: 0x50 */
//   __IO uint16_t RESERVEDC;       /*!< Reserved */
//   __IO uint16_t LPMCSR;          /*!< LPM Control and Status register,        Address offset: 0x54 */
//   __IO uint16_t RESERVEDD;       /*!< Reserved */
//   __IO uint16_t BCDR;            /*!< Battery Charging detector register,     Address offset: 0x58 */
//   __IO uint16_t RESERVEDE;       /*!< Reserved */
// } USB_TypeDef;