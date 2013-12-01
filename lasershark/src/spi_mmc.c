/*-----------------------------------------------------------------------
 *      Name:    SPI_MMC.C
 *      Purpose: SPI and SD/MMC command interface Module
 *      Version: V1.03
 *      Copyright (c) 2006 NXP Semiconductor. All rights reserved.
 *---------------------------------------------------------------------*/

#include "type.h"
#include "spi_mmc.h"
#include "ssp.h"
#include "diskio.h" // This file implements the disk access functions needed by the FAT library

#define SD_SSP_NUMBER 1

/* MMC/SD command */
#define CMD0	(0)			/* GO_IDLE_STATE */
#define CMD1	(1)			/* SEND_OP_COND (MMC) */
#define	ACMD41	(0x80+41)	/* SEND_OP_COND (SDC) */
#define CMD8	(8)			/* SEND_IF_COND */
#define CMD9	(9)			/* SEND_CSD */
#define CMD10	(10)		/* SEND_CID */
#define CMD12	(12)		/* STOP_TRANSMISSION */
#define ACMD13	(0x80+13)	/* SD_STATUS (SDC) */
#define CMD16	(16)		/* SET_BLOCKLEN */
#define CMD17	(17)		/* READ_SINGLE_BLOCK */
#define CMD18	(18)		/* READ_MULTIPLE_BLOCK */
#define CMD23	(23)		/* SET_BLOCK_COUNT (MMC) */
#define	ACMD23	(0x80+23)	/* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24	(24)		/* WRITE_BLOCK */
#define CMD25	(25)		/* WRITE_MULTIPLE_BLOCK */
#define CMD32	(32)		/* ERASE_ER_BLK_START */
#define CMD33	(33)		/* ERASE_ER_BLK_END */
#define CMD38	(38)		/* ERASE */
#define CMD55	(55)		/* APP_CMD */
#define CMD58	(58)		/* READ_OCR */

uint8_t MMCCmd[10];
DSTATUS MMCStatus = STA_NOINIT;
static BYTE CardType;			/* Card type flags */

/*-----------------------------------------------------------------------*/
/* Send a command packet to the MMC                                      */
/*-----------------------------------------------------------------------*/

/*-----------------------------------------------------------------------*/
/* Wait for card ready                                                   */
/*-----------------------------------------------------------------------*/

static
int wait_ready (	/* 1:Ready, 0:Timeout */
	UINT wt			/* Timeout [ms] */
)
{
	BYTE d;
	int Timer2;

	Timer2 = wt;
	do {
		d = SPI_ReceiveByte();
		Timer2--;
		/* This loop takes a time. Insert rot_rdq() here for multitask envilonment. */
	} while (d != 0xFF && Timer2);	/* Wait for card goes ready or timeout */

	return (d == 0xFF) ? 1 : 0;
}



/*-----------------------------------------------------------------------*/
/* Deselect card and release SPI                                         */
/*-----------------------------------------------------------------------*/

static
void deselect (void)
{
	SSP0_UNSEL(SSP0_SSEL_SD_BIT);		/* CS = H */
	SPI_ReceiveByte();	/* Dummy clock (force DO hi-z for multiple slave SPI) */
}



/*-----------------------------------------------------------------------*/
/* Select card and wait for ready                                        */
/*-----------------------------------------------------------------------*/

static
int select (void)	/* 1:OK, 0:Timeout */
{
	SSP0_SEL(SSP0_SSEL_SD_BIT);
	SPI_ReceiveByte();	/* Dummy clock (force DO enabled) */

	if (wait_ready(500)) return 1;	/* OK */
	deselect();
	return 0;	/* Timeout */
}

static
BYTE send_cmd (		/* Return value: R1 resp (bit7==1:Failed to send) */
	BYTE cmd,		/* Command index */
	DWORD arg		/* Argument */
)
{
	BYTE n, res;


	if (cmd & 0x80) {	/* Send a CMD55 prior to ACMD<n> */
		cmd &= 0x7F;
		res = send_cmd(CMD55, 0);
		if (res > 1) return res;
	}

	/* Select the card and wait for ready except to stop multiple block read */
	if (cmd != CMD12) {
		deselect();
		if (!select()) return 0xFF;
	}

	/* Send command packet */
	MMCCmd[0] = (0x40 | cmd);				/* Start + command index */
	MMCCmd[1] = ((BYTE)(arg >> 24));		/* Argument[31..24] */
	MMCCmd[2] = ((BYTE)(arg >> 16));		/* Argument[23..16] */
	MMCCmd[3] = ((BYTE)(arg >> 8));			/* Argument[15..8] */
	MMCCmd[4] = ((BYTE)arg);				/* Argument[7..0] */
	n = 0x01;							/* Dummy CRC + Stop */
	if (cmd == CMD0) n = 0x95;			/* Valid CRC for CMD0(0) */
	if (cmd == CMD8) n = 0x87;			/* Valid CRC for CMD8(0x1AA) */
	MMCCmd[5] = (n);
	SSPSend(MMCCmd, 6);

	/* Receive command resp */
	if (cmd == CMD12) SPI_ReceiveByte();	/* Diacard following one byte when CMD12 */
	n = 10;								/* Wait for response (10 bytes max) */
	do
		res = SPI_ReceiveByte();
	while ((res & 0x80) && --n);

	return res;							/* Return received response */
}

/* Receive multiple byte */
static
void rcvr_spi_multi (
	BYTE *buff,		/* Pointer to data buffer */
	UINT btr		/* Number of bytes to receive (16, 64 or 512) */
)
{
	while (btr) {					/* Receive the data block into buffer */
		*buff++ = SPI_ReceiveByte();
		btr--;
	}
}

