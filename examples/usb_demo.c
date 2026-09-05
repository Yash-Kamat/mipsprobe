/*
 * USB demo — brings up the USB FS peripheral (see src/usb/).
 *
 * NOTE: endpoint handling (src/usb/usb_ep.c / usb_ep.h) is still an empty
 * stub, so this only configures the peripheral registers — it does not
 * enumerate on a host yet.
 *
 * Build + flash: make DEMO=usb_demo flash
 */
#include "rcc.h"
#include "gpio.h"
#include "uart.h"
#include "usb/usb.h"

int main(void)
{
    SysClockConfig();

    GPIO_Config();

    UART_config(115200);

    UART_send_buffer("\r\n");
    UART_send_buffer("================================\r\n");
    UART_send_buffer("STM32 USB Experiment\r\n");
    UART_send_buffer("================================\r\n");

    USB_FS_Configure();

    UART_send_buffer("USB peripheral configured.\r\n");
    UART_send_buffer("Test value(uint32): ");
    UART_send_hex32(0x00304500);

    while(1)
    {

    }
}
