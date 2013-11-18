/*
 main.c - Lasershark firmware.
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

#ifdef __USE_CMSIS
#include "LPC13Uxx.h"
#endif

#include <cr_section_macros.h>
#include <NXP/crp.h>
#include <stdbool.h>
#include "config.h"
#include "gpio.h"
#include "usbhw.h"
#include "mw_usbd.h"
#include "mw_usbd_desc.h"
#include "type.h"
#include "lasershark.h"
#include "ssp.h"
#include "spi_mmc.h"

bool updateDAC = false;

// Variable to store CRP value in. Will be placed automatically
// by the linker when "Enable Code Read Protect" selected.
// See crp.h header for more information
__CRP const unsigned int CRP_WORD = CRP_NO_CRP;

// Set this to zero if you are debugging.
#define WATCHDOG_ENABLED 1

#define WDEN (0x1<<0)
#define WDRESET (0x1<<1)

#if (WATCHDOG_ENABLED)
void watchdog_feed() {
	LPC_WWDT->FEED = 0xAA;
	LPC_WWDT->FEED = 0x55;
}

void watchdog_init() {
	LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 15); // Power on WTD peripheral
	//	LPC_SYSCON->CLKOUTCLKSEL = 0x02; // Use WDTOSC for CLKOUT pin

	LPC_SYSCON->WDTOSCCTRL = 0x1 << 5 | 0xF; // FREQSEL = 1.4Mhz, 64 =  ~9.375kHz
	LPC_SYSCON->PDRUNCFG &= ~(0x1 << 6); // Power WDTOSC

	LPC_WWDT->CLKSEL = 0x01; // Use WDTOSC as the WTD clock source

	NVIC_EnableIRQ(WDT_IRQn);
	LPC_WWDT->TC = 256; // Delay = <276 (minimum of 256, max of 2^24)>*4 / 9.375khz(WDTCLK) =0.10922666666 s
	LPC_WWDT->MOD = WDEN | WDRESET; // Cause reset to occur when WDT hits.

	watchdog_feed();
}
#endif

int main_init(void) {
	/* Basic chip initialization is taken care of in SystemInit() called
	 * from the startup code. SystemInit() and chip settings are defined
	 * in the CMSIS system_<part family>.c file.
	 */
	SystemCoreClockUpdate ();

	/* Initialize GPIO (sets up clock) */
	GPIOInit();

	/* Set pin C, INTL A and INTL B function and state ASAP!
	 * If pin C and INTL A are set as inputs, it will cause the amplifiers to output > 10v
	 * which is NOT GOOD! There is no real way to avoid this issue unfortunately other than
	 * to insist the board is not connected when being programmed.
	 */
	/* Make pin functions digital IOs, pulldowns enabled, ADMODE=digital. */
	LPC_IOCON->PIO1_1 = (1 << 0) | (0x1 << 3) | (1 << 7); // C
	LPC_IOCON->PIO1_2 = (1 << 0) | (0x1 << 3) | (1 << 7); // INTL A
	LPC_IOCON->PIO1_0 = (1 << 0) | (0x1 << 3) | (1 << 7); // INTL B

	/* Set LED port pin to output */
	GPIOSetDir(LED_PORT, USR1_LED_BIT, 1);
	GPIOSetDir(LED_PORT, USR2_LED_BIT, 0); // TODO: Had to re-use this pin so set to input

	/* Enable IOCON blocks for io pin multiplexing.*/
	LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 16);

	// Set initial state of lasershark ASAP!
	lasershark_init();

	// Turn on USB
	USBIOClkConfig();

	usb_populate_serialno(); // Populate the devices serial number
	// USB Initialization
	USB_Init();

	// Make USB a lower priority than the timer used for output.
	NVIC_SetPriority(USB_IRQ_IRQn, 2);
}

void CT32B1_IRQHandler(void) {
	LPC_CT32B1->IR = 1; /* clear interrupt flag */
	updateDAC = true;
}

int main(void) {
	bool sdReady = false;
	bool readBack = false;
	int block = 0;
	int i;

	main_init();
	//watchdog_init();

	while (1) {
		// Wait until it's time to update the DACs
		while(!updateDAC);
		updateDAC = false;
		//Lasershark_Update_DAC();

		// Feed the watchdog before our next wait state
		watchdog_feed();

		// SD card shares SPI bus with the DACs. Since we just
		// wrote them, we have a little time to read the SD
		// before we have to update the DACs again.
		while(SSP0_BUSY());

		if(!sdReady){
			if(mmc_init() == 0){
				sdReady = true;
			}
		}else{
			if(!readBack){
				mmc_write_block(block);
			}else{
				mmc_read_block(block);

				for ( i = 0; i < MMC_DATA_SIZE; i++ ) /* Validate */ {
				  if ( MMCRDData[i] != MMCWRData[i] )
				  {
					  sdReady = false;
				  }
				}
				for ( i = 0; i < MMC_DATA_SIZE; i++ ) /* clear read buffer */ MMCRDData[i] = 0x00;

				readBack = false;
				block++;
				if(block > MAX_BLOCK_NUM){
					block = 0;
				}
			}
		}
	}
	return 0;
}
