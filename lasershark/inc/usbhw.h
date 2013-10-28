#include "mw_usbd_rom_api.h"

#ifndef __USB_H_LSHRK_
#define __USB_H_LSRHK_

extern USBD_API_T* pUsbApi;
extern USBD_HANDLE_T hUsb;

void USBIOClkConfig( void );
void USB_Init (void);

#endif
