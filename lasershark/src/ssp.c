/*****************************************************************************
 *   ssp.c:  SSP C file for NXP LPC13xx Family Microprocessors
 *
 *   Copyright(C) 2008, NXP Semiconductor
 *   All rights reserved.
 *
 *   History
 *   2012.10.15 ver 1.1 Tweaked SSP settings for project <nelsonjm@macpod.net>
 *   2008.07.20  ver 1.00    Preliminary version, first Release
 *
 *****************************************************************************/
#include "LPC13Uxx.h"			/* LPC13xx Peripheral Registers */
#include "gpio.h"
#include "ssp.h"

/*****************************************************************************
 ** Function name:		SSPInit
 **
 ** Descriptions:		SSP port initialization routine
 **
 ** parameters:			None
 ** Returned value:		None
 **
 *****************************************************************************/
void SSPInit(void) {
	uint8_t i, Dummy = Dummy;

	LPC_SYSCON->PRESETCTRL |= 0x05; // Reset SSP0 and SSP1
	LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 11) | (1 << 18); // Turn on clock to SSP0 and SSP1
	LPC_SYSCON->SSP0CLKDIV = 0x01;			/* Divided by 1 (PCKL) */
	LPC_SYSCON->SSP1CLKDIV = 0x01;			/* Divided by 1 (PCKL) */

	// SSP0 I/O Config
	LPC_IOCON->PIO0_8 =  0x00; //SSP0 MISO (Not used [0x01 to select])
	LPC_IOCON->PIO0_9 =  0x01; //SSP0 MOSI
	LPC_IOCON->PIO1_29 = 0x01; //SSP0 SCK
	LPC_IOCON->PIO0_2 =  0x01; //SSP0 SSEL - It's safe to let the hardware control this

	// SSP1 I/O Config
	LPC_IOCON->PIO1_21 = 0x02; // SSP1 MISO
	LPC_IOCON->PIO0_21 = 0x02; // SSP1 MOSI
	LPC_IOCON->PIO1_20 = 0x02; // SSP1 SCK
	LPC_IOCON->PIO0_7 = 0; /* SSEL1 is a GPIO */
	GPIOSetDir( SSP0_PORT, SSP0_SSEL_SD_BIT, 1 );
	GPIOSetBitValue( SSP0_PORT, SSP0_SSEL_SD_BIT, 1 );

	LPC_SSP0->CR0 = 0; // Clear out
	LPC_SSP1->CR0 = 0; // Clear out

	ssp_set_bits(0, 16); // DAC SPI uses 16 bit transfers
	ssp_set_bits(1, 8); // SD card uses 8 bit transfers

	ssp_fast_clock_mode(0); // DAC SPI always runs fast
	ssp_slow_clock_mode(1); // SD SPI starts slow

	for (i = 0; i < FIFOSIZE; i++)
	{
		Dummy = LPC_SSP0->DR; /* clear the RxFIFO */
		Dummy = LPC_SSP1->DR; /* clear the RxFIFO */
	}

	return;
}

__inline LPC_SSP0_Type* getSSP(int sspNumber){
	if(sspNumber == 1){
		// These are actually the same type
		return (LPC_SSP0_Type*) LPC_SSP1;
	}else{
		return LPC_SSP0;
	}
}

void ssp_fast_clock_mode(int sspNumber){
	LPC_SSP0_Type* SSP = getSSP(sspNumber);

	// Disable SPI
	SSP->CR1 = 0;

	SSP->CR0 &= ~(0xFF00);
	SSP->CR0 |= 0x0100;
	SSP->CPSR = 2; // Clock prescale (minimum value is 2)

	// Re-enable
	SSP->CR1 = SSPCR1_SSE;
}

void ssp_slow_clock_mode(int sspNumber){
	LPC_SSP0_Type* SSP = getSSP(sspNumber);

	// Disable SPI
	SSP->CR1 = 0;

	SSP->CR0 &= ~(0xFF00);
	SSP->CR0 |= 0x0800;
	SSP->CPSR = 40; // Clock prescale (minimum value is 2)

	// Re-enable
	SSP->CR1 = SSPCR1_SSE;
}

void ssp_set_bits(int sspNumber, int bits){
	LPC_SSP0_Type* SSP = getSSP(sspNumber);

	if(bits < 4 || bits > 16){
		return;
	}

	// Disable SPI
	SSP->CR1 = 0;

	SSP->CR0 &= ~(0x0F); // Clear bit settings
	SSP->CR0 |= ((bits - 1) & 0x0F);

	// Re-enable
	SSP->CR1 = SSPCR1_SSE;
}

/*****************************************************************************
 ** Function name:		SSPSend
 **
 ** Descriptions:		Send a block of data to the SSP port, the
 **						first parameter is the buffer pointer, the 2nd
 **						parameter is the block length.
 **
 ** parameters:			buffer pointer, and the block length
 ** Returned value:		None
 **
 *****************************************************************************/
__inline void SSPSend(uint8_t *buf, uint32_t Length) { // 8-bit functions are for SD only
	uint32_t i;
	uint8_t Dummy = Dummy;

	for (i = 0; i < Length; i++) {
		/* Move on only if NOT busy and TX FIFO not full. */
		while ((LPC_SSP1->SR & (SSPSR_TNF | SSPSR_BSY)) != SSPSR_TNF)
			;
		LPC_SSP1->DR = *buf;
		buf++;
		while ((LPC_SSP1->SR & (SSPSR_BSY | SSPSR_RNE)) != SSPSR_RNE)
			;
		/* Whenever a byte is written, MISO FIFO counter increments, Clear FIFO
		 on MISO. Otherwise, when SSP0Receive() is called, previous data byte
		 is left in the FIFO. */
		Dummy = LPC_SSP1->DR;
	}
	return;
}

__inline void SSPSendC16(uint16_t c) { // 16-bit functions are for DAC only
	uint8_t Dummy = Dummy;

	/* Move on only if NOT busy and TX FIFO not full. */
	while ((LPC_SSP0->SR & (SSPSR_TNF | SSPSR_BSY)) != SSPSR_TNF);
	LPC_SSP0->DR = c;

	// We have no interest in reading data and this function checks
	// busy, so we can just return

	//while (SSP_BUSY(LPC_SSP0));
	/* Whenever a byte is written, MISO FIFO counter increments, Clear FIFO
	 on MISO. Otherwise, when SSP0Receive() is called, previous data byte
	 is left in the FIFO. */
	//Dummy = LPC_SSP0->DR;

	return;
}

/*
 * SPI Receive Byte, receive one byte only, return Data byte
 * used a lot to check the status.
 */
__inline uint8_t SPI_ReceiveByte() { // 8-bit functions are for SD only
	uint8_t data;

	/* write dummy byte out to generate clock, then read data from
	 MISO */
	LPC_SSP1->DR = 0xFF;

	/* Wait until the Busy bit is cleared */
	while (SSP_BUSY(LPC_SSP1));

	data = LPC_SSP1->DR;
	return data;
}

/******************************************************************************
 **                            End Of File
 ******************************************************************************/

