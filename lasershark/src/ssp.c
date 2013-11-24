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

	LPC_SYSCON->PRESETCTRL |= (0x1 << 0);
	LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 11);
	LPC_SYSCON->SSP0CLKDIV = 0x01;			/* Divided by 1 (PCKL) */
	LPC_IOCON->PIO0_8 &= ~0x07; /*  SSP I/O config */
	LPC_IOCON->PIO0_8 |= 0x01; /* SSP MISO */
	LPC_IOCON->PIO0_9 &= ~0x07;
	LPC_IOCON->PIO0_9 |= 0x01; /* SSP MOSI */
#ifdef __JTAG_DISABLED
	LPC_IOCON->SCKLOC = 0x00;
	LPC_IOCON->JTAG_TCK_PIO0_10 &= ~0x07;
	LPC_IOCON->JTAG_TCK_PIO0_10 |= 0x02; /* SSP CLK */
#endif

#if 1
	LPC_IOCON->PIO1_29 = 0x01; /* SCK0 */
#else
	LPC_IOCON->SCKLOC = 0x02;
	LPC_IOCON->PIO0_6 = 0x02; /* P0.6 function 2 is SSP clock, need to combined
	 with IOCONSCKLOC register setting */
#endif

#if USE_CS
	LPC_IOCON->PIO0_2 &= ~0x07;
	LPC_IOCON->PIO0_2 |= 0x01; /* SSP SSEL */
#else
	LPC_IOCON->PIO0_2 = 0; /* SSP SSEL is a GPIO pin */
	LPC_IOCON->PIO0_7 = 0; /* SSEL 1 is a GPIO */
	/* port0, bit 2 is set to GPIO output and high */
	GPIOSetDir( SSP0_PORT, SSP0_SSEL_DAC_BIT, 1 );
	GPIOSetBitValue( SSP0_PORT, SSP0_SSEL_DAC_BIT, 1 );
	GPIOSetDir( SSP0_PORT, SSP0_SSEL_SD_BIT, 1 );
	GPIOSetBitValue( SSP0_PORT, SSP0_SSEL_SD_BIT, 1 );
#endif

	// dss=8bit, frame format = spi, CPOL = 0, cpha = 0, SCR is 7
	LPC_SSP0->CR0 = 0x7 << 0 | 0x0 << 4 | 0 << 6 | 0 << 7 | 0x0F << 8;
	/* SSPCPSR clock prescale register, master mode, minimum divisor is 0x02 */
	LPC_SSP0->CPSR = 40; /* CPSDVSR */

	for (i = 0; i < FIFOSIZE; i++)
	{
		Dummy = LPC_SSP0->DR; /* clear the RxFIFO */
	}

	/* Device select as master, SSP Enabled */
	/* Master mode */
	LPC_SSP0->CR1 = SSPCR1_SSE;

	/* Set SSPINMS registers to enable interrupts */
	/* enable all error related interrupts */
	//LPC_SSP0->IMSC = SSPIMSC_RORIM | SSPIMSC_RTIM;
	return;
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
void SSPSend(uint8_t *buf, uint32_t Length) {
	uint32_t i;
	uint8_t Dummy = Dummy;

	for (i = 0; i < Length; i++) {
		/* Move on only if NOT busy and TX FIFO not full. */
		while ((LPC_SSP0->SR & (SSPSR_TNF | SSPSR_BSY)) != SSPSR_TNF)
			;
		LPC_SSP0->DR = *buf;
		buf++;
#if !LOOPBACK_MODE
		while ((LPC_SSP0->SR & (SSPSR_BSY | SSPSR_RNE)) != SSPSR_RNE)
			;
		/* Whenever a byte is written, MISO FIFO counter increments, Clear FIFO
		 on MISO. Otherwise, when SSP0Receive() is called, previous data byte
		 is left in the FIFO. */
		Dummy = LPC_SSP0->DR;
#else
		/* Wait until the Busy bit is cleared. */
		while ( LPC_SSP->SR & SSPSR_BSY );
#endif
	}
	return;
}

/*****************************************************************************
 ** Function name:		SSPSend16
 **
 ** Descriptions:		Send a block of data to the SSP port, the
 **						first parameter is the buffer pointer, the 2nd
 **						parameter is the block length.
 **
 ** parameters:			buffer pointer, and the block length
 ** Returned value:		None
 **
 *****************************************************************************/
void SSPSend16(uint16_t *buf, uint32_t Length) {
	uint32_t i;
	uint8_t Dummy = Dummy;

	for (i = 0; i < Length; i++) {
		/* Move on only if NOT busy and TX FIFO not full. */
		while ((LPC_SSP0->SR & (SSPSR_TNF | SSPSR_BSY)) != SSPSR_TNF)
			;
		LPC_SSP0->DR = *buf;
		buf++;
#if !LOOPBACK_MODE
		while ((LPC_SSP0->SR & (SSPSR_BSY | SSPSR_RNE)) != SSPSR_RNE)
			;
		/* Whenever a byte is written, MISO FIFO counter increments, Clear FIFO
		 on MISO. Otherwise, when SSP0Receive() is called, previous data byte
		 is left in the FIFO. */
		Dummy = LPC_SSP0->DR;
#else
//		/* Wait until the Busy bit is cleared. */
//		while ( LPC_SSP->SR & SSPSR_BSY );
#endif
	}
	return;
}

void SSPSendC16(uint16_t c) {
	uint8_t Dummy = Dummy;

	/* Move on only if NOT busy and TX FIFO not full. */
	while ((LPC_SSP0->SR & (SSPSR_TNF | SSPSR_BSY)) != SSPSR_TNF);
	LPC_SSP0->DR = c;
#if !LOOPBACK_MODE
	while (SSP0_BUSY());
	/* Whenever a byte is written, MISO FIFO counter increments, Clear FIFO
	 on MISO. Otherwise, when SSP0Receive() is called, previous data byte
	 is left in the FIFO. */
	Dummy = LPC_SSP0->DR;
#else
//	/* Wait until the Busy bit is cleared. */
//	while ( LPC_SSP->SR & SSPSR_BSY );
#endif

	return;
}

/*****************************************************************************
 ** Function name:		SSPReceive
 ** Descriptions:		the module will receive a block of data from
 **						the SSP, the 2nd parameter is the block
 **						length.
 ** parameters:			buffer pointer, and block length
 ** Returned value:		None
 **
 *****************************************************************************/
void SSPReceive(uint8_t *buf, uint32_t Length) {
	uint32_t i;

	for (i = 0; i < Length; i++) {
		*buf = SPI_ReceiveByte();
		buf++;

	}
	return;
}

/*
 * SPI Receive Byte, receive one byte only, return Data byte
 * used a lot to check the status.
 */
uint8_t SPI_ReceiveByte(void) {
	uint8_t data;

	/* write dummy byte out to generate clock, then read data from
	 MISO */
	LPC_SSP0->DR = 0xFF;

	/* Wait until the Busy bit is cleared */
	while (SSP0_BUSY());

	data = LPC_SSP0->DR;
	if(data != 255){
		return data;
	}else{
		return 66;
	}
	//return (data);
}

/******************************************************************************
 **                            End Of File
 ******************************************************************************/

