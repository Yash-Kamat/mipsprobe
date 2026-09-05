#include "uart.h"

void UART_config(uint32_t baud)
{
    if(baud ==0) return ;
    //RCC enable
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;

    //Configure GPIO pins (UART_2_TX: PA2, UART_2_RX: PA15)
        //1. Enable the GPIO CLOCK
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    while(!(RCC->AHB2ENR & RCC_AHB2ENR_GPIOAEN_Msk));

    //2. Set the Pin as Alternate Function
    GPIOA->MODER &= ~((3U<<(2*15)) | (3U<<(2*2)));
    GPIOA->MODER |= (2U<<(2*15)) | (2U<<(2*2));              // pin PB3(bits 7:6) as Output (10)

    //3. Configure the Alternate Function Mode
    GPIOA->OTYPER &= ~((1U<<(15)) | (1U<<(2)));     //GPIO Push Pull

    GPIOA->OSPEEDR &= ~((3U << (2*2)) | (3U << (2*15)));
    GPIOA->OSPEEDR |= (3U << (2*2)) |
                      (3U << (2*15));

    GPIOA->PUPDR &= ~((3U << (2*2)) | (3U << (2*15)));

    GPIOA->AFR[0] &= ~(0xFU << (4*2));
    GPIOA->AFR[0] |= (7U << (4*2));

    GPIOA->AFR[1] &= ~(0xFU << (4*(15-8)));
    GPIOA->AFR[1] |= (3U << (4*(15-8)));

    //     Character transmission procedure (From Reference Manual)
    // 1. Program the M bits in USART_CR1 to define the word length.
    USART2->CR1 &= ~USART_CR1_UE;
    USART2->CR1 &= ~(USART_CR1_M1 | USART_CR1_M0);

    // 2. Select the desired baud rate using the USART_BRR register. (USARTDIV = 833)
    USART2->BRR = ((uint32_t) SystemCoreClock / baud);

    // 3. Program the number of stop bits in USART_CR2.
    USART2->CR2 &= ~(USART_CR2_STOP_Msk);

    // 4. Enable the USART by writing the UE bit in USART_CR1 register to 1.
    USART2->CR1 |= USART_CR1_UE;

    // 5. Select DMA enable (DMAT) in USART_CR3 if multibuffer communication is to take
    // place. Configure the DMA register as explained in multibuffer communication.
    
    // 6. Set the TE bit in USART_CR1 to send an idle frame as first transmission.
    USART2->CR1 |= USART_CR1_TE;
    while (!(USART2->ISR & USART_ISR_TEACK));

    // 7. Write the data to send in the USART_TDR register (this clears the TXE bit). Repeat this
    // for each data to be transmitted in case of single buffer.
    
    // 8. After writing the last data into the USART_TDR register, wait until TC=1. This indicates
    // that the transmission of the last frame is complete. This is required for instance when
    // the USART is disabled or enters the Halt mode to avoid corrupting the last
    // transmission.
}

void UART_send_byte(char data)
{
    while(!(USART2->ISR & USART_ISR_TXE));
    USART2->TDR = data;
}
void UART_send_buffer(const char* buffer)
{
    // 6. Set the TE bit in USART_CR1 to send an idle frame as first transmission.

    // 7. Write the data to send in the USART_TDR register (this clears the TXE bit). Repeat this
    // for each data to be transmitted in case of single buffer.
    while(*buffer)
    {
        UART_send_byte(*buffer++);
    }
    // 8. After writing the last data into the USART_TDR register, wait until TC=1. This indicates
    // that the transmission of the last frame is complete. This is required for instance when
    // the USART is disabled or enters the Halt mode to avoid corrupting the last
    // transmission.
    while(!(USART2->ISR & USART_ISR_TC));
}

