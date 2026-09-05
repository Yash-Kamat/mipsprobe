#ifndef USB_REQUESTS_H
#define USB_REQUESTS_H

#include "stm32l4xx.h"

enum USB_Request
{
    USB_GET_STATUS        = 0,
    USB_CLEAR_FEATURE     = 1,
    USB_SET_FEATURE       = 3,
    USB_SET_ADDRESS       = 5,
    USB_GET_DESCRIPTOR    = 6,
    USB_SET_DESCRIPTOR    = 7,
    USB_GET_CONFIGURATION = 8,
    USB_SET_CONFIGURATION = 9
};

typedef struct __attribute__((packed))
{
    uint8_t  bmRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} USB_SetupPacket;

#endif