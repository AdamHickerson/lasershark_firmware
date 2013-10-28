#include <string.h>
#include "usbhw.h"
#include "LPC13Uxx.h"
#include "mw_usbd_desc.h"
#include "power_api.h"
#include "usbuser.h"

USBD_API_T* pUsbApi;
USBD_HANDLE_T hUsb;

void USB_IRQHandler(void)
{
  pUsbApi->hw->ISR(hUsb);
}

/*
 *    USB and IO Clock configuration only.
 *    The same as call PeriClkIOInit(IOCON_USB);
 *    The purpose is to reduce the code space for
 *    overall USB project and reserve code space for
 *    USB debugging.
 *    Parameters:      None
 *    Return Value:    None
 */
void USBIOClkConfig( void )
{
  /* Enable AHB clock to the GPIO domain. */
  LPC_SYSCON->SYSAHBCLKCTRL |= (1<<6);

  /* Enable AHB clock to the USB block and USB RAM. */
  LPC_SYSCON->SYSAHBCLKCTRL |= ((0x1<<14)|(0x1<<27));

  LPC_IOCON->PIO0_1 &= ~0x07;
  LPC_IOCON->PIO0_1 |= 0x01;		/* CLK OUT */

  /* Pull-down is needed, or internally, VBUS will be floating. This is to
  address the wrong status in VBUSDebouncing bit in CmdStatus register. */
  LPC_IOCON->PIO0_3   &= ~0x1F;
  LPC_IOCON->PIO0_3   |= 0x01;		/* Secondary function VBUS */
  LPC_IOCON->PIO0_6   &= ~0x07;
  LPC_IOCON->PIO0_6   |= 0x01;		/* Secondary function SoftConn */
  return;
}

void USB_Init (void) {
  ErrorCode_t ret;
  USBD_API_INIT_PARAM_T usb_param;
  USB_CORE_DESCS_T desc;

  /* get USB API table pointer */
  pUsbApi = (USBD_API_T*)((*(ROM **)(0x1FFF1FF8))->pUSBD);

  /* initialize call back structures */
  memset((void*)&usb_param, 0, sizeof(USBD_API_INIT_PARAM_T));
  usb_param.usb_reg_base = LPC_USB_BASE;
  usb_param.mem_base = 0x10000800;
  usb_param.mem_size = 0x00001000;
  usb_param.max_num_ep = 5;

  /* Initialize Descriptor pointers */
  memset((void*)&desc, 0, sizeof(USB_CORE_DESCS_T));
  desc.device_desc = (uint8_t *)&USB_DeviceDescriptor[0];
  desc.string_desc = (uint8_t *)&USB_StringDescriptor[0];
  desc.full_speed_desc = (uint8_t *)&USB_ConfigDescriptor[0];
  desc.high_speed_desc = (uint8_t *)&USB_ConfigDescriptor[0];

  /* USB Initialization */
  ret = pUsbApi->hw->Init(&hUsb, &desc, &usb_param);

  if (ret == LPC_OK) {
	ret = USB_InitUser();
	if (ret == LPC_OK) {
	  NVIC_EnableIRQ(USB_IRQ_IRQn); //  enable USB interrrupts
	  /* now connect */
	  pUsbApi->hw->Connect(hUsb, 1);
	}
  }

  return;
}
