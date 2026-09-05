#ifndef USB_PMA_H
#define USB_PMA_H

#include <stdint.h>

#define USB_SRAM_SIZE (0x400)
#define USB_SRAM_BASE (0x40006C00)
#define USB_SRAM_MAX (USB_SRAM_BASE + USB_SRAM_SIZE)

#define USB_PMA (USB_SRAM_BASE)

/* ---------- PMA Layout ---------- */

#define USB_BTABLE_ADDR        0x000

/* Endpoint 0 BTABLE entry */
#define USB_EP0_TX_ADDR        0x040
#define USB_EP0_RX_ADDR        0x080

/* Endpoint 1 (future CDC/HID) */
#define USB_EP1_TX_ADDR        0x0C0
#define USB_EP1_RX_ADDR        0x100

#define USB_BTABLE_ENTRY_SIZE      8

#define USB_EP0_BTABLE_OFFSET      0
#define USB_EP1_BTABLE_OFFSET      8
#define USB_EP2_BTABLE_OFFSET      16
#define USB_EP3_BTABLE_OFFSET      24

typedef volatile struct __attribute__((packed))
{
    uint16_t TX_ADDR;
    uint16_t TX_COUNT;

    uint16_t RX_ADDR;
    uint16_t RX_COUNT;

} USB_BTableEntry;


void USB_PMA_Write(const void *src,
                   uint16_t pma_addr,
                   uint16_t len);

void USB_PMA_Read(void *dst,
                  uint16_t pma_addr,
                  uint16_t len);

void USB_PMA_SetTxAddress(uint8_t ep,
                          uint16_t addr);

void USB_PMA_SetRxAddress(uint8_t ep,
                          uint16_t addr);

void USB_PMA_SetTxCount(uint8_t ep,
                        uint16_t count);

void USB_PMA_SetRxBufferSize(uint8_t ep,
                             uint16_t size);

uint16_t USB_PMA_GetRxCount(uint8_t ep);

#endif