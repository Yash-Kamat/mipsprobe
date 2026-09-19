#ifndef UART_H
#define UART_H
#include "stm32l4xx.h"

extern void UART_config(uint32_t baud);
extern void UART_send_byte(char data);
extern void UART_send_buffer(const char* buffer);

// These print the number alone, with no trailing newline, so they can be used
// mid-line. Add your own "\r\n" when you want one.
extern void UART_send_uint32(uint32_t number);
extern void UART_send_hex8(uint8_t number);
extern void UART_send_hex16(uint16_t number);
extern void UART_send_hex32(uint32_t number);

// Interrupt-driven RX into an internal ring buffer (see uart.c). USART2_IRQHandler
// fills it; these just drain it.
extern uint8_t UART_rx_available(void);          // number of bytes currently buffered
extern int UART_receive_byte(void);               // -1 if buffer empty, else 0-255
extern uint8_t UART_receive_byte_blocking(void);   // spins until a byte is available

// DMA TX/RX

#endif