#ifndef USB_EP0_H
#define USB_EP0_H

#include "stm32l4xx.h"

void USB_EP0_Init(void);

void USB_EP0_HandleSetup(void);

void USB_EP0_SendData(const void *buf,
                      uint16_t len);

void USB_EP0_Stall(void);

#endif