void UART_send_uint32(uint32_t number)
{
    if(number == 0)
    {
        UART_send_buffer("0\r\n");
        return;
    }
    char buffer[20] = {(uint8_t) 0};
    uint8_t count = 0;
    while(number!=0)
    {
        buffer[count++]= number%10 +'0';
        number /= 10;
    }
    for(int i=0;i<count/2;i++)
    {
        char temp = buffer[count-1-i];
        buffer[count-1-i] = buffer[i];
        buffer[i] = temp;
    }
    buffer[count++] = '\r';
    buffer[count++] = '\n';
    buffer[count++] = '\0';

    UART_send_buffer(buffer);
}

void UART_send_hex8(uint8_t number)
{
    char buffer[20] = {(uint8_t)'0'};

    buffer[0] = '0';
    buffer[1] = 'x';
    buffer[2] = (((number >> 4) & 0xF) < 10) ? (((number >> 4) & 0xF) + '0') : (((number >> 4) & 0xF) - 10 + 'A');
    buffer[3] = (((number >> 0) & 0xF) < 10) ? (((number >> 0) & 0xF) + '0') : (((number >> 0) & 0xF) - 10 + 'A');
    buffer[4] = '\r';
    buffer[5] = '\n';
    buffer[6] = '\0';

    UART_send_buffer(buffer);
}
void UART_send_hex16(uint16_t number)
{
    char buffer[20] = {(uint8_t) '0'};

    buffer[0] = '0';
    buffer[1] = 'x';
    buffer[2] = (((number >>12) & 0xF) < 10) ? ((number >>12) & 0xF) + '0' : (((number >>12) & 0xF) - 10 + 'A');
    buffer[3] = (((number >> 8) & 0xF) < 10) ? ((number >> 8) & 0xF) + '0' : (((number >> 8) & 0xF) - 10 + 'A');
    buffer[4] = (((number >> 4) & 0xF) < 10) ? ((number >> 4) & 0xF) + '0' : (((number >> 4) & 0xF) - 10 + 'A');
    buffer[5] = (((number >> 0) & 0xF) < 10) ? ((number >> 0) & 0xF) + '0' : (((number >> 0) & 0xF) - 10 + 'A');
    buffer[6] = '\r';
    buffer[7] = '\n';
    buffer[8] = '\0';

    UART_send_buffer(buffer);
}
void UART_send_hex32(uint32_t number)
{
    char buffer[20] = {(uint8_t)'0'};

    buffer[0] = '0';
    buffer[1] = 'x';
    buffer[2] = (((number >> 28) & 0xF) < 10) ? (((number >> 28) & 0xF) + '0') : (((number >> 28) & 0xF) - 10 + 'A');
    buffer[3] = (((number >> 24) & 0xF) < 10) ? (((number >> 24) & 0xF) + '0') : (((number >> 24) & 0xF) - 10 + 'A');
    buffer[4] = (((number >> 20) & 0xF) < 10) ? (((number >> 20) & 0xF) + '0') : (((number >> 20) & 0xF) - 10 + 'A');
    buffer[5] = (((number >> 16) & 0xF) < 10) ? (((number >> 16) & 0xF) + '0') : (((number >> 16) & 0xF) - 10 + 'A');
    buffer[6] = (((number >> 12) & 0xF) < 10) ? (((number >> 12) & 0xF) + '0') : (((number >> 12) & 0xF) - 10 + 'A');
    buffer[7] = (((number >>  8) & 0xF) < 10) ? (((number >>  8) & 0xF) + '0') : (((number >>  8) & 0xF) - 10 + 'A');
    buffer[8] = (((number >>  4) & 0xF) < 10) ? (((number >>  4) & 0xF) + '0') : (((number >>  4) & 0xF) - 10 + 'A');
    buffer[9] = (((number >>  0) & 0xF) < 10) ? (((number >>  0) & 0xF) + '0') : (((number >>  0) & 0xF) - 10 + 'A');
    buffer[10] = '\r';
    buffer[11] = '\n';
    buffer[12] = '\0';

    UART_send_buffer(buffer);
}