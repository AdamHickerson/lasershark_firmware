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
#include <string.h>
#include "config.h"
#include "gpio.h"
#include "usbhw.h"
#include "mw_usbd.h"
#include "mw_usbd_desc.h"
#include "type.h"
#include "lasershark.h"
#include "ssp.h"
#include "spi_mmc.h"
#include "ff.h"
#include "diskio.h"
#include "dac124s085.h"

FATFS FatFs;
DIR Dir;
FIL File;
FILINFO Finfo;
#define FILE_READ_SAMPLES 32
#define FILE_READ_BYTES FILE_READ_SAMPLES * 8
BYTE Buff[FILE_READ_BYTES] __attribute__ ((aligned (4))) ;

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

void main_init(void) {
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
	//USBIOClkConfig();

	//usb_populate_serialno(); // Populate the devices serial number
	// USB Initialization
	//USB_Init();

	// Make USB a lower priority than the timer used for output.
	NVIC_SetPriority(USB_IRQ_IRQn, 2);
}

enum { INIT_DISK, MOUNT_FS, FIND_FILE, PLAY_FILE };
int playFileState = INIT_DISK;

int main(void) {
	int fs_result;
	uint32_t bytesRead;

	main_init();
	//watchdog_init();

	while (1) {
		watchdog_feed();

		switch(playFileState){
		default:
		case INIT_DISK:
			// TODO: Card insert detect
			playFileState = MOUNT_FS;
			break;

		case MOUNT_FS:
			fs_result = f_mount(&FatFs, " ", 1);
			if(fs_result == 0){
				f_opendir(&Dir, "/");
				playFileState = FIND_FILE;
			}
			break;

		case FIND_FILE:
			// Look at one file per loop
			fs_result = f_readdir(&Dir, &Finfo); // f_readdir reads the next file in the listing
			if ((fs_result != FR_OK)){
				// Something's wrong
				playFileState = MOUNT_FS;
				lasershark_output_enabled = false;
				break;
			}else if(!Finfo.fname[0]){
				// End of directory. Start from the top
				f_opendir(&Dir, "/");
				break;
			}

			if (Finfo.fattrib & AM_DIR) {
				// TODO: Scan subdirectories (or maybe don't bother?)
				break;
			}

			// It's a real file. See if it's a .lsr that we can play
			if(strncmp(&(Finfo.fname[strlen(Finfo.fname) - 4]), ".LS2", 4) == 0){
				// Yep. Get ready to play it
				f_open(&File, Finfo.fname, FA_READ);

				// The first byte in the file specifies the sample rate
				// in kHz
				f_read(&File, Buff, 1, &bytesRead);
				lasershark_set_ilda_rate(Buff[0] * 1000);

				playFileState = PLAY_FILE;
			}
			break;
		case PLAY_FILE:
			if(lasershark_get_empty_sample_count() > FILE_READ_SAMPLES){
				// Room for more data
				fs_result = f_read(&File, Buff, FILE_READ_BYTES, &bytesRead);
				if(fs_result != 0){
					// Some read error
					playFileState = FIND_FILE;
					break;
				}

				lasershark_process_data(Buff, bytesRead);
				if(lasershark_get_empty_sample_count() <= FILE_READ_SAMPLES){
					// Buffer is full. Start playing
					lasershark_output_enabled = true;
				}

				if(bytesRead < FILE_READ_BYTES){
					// EOF
					playFileState = FIND_FILE;
				}
			}
		}
	}
	return 0;
}
