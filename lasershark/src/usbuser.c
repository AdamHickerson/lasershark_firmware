/*
 usbuser.c - Lasershark firmware.
 Copyright (C) 2012 Jeffrey Nelson <nelsonjm@macpod.net>

 This file is part of Lasershark's Firmware.

 Lasershark is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.

 Lasershark is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Lasershark. If not, see <http://www.gnu.org/licenses/>.
 */

#include "usbuser.h"

#include "type.h"

#include "lasershark.h"
#include "gpio.h"
#include "config.h"

ErrorCode_t USB_EndPoint1(USBD_HANDLE_T hUsb, void* data, uint32_t event);
ErrorCode_t USB_EndPoint2(USBD_HANDLE_T hUsb, void* data, uint32_t event);
ErrorCode_t USB_EndPoint3(USBD_HANDLE_T hUsb, void* data, uint32_t event);
ErrorCode_t USB_EndPoint4(USBD_HANDLE_T hUsb, void* data, uint32_t event);

ErrorCode_t USB_InitUser(void){
	ErrorCode_t err;

	err = pUsbApi->core->RegisterEpHandler(hUsb, (1 << 1) + 1, USB_EndPoint1, NULL); // Endpoint 1 In
	if(err != LPC_OK){
		return err;
	}
	err = pUsbApi->core->RegisterEpHandler(hUsb, (1 << 1), USB_EndPoint1, NULL); // Endpoint 1 Out
	if(err != LPC_OK){
		return err;
	}
	err = pUsbApi->core->RegisterEpHandler(hUsb, (3 << 1), USB_EndPoint3, NULL); // Endpiont 3 Out

	return err;
}

/*
 *  USB Endpoint 1 Event Callback
 *   Called automatically on USB Endpoint 1 Event
 *    Parameter:       event
 */
ErrorCode_t USB_EndPoint1(USBD_HANDLE_T hUsb, void* data, uint32_t event) {
	switch (event) {
	case USB_EVT_OUT:
		pUsbApi->hw->ReadEP(hUsb, USB_ENDPOINT_OUT(1), OUT1Packet);
		lasershark_process_command();
		pUsbApi->hw->WriteEP(hUsb, USB_ENDPOINT_IN(1), IN1Packet, 64);
		break;
	case USB_EVT_IN:
		break;
	}

	return LPC_OK;
}

/*
 *  USB Endpoint 2 Event Callback
 *   Called automatically on USB Endpoint 2 Event
 *    Parameter:       event
 */
ErrorCode_t USB_EndPoint2(USBD_HANDLE_T hUsb, void* data, uint32_t event) {
	switch (event) {
	case USB_EVT_IN:
		break;
	}

	return LPC_OK;
}

/*
 *  USB Endpoint 3 Event Callback
 *   Called automatically on USB Endpoint 3 Event
 *    Parameter:       event
 */
ErrorCode_t USB_EndPoint3(USBD_HANDLE_T hUsb, void* data, uint32_t event) {
	uint32_t cnt;
	unsigned char packet[LASERSHARK_USB_DATA_BULK_SIZE*4];

	switch (event) {
	case USB_EVT_OUT:
		//while (1) {
		//	if (LASERSHARK_USB_DATA_BULK_SIZE <= lasershark_get_empty_sample_count()) {
		//		break;
		//	}
		//}
		//LPC_USB->Ctrl = ((USB_ENDPOINT_OUT(3) & 0x0F) << 2) | CTRL_RD_EN; // enable read
		// 3 clock cycles to fetch the packet length from RAM.
		asm("nop");
		asm("nop");
		asm("nop");
		asm("nop");
		
		//cnt = pUsbApi->hw->ReadEP(hUsb, USB_CDC_EP_BULK_OUT, pVcom->rxBuf);
		cnt = pUsbApi->hw->ReadEP(hUsb, 3, packet);

		//if ((cnt = LPC_USB->RxPLen) & PKT_DV) { // We have data...
		//	cnt &= PKT_LNGTH_MASK; // Get length in bytes
			lasershark_process_data(packet, cnt);
		//	LPC_USB->Ctrl = 0;
		//}
		//LPC_USB->Ctrl = 0; // Disable read mode.. do this if you ever want to see a USB packet again
	    //WrCmdEP(USB_ENDPOINT_OUT(3), CMD_CLR_BUF);
		break;
	}

	return LPC_OK;
}

/*
 *  USB Endpoint 4 Event Callback
 *   Called automatically on USB Endpoint 4 Event
 *    Parameter:       event
 */
ErrorCode_t USB_EndPoint4(USBD_HANDLE_T hUsb, void* data, uint32_t event) {
	switch (event) {
	case USB_EVT_IN:
		break;
	}

	return LPC_OK;
}

