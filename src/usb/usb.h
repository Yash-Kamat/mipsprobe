#ifndef USB_H
#define USB_H

#include "stm32l4xx.h"

void USB_Init(void);
void USB_FS_Configure();
void USB_IRQHandler(void);

#endif