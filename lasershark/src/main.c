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
#include "ilda.h"

// FAT/File I/O Buffers/Variables
FATFS FatFs;
DIR Dir;
FIL File;
FILINFO Finfo;
#define FILE_READ_SAMPLES 32
#define FILE_READ_BYTES FILE_READ_SAMPLES * 8
BYTE Buff[FILE_READ_BYTES] __attribute__ ((aligned (4))) ;

// Variables for cycling between files
#define MAX_FILES 10
#define MAX_FILENAME_LEN 17
char files[MAX_FILES][MAX_FILENAME_LEN];
int fileCount = 0;
int currentFile = 0;

// ILDA Format Variables
#define FPS 30
#define ILDA_MAX_FRAME_POINTS 128 // We'll see how reasonable this is
typedef enum {ILDA_START, ILDA_START_FRAME, ILDA_CONTINUE_FRAME, ILDA_PLAY_FRAME, ILDA_NEXT_FRAME} ildaState_t;
ildaState_t ildaState;
IldaFile frameInfo;
IldaPoint framePoints[ILDA_MAX_FRAME_POINTS];
int currentIldaPoint = 0;
struct lasershark_stream_format streamBuffer;

// Galvo calibration and tracking
#define MAX_DELTA_SAMPLE (INT16_MAX * 0.25) // Set to the percentage of frame that galvo can move in 1 sample
#define MAX_DELTA_VELOCITY (INT16_MAX * 0.05)
IldaPoint lastPoint;
int duplicateSampleCount = 0;

__inline int velocityChange(int start, int mid, int end){
	return abs((end - mid) - (mid - start));
}

__inline bool endsWith(char* string, char* ending){
	int stringLength = strlen(string);
	int endingLength = strlen(ending);
	return strncmp(&(string[stringLength - endingLength]), ending, endingLength) == 0;
}

uint16_t SCALE_FRAME_U16(int16_t val){
	volatile int temp = (int) val;
	temp -= INT16_MIN; // force positive
	temp *= (DAC124S085_DAC_VAL_MAX * .75);
	temp /= (INT16_MAX - INT16_MIN);
	temp += DAC124S085_DAC_VAL_MAX * .25 / 2;
	return temp;
}

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

typedef enum { INIT_DISK, MOUNT_FS, FIND_FILES, NEXT_FILE, PLAY_RAW_FILE, PLAY_ILDA_FILE } playFileState_t;
playFileState_t playFileState = INIT_DISK;