/*-----------------------------------------------------------------------*/
/* Receive a data packet from the MMC                                    */
/*-----------------------------------------------------------------------*/

static
int rcvr_datablock (	/* 1:OK, 0:Error */
	BYTE *buff,			/* Data buffer */
	UINT btr			/* Data block length (byte) */
)
{
	BYTE token;
	int Timer1;

	Timer1 = 5000;
	do {							/* Wait for DataStart token in timeout of 200ms */
		token = SPI_ReceiveByte();
		Timer1--;
	} while ((token == 0xFF) && Timer1);
	if(Timer1 == 0){
		return 0;
	}

	if(token != 0xFE) return 0;		/* Function fails if invalid DataStart token or timeout */

	rcvr_spi_multi(buff, btr);		/* Store trailing data to the buffer */
	SPI_ReceiveByte(); SPI_ReceiveByte();			/* Discard CRC */

	return 1;						/* Function succeeded */
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber (0..) */
)
{
	BYTE n, cmd, ty, ocr[4];
	int Timer1;

	if (pdrv) return STA_NOINIT;			/* Supports only drive 0 */

	ssp_slow_clock_mode(SD_SSP_NUMBER);
	for (n = 10; n; n--) SPI_ReceiveByte();	/* Send 80 dummy clocks */

	ty = 0;
	if (send_cmd(CMD0, 0) == 1) {			/* Put the card SPI/Idle state */
		Timer1 = 1000;						/* Initialization timeout = 1 sec */
		if (send_cmd(CMD8, 0x1AA) == 1) {	/* SDv2? */
			for (n = 0; n < 4; n++) ocr[n] = SPI_ReceiveByte();	/* Get 32 bit return value of R7 resp */
			if (ocr[2] == 0x01 && ocr[3] == 0xAA) {				/* Is the card supports vcc of 2.7-3.6V? */
				while (Timer1 && send_cmd(ACMD41, 1UL << 30)) {Timer1--;}	/* Wait for end of initialization with ACMD41(HCS) */
				if (Timer1 && send_cmd(CMD58, 0) == 0) {		/* Check CCS bit in the OCR */
					for (n = 0; n < 4; n++) ocr[n] = SPI_ReceiveByte();
					ty = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;	/* Card id SDv2 */
				}
			}
		} else {	/* Not SDv2 card */
			if (send_cmd(ACMD41, 0) <= 1) 	{	/* SDv1 or MMC? */
				ty = CT_SD1; cmd = ACMD41;	/* SDv1 (ACMD41(0)) */
			} else {
				ty = CT_MMC; cmd = CMD1;	/* MMCv3 (CMD1(0)) */
			}
			while (Timer1 && send_cmd(cmd, 0)) {Timer1--;}		/* Wait for end of initialization */
			if (!Timer1 || send_cmd(CMD16, 512) != 0)	/* Set block length: 512 */
				ty = 0;
		}
	}
	CardType = ty;	/* Card type */
	deselect();

	if (ty) {			/* OK */
		ssp_fast_clock_mode(SD_SSP_NUMBER);
		MMCStatus &= ~STA_NOINIT;	/* Clear STA_NOINIT flag */
	} else {			/* Failed */
		MMCStatus = STA_NOINIT;
	}

	return MMCStatus;
}

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber (0..) */
)
{
	// Only drive 0 is supported
	if(pdrv != 0){
		return STA_NOINIT;
	}

	return MMCStatus;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address (LBA) */
	UINT count		/* Number of sectors to read (1..128) */
)
{
	if (pdrv || !count) return RES_PARERR;		/* Check parameter */
	if (MMCStatus & STA_NOINIT) return RES_NOTRDY;	/* Check if drive is ready */

	if (!(CardType & CT_BLOCK)) sector *= 512;	/* LBA ot BA conversion (byte addressing cards) */

	if (count == 1) {	/* Single sector read */
		if ((send_cmd(CMD17, sector) == 0)	/* READ_SINGLE_BLOCK */
			&& rcvr_datablock(buff, 512))
			count = 0;
	}
	else {				/* Multiple sector read */
		if (send_cmd(CMD18, sector) == 0) {	/* READ_MULTIPLE_BLOCK */
			do {
				if (!rcvr_datablock(buff, 512)) break;
				buff += 512;
			} while (--count);
			send_cmd(CMD12, 0);				/* STOP_TRANSMISSION */
		}
	}
	deselect();

	return count ? RES_ERROR : RES_OK;	/* Return result */
}

/***************** MMC get response *******************/
/*
 * Repeatedly reads the MMC until we get the
 * response we want or timeout
 */
int mmc_response(uint8_t response) {
	int count = 0xFFF;
	uint8_t result;

	while (count > 0) {
		result = SPI_ReceiveByte();
		if (result == response) {
			break;
		}
		count--;
	}

	if (count == 0)
		return 1; /* Failure, loop was exited due to timeout */
	else
		return 0; /* Normal, loop was exited before timeout */
}

/***************** MMC wait for write finish *******************/
/*
 * Repeatedly reads the MMC until we get a non-zero value (after
 * a zero value) indicating the write has finished and card is no
 * longer busy.
 *
 */
int mmc_wait_for_write_finish(void) {
	int count = 0xFFFF; /* The delay is set to maximum considering the longest data block length to handle */
	uint8_t result = 0;

	while ((result == 0) && count) {
		result = SPI_ReceiveByte();
		count--;
	}

	if (count == 0)
		return 1; /* Failure, loop was exited due to timeout */
	else
		return 0; /* Normal, loop was exited before timeout */
}
