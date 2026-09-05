//Only implement the stm32 hardware here.
#include "usb_hw.h"

void USB_HW_EnableClock(void);
void USB_HW_Reset(void);

void USB_HW_Enable(void);
void USB_HW_Disable(void);

void USB_HW_SetAddress(uint8_t addr);

void USB_HW_SetEndpointType(uint8_t ep,
                            uint16_t type);

void USB_HW_SetRxStatus(uint8_t ep,
                        uint16_t status);

void USB_HW_SetTxStatus(uint8_t ep,
                        uint16_t status);

void USB_HW_ClearCTR_RX(uint8_t ep);
void USB_HW_ClearCTR_TX(uint8_t ep);