int main(void) {
	int fs_result;
	uint32_t bytesRead;
	int i;

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
				playFileState = FIND_FILES;
				fileCount = 0;
			}
			break;

		case FIND_FILES:
			// Look at one file per loop
			fs_result = f_readdir(&Dir, &Finfo); // f_readdir reads the next file in the listing
			if ((fs_result != FR_OK)){
				// Something's wrong
				playFileState = MOUNT_FS;
				lasershark_output_enabled = false;
				break;
			}else if(!Finfo.fname[0]){
				// End of directory
				if(fileCount > 0){
					currentFile = 0;
					playFileState = NEXT_FILE;
				}else{
					playFileState = MOUNT_FS;
					lasershark_output_enabled = false;
				}
				break;
			}

			if (Finfo.fattrib & AM_DIR) {
				// TODO: Scan subdirectories (or maybe don't bother?)
				break;
			}

			// It's a real file. See if it's a .ls2 or .ild that we can play
			if(endsWith(Finfo.fname, ".LS2") || endsWith(Finfo.fname, ".ILD")){
				if(fileCount < MAX_FILES && strlen(Finfo.fname) < MAX_FILENAME_LEN){
					strncpy(files[fileCount], Finfo.fname, MAX_FILENAME_LEN);
					fileCount++;
				}
			}
			break;

		case NEXT_FILE:
			// Yep. Get ready to play it
			currentFile = 0;
			fs_result = f_open(&File, files[currentFile], FA_READ);
			if(fs_result != 0){
				// Something wrong
				playFileState = MOUNT_FS;
				break;
			}

			if(endsWith(files[currentFile], ".LS2")){
				// Raw format
				// To start playing a raw file, read in the sample
				// rate (first byte) and set accordingly
				f_read(&File, Buff, 1, &bytesRead);
				lasershark_set_ilda_rate(Buff[0] * 1000);
				playFileState = PLAY_RAW_FILE;
			}else{
				// ILDA format
				//
				ildaState = ILDA_START;
				lasershark_set_ilda_rate(16000);
				playFileState = PLAY_ILDA_FILE;
			}

			currentFile++;
			if(currentFile >= fileCount){
				currentFile = 0;
			}
			break;

		case PLAY_RAW_FILE:
			if(lasershark_get_empty_sample_count() > FILE_READ_SAMPLES){
				// Room for more data
				fs_result = f_read(&File, Buff, FILE_READ_BYTES, &bytesRead);
				if(fs_result != 0){
					// Some read error
					playFileState = MOUNT_FS;
					break;
				}

				lasershark_process_data(Buff, bytesRead);
				if(lasershark_get_empty_sample_count() <= FILE_READ_SAMPLES){
					// Buffer is full. Start playing
					lasershark_output_enabled = true;
				}

				if(bytesRead < FILE_READ_BYTES){
					// EOF
					playFileState = NEXT_FILE;
				}
			}
			break;

		case PLAY_ILDA_FILE:
			switch(ildaState){
			case ILDA_START:
				ildaState = ILDA_START_FRAME;
				break;

			case ILDA_START_FRAME:
				switch(olLoadIldaFrame(&File, &frameInfo, framePoints, ILDA_MAX_FRAME_POINTS)){
				case FILE_ERROR:
				case FRAME_SKIPPED:
					// Can't use file anymore
					playFileState = NEXT_FILE;
					break;
				case FRAME_LOADED:
					ildaState = ILDA_PLAY_FRAME; // Next read will error
					currentIldaPoint = 0; // Just to make a nice breakpoint
					break;
				}
				break;

			case ILDA_CONTINUE_FRAME:
				switch(olLoadIldaPoints(&File, &frameInfo, framePoints, ILDA_MAX_FRAME_POINTS)){
				case FILE_ERROR:
				case FRAME_SKIPPED:
					// File can't be read
					playFileState = NEXT_FILE;
					break;
				case FRAME_LOADED:
					ildaState = ILDA_PLAY_FRAME;
					currentIldaPoint = 0;
				}
				break;

			case ILDA_PLAY_FRAME:

				i = 0; // Only allow a few to be loaded each time around
				while(lasershark_get_empty_sample_count() > 1 && currentIldaPoint < frameInfo.loadedPointCount && i < 200){
					// Load the buffer with samples in the stream format
					streamBuffer.x = SCALE_FRAME_U16(framePoints[currentIldaPoint].x);
					streamBuffer.y = SCALE_FRAME_U16(framePoints[currentIldaPoint].y);
					streamBuffer.a = framePoints[currentIldaPoint].color == 0 ? DAC124S085_DAC_VAL_MIN : DAC124S085_DAC_VAL_MAX; // Todo: Color scale
					streamBuffer.b = streamBuffer.a;

					lasershark_add_sample(&streamBuffer);

					i++;
					currentIldaPoint++;
				}

				if(lasershark_get_empty_sample_count() < 10){
					lasershark_output_enabled = true;
				}

				if(currentIldaPoint >= frameInfo.loadedPointCount){
					currentIldaPoint = 0;
					//frameInfo.displayCount++;
					//if(frameInfo.displayCount > 2){
						ildaState = ILDA_NEXT_FRAME;
					//}
				}
				break;

			case ILDA_NEXT_FRAME:
				if(frameInfo.loadedStartPoint + frameInfo.loadedPointCount >= frameInfo.totalPoints){
					// All points were loaded last round. Next frame
					ildaState = ILDA_START_FRAME;
				}else{
					ildaState = ILDA_CONTINUE_FRAME;
				}
				break;
			}
		}
	}
	return 0;
}
