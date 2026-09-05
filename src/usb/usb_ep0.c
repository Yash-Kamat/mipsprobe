#include "usb_ep0.h"
#include "usb_pma.h"
#include "usb_requests.h"
#include "uart.h"

void USB_EP0_Init(void)
{
    //Configure endpoint 0
    USB->EP0R |= USB_EP_CONTROL | (0x00 << 00);

    //Configure PMA addresses
    USB_PMA_SetTxAddress(0x0,USB_EP0_TX_ADDR);

    USB_PMA_SetRxAddress(0x0,USB_EP0_RX_ADDR);

    //Configure RX/TX buffer sizes
    USB_PMA_SetTxCount(0,0);

    USB_PMA_SetRxBufferSize(0,64);

    //Enable RX
    USB->EP0R |= 0x3<<12;           //Toggle these bits to 11 binary which corresponds to VALID(enabled for reception)
                                    //according to reference manual page 1568
}

void USB_EP0_HandleSetup(void)
{
    uint16_t rx_count = USB_PMA_GetRxCount(0);
    USB_SetupPacket packet;
    USB_PMA_Read((void *) &packet,0x80, sizeof(USB_SetupPacket));

    UART_send_buffer("bmRequestType = "); UART_send_hex8(packet.bmRequestType);
    UART_send_buffer("bRequest      = "); UART_send_hex8(packet.bRequest);
    UART_send_buffer("wValue        = "); UART_send_hex16(packet.wValue);
    UART_send_buffer("wIndex        = "); UART_send_hex16(packet.wIndex);
    UART_send_buffer("wLength       = "); UART_send_hex16(packet.wLength);

    USB->EP0R &= ~(USB_EP_CTR_RX);
}

void USB_EP0_SendData(const void *buf,
                      uint16_t len)
{

}

void USB_EP0_Stall(void)
{

}