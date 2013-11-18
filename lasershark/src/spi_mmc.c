/*-----------------------------------------------------------------------
 *      Name:    SPI_MMC.C
 *      Purpose: SPI and SD/MMC command interface Module
 *      Version: V1.03
 *      Copyright (c) 2006 NXP Semiconductor. All rights reserved.
 *---------------------------------------------------------------------*/

#include "type.h"
#include "spi_mmc.h"
#include "ssp.h"

uint8_t MMCWRData[MMC_DATA_SIZE];
uint8_t MMCRDData[MMC_DATA_SIZE];
uint8_t MMCCmd[MMC_CMD_SIZE];
uint8_t MMCStatus = 0;

/************************** MMC Init *********************************/
/*
 * Initialises the MMC into SPI mode and sets block size(512), returns
 * 0 on success
 *
 */
int mmc_init() {
	int i;

	/* Generate a data pattern for write block */
	for (i = 0; i < MMC_DATA_SIZE; i++) {
		MMCWRData[i] = i;
	}
	MMCStatus = 0;

	SSP0_UNSEL(SSP0_SSEL_SD_BIT);

	/* initialise the MMC card into SPI mode by sending 80 clks on */
	/* Use MMCRDData as a temporary buffer for SSPSend() */
	for (i = 0; i < 10; i++) {
		MMCRDData[i] = 0xFF;
	}
	SSPSend(MMCRDData, 10);

	SSP0_SEL(SSP0_SSEL_SD_BIT);
	/* send CMD0(RESET or GO_IDLE_STATE) command, all the arguments
	 are 0x00 for the reset command, precalculated checksum */
	MMCCmd[0] = 0x40;
	MMCCmd[1] = 0x00;
	MMCCmd[2] = 0x00;
	MMCCmd[3] = 0x00;
	MMCCmd[4] = 0x00;
	MMCCmd[5] = 0x95;
	SSPSend(MMCCmd, MMC_CMD_SIZE);
	/* if = 1 then there was a timeout waiting for 0x01 from the MMC */
	if (mmc_response(0x01) == 1) {
		MMCStatus = IDLE_STATE_TIMEOUT;
		SSP0_UNSEL(SSP0_SSEL_SD_BIT);
		return MMCStatus;
	}

	/* Send some dummy clocks after GO_IDLE_STATE */
	SSP0_UNSEL(SSP0_SSEL_SD_BIT);
	SPI_ReceiveByte();

	SSP0_SEL(SSP0_SSEL_SD_BIT);
	/* must keep sending command until zero response ia back. */
	i = MAX_TIMEOUT;
	do {
		/* send mmc CMD1(SEND_OP_COND) to bring out of idle state */
		/* all the arguments are 0x00 for command one */
		MMCCmd[0] = 0x41;
		MMCCmd[1] = 0x00;
		MMCCmd[2] = 0x00;
		MMCCmd[3] = 0x00;
		MMCCmd[4] = 0x00;
		/* checksum is no longer required but we always send 0xFF */
		MMCCmd[5] = 0xFF;
		SSPSend(MMCCmd, MMC_CMD_SIZE);
		i--;
	} while ((mmc_response(0x00) != 0) && (i > 0));
	/* timeout waiting for 0x00 from the MMC */
	if (i == 0) {
		MMCStatus = OP_COND_TIMEOUT;
		SSP0_UNSEL(SSP0_SSEL_SD_BIT);
		return MMCStatus;
	}

	/* Send some dummy clocks after SEND_OP_COND */
	SSP0_UNSEL(SSP0_SSEL_SD_BIT);
	SPI_ReceiveByte();

	SSP0_SEL(SSP0_SSEL_SD_BIT);
	/* send MMC CMD16(SET_BLOCKLEN) to set the block length */
	MMCCmd[0] = 0x50;
	MMCCmd[1] = 0x00; /* 4 bytes from here is the block length */
	/* LSB is first */
	/* 00 00 00 10 set to 16 bytes */
	/* 00 00 02 00 set to 512 bytes */
	MMCCmd[2] = 0x00;
	/* high block length bits - 512 bytes */
	MMCCmd[3] = 0x02;
	/* low block length bits */
	MMCCmd[4] = 0x00;
	/* checksum is no longer required but we always send 0xFF */
	MMCCmd[5] = 0xFF;
	SSPSend(MMCCmd, MMC_CMD_SIZE);
	if ((mmc_response(0x00)) == 1) {
		MMCStatus = SET_BLOCKLEN_TIMEOUT;
		SSP0_UNSEL(SSP0_SSEL_SD_BIT);
		return MMCStatus;
	}

	SSP0_UNSEL(SSP0_SSEL_SD_BIT);
	SPI_ReceiveByte();
	return 0;
}

/************************** MMC Write Block ***************************/
/* write a block of data based on the length that has been set
 * in the SET_BLOCKLEN command.
 * Send the WRITE_SINGLE_BLOCK command out first, check the
 * R1 response, then send the data start token(bit 0 to 0) followed by
 * the block of data. The test program sets the block length to 512
 * bytes. When the data write finishs, the response should come back
 * as 0xX5 bit 3 to 0 as 0101B, then another non-zero value indicating
 * that MMC card is in idle state again.
 *
 */
