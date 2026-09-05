#include "usb_pma.h"

//The USB BTABLE defined as an array
static volatile USB_BTableEntry *const BTABLE = (USB_BTableEntry*)(USB_PMA + USB_BTABLE_ADDR);

#define USB_PMA16(addr) (( volatile uint16_t*)(USB_PMA + addr))



void USB_PMA_Write(const void *src,
                   uint16_t pma_addr,
                   uint16_t len)
{
    pma_addr = pma_addr & 0xFFFE;
    len = (len + 1) & 0xFFFE;         //make len even so that we can access half words
    uint16_t index = 0;
    uint8_t * src_8 = (uint8_t *)src;
    while(len>0)
    {
        uint16_t value = src_8[0] |
                        (src_8[1] << 8);
        USB_PMA16(pma_addr) [index] = value;        //Cortex-M4 is little endian 
        src_8 += 2;
        index++;
        len-=2;
    }
}

void USB_PMA_Read(void *dst,
                  uint16_t pma_addr,
                  uint16_t len)
{
    pma_addr = pma_addr & 0xFFFE;
    len = (len + 1) & 0xFFFE;         //make len even so that we can access half words
    uint16_t index = 0;
    uint8_t * dst_8 = (uint8_t *)dst;
    while(len>0)
    {
        uint16_t value = USB_PMA16(pma_addr)[index];
        dst_8[0] = value & 0xFF;
        dst_8[1] = value >> 8;
        dst_8 += 2;
        index++;
        len-=2;
    }
}

void USB_PMA_SetTxAddress(uint8_t ep,
                          uint16_t addr)
{
    BTABLE[ep].TX_ADDR = (addr & 0xFFFEUL);     //Confirm the address is half-word aligned
}

void USB_PMA_SetRxAddress(uint8_t ep,
                          uint16_t addr)
{
    BTABLE[ep].RX_ADDR = (addr & 0xFFFEUL);
}

void USB_PMA_SetTxCount(uint8_t ep,
                        uint16_t count)
{
    BTABLE[ep].TX_COUNT = (count & 0x3FF);
}

void USB_PMA_SetRxBufferSize(uint8_t ep,
                             uint16_t size)
{
    uint16_t size_actual = ((size + 1) & 0xFFFE);     //make the size an even number

    if(size_actual>=(USB_SRAM_SIZE-(uint16_t)BTABLE[ep].RX_ADDR) || size_actual <=0) 
    {
        BTABLE[ep].RX_COUNT = (0);
        return;            //We don't wanna overflow from the USB_SRAM area
    }

    if(size_actual<=62)     //Due to this config, we never do BL_SIZE = 1 and NUM_BLOCK = 0 for 32 bytes
    {
        uint16_t BLSIZE = (0);
        uint16_t NUM_BLOCK = (size_actual / 2);
        BTABLE[ep].RX_COUNT = (BLSIZE << 15 | NUM_BLOCK << 10);        
    }
    else
    {
        uint16_t BLSIZE = (1);
        uint16_t NUM_BLOCK = (size_actual / 32) - 1;
        BTABLE[ep].RX_COUNT = (BLSIZE << 15 | NUM_BLOCK << 10);
    }

    return;
}

uint16_t USB_PMA_GetRxCount(uint8_t ep)
{
    uint16_t RX_COUNT = (BTABLE[ep].RX_COUNT);
    uint16_t size = (RX_COUNT&(0x1FF));
    return size;
}