int mmc_write_block(uint32_t block_number) {
	uint32_t varl, varh;
	uint8_t Status;

	SSP0_SEL(SSP0_SSEL_SD_BIT);
	/* block size has been set in mmc_init() */
	varl = ((block_number & 0x003F) << 9);
	varh = ((block_number & 0xFFC0) >> 7);
	/* send mmc CMD24(WRITE_SINGLE_BLOCK) to write the data to MMC card */
	MMCCmd[0] = 0x58;
	/* high block address bits, varh HIGH and LOW */
	MMCCmd[1] = varh >> 0x08;
	MMCCmd[2] = varh & 0xFF;
	/* low block address bits, varl HIGH and LOW */
	MMCCmd[3] = varl >> 0x08;
	MMCCmd[4] = varl & 0xFF;
	/* checksum is no longer required but we always send 0xFF */
	MMCCmd[5] = 0xFF;
	SSPSend(MMCCmd, MMC_CMD_SIZE);
	/* if mmc_response returns 1 then we failed to get a 0x00 response */
	if ((mmc_response(0x00)) == 1) {
		MMCStatus = WRITE_BLOCK_TIMEOUT;
		SSP0_UNSEL(SSP0_SSEL_SD_BIT);
		return MMCStatus;
	}
	/* Set bit 0 to 0 which indicates the beginning of the data block */
	MMCCmd[0] = 0xFE;
	SSPSend(MMCCmd, 1);
	/* send data, pattern as 0x00,0x01,0x02,0x03,0x04,0x05 ...*/
	SSPSend(MMCWRData, MMC_DATA_SIZE);
	/* Send dummy checksum */
	/* when the last check sum is sent, the response should come back
	 immediately. So, check the SPI FIFO MISO and make sure the status
	 return 0xX5, the bit 3 through 0 should be 0x05 */
	MMCCmd[0] = 0xFF;
	MMCCmd[1] = 0xFF;
	/* Set bit 0 to 0 which indicates the beginning of the data block */
	MMCCmd[0] = 0xFE;
	SSPSend(MMCCmd, 1);
	/* send data, pattern as 0x00,0x01,0x02,0x03,0x04,0x05 ...*/
	SSPSend(MMCWRData, MMC_DATA_SIZE);
	/* Send dummy checksum */
	/* when the last check sum is sent, the response should come back
	 immediately. So, check the SPI FIFO MISO and make sure the status
	 return 0xX5, the bit 3 through 0 should be 0x05 */
	MMCCmd[0] = 0xFF;
	MMCCmd[1] = 0xFF;

	Status = SPI_ReceiveByte();
	if ((Status & 0x0F) != 0x05) {
		MMCStatus = WRITE_BLOCK_FAIL;
		SSP0_UNSEL(SSP0_SSEL_SD_BIT);
		return MMCStatus;
	}
	/* if the status is already zero, the write hasn't finished
	 yet and card is busy */
	if (mmc_wait_for_write_finish() == 1) {
		MMCStatus = WRITE_BLOCK_FAIL;
		SSP0_UNSEL(SSP0_SSEL_SD_BIT);
		return MMCStatus;
	}
	SSP0_UNSEL(SSP0_SSEL_SD_BIT);
	SPI_ReceiveByte();
	return 0;
}

/************************** MMC Read Block ****************************/
/*
 * Reads a 512 Byte block from the MMC
 * Send READ_SINGLE_BLOCK command first, wait for response come back
 * 0x00 followed by 0xFE. The call SPI_Receive() to read the data
 * block back followed by the checksum.
 *
 */
int mmc_read_block(uint32_t block_number) {
	uint32_t Checksum;
	uint32_t varh, varl;

	SSP0_SEL(SSP0_SSEL_SD_BIT);
	varl = ((block_number & 0x003F) << 9);
	varh = ((block_number & 0xFFC0) >> 7);
	/* send MMC CMD17(READ_SINGLE_BLOCK) to read the data from MMC card */
	MMCCmd[0] = 0x51;
	/* high block address bits, varh HIGH and LOW */
	MMCCmd[1] = varh >> 0x08;
	MMCCmd[2] = varh & 0xFF;
	/* low block address bits, varl HIGH and LOW */
	MMCCmd[3] = varl >> 0x08;
	MMCCmd[4] = varl & 0xFF;
	/* checksum is no longer required but we always send 0xFF */
	MMCCmd[5] = 0xFF;
	SSPSend(MMCCmd, MMC_CMD_SIZE);
	/* if mmc_response returns 1 then we failed to get a 0x00 response */
	if ((mmc_response(0x00)) == 1) {
		MMCStatus = READ_BLOCK_TIMEOUT;
		SSP0_UNSEL(SSP0_SSEL_SD_BIT);
		return MMCStatus;
	}
	/* wait for data token */
	if ((mmc_response(0xFE)) == 1) {
		MMCStatus = READ_BLOCK_DATA_TOKEN_MISSING;
		SSP0_UNSEL(SSP0_SSEL_SD_BIT);
		return MMCStatus;
	}
	/* Get the block of data based on the length */
	SSPReceive(MMCRDData, MMC_DATA_SIZE);
	/* CRC bytes that are not needed */
	Checksum = SPI_ReceiveByte();
	Checksum = Checksum << 0x08 | SPI_ReceiveByte();
	SSP0_UNSEL(SSP0_SSEL_SD_BIT);
	SPI_ReceiveByte();
	return 0;
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
