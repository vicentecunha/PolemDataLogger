//-----------------------------------------------------------------------------
// ATmega328p SD Card
//	Author: Vicente Cunha
//	Date: june 2015
//	License: --
//-----------------------------------------------------------------------------
// This include file does not cover CRC generation. Therefore, CMD59 (which
//	activates CRC verification) is not defined, and should not be used.
//-----------------------------------------------------------------------------
#include <avr/io.h>
#include "atmega328p_spi.h"
#ifndef SD_H
#define SD_H
#define SD_DATABLOCK_LENGTH 512
//-----------------------------------------------------------------------------
// Power on to native operating mode routine:
//-----------------------------------------------------------------------------
#define SD_NATIVE SPI_CLOCK_DIVIDER_64; SPI_SET_SS;\
			for(int SD_K=0;SD_K<10;SD_K++) SPI_TRANSMIT_BYTE(0xff)
//-----------------------------------------------------------------------------
// Command responses:
//-----------------------------------------------------------------------------
#define SD_R1(R1) SPI_TRANSMIT_BYTE(0xff); R1 = SPI_RECEIVED_BYTE

#define SD_R1_IDLE					0x01
#define SD_R1_ERASE_RESET			0x02
#define SD_R1_ILLEGAL_COMMAND		0x04
#define SD_R1_CRC_ERROR				0x08
#define SD_R1_ERASE_SEQUENCE_ERROR	0x10
#define SD_R1_ADDRESS_ERROR			0x20
#define SD_R1_PARAMETER_ERROR		0x40

#define IS_SD_IDLE(R1)	(R1 & SD_R1_IDLE ? 1 : 0)
//-----------------------------------------------------------------------------
// CMD0: GO_IDLE_STATE (software reset)
//-----------------------------------------------------------------------------
#define SD_CMD0(R1) SPI_LOWER_SS; SPI_TRANSMIT_BYTE(0x40);\
	for(int SD_K = 0; SD_K < 4; SD_K++) SPI_TRANSMIT_BYTE(0x00);\
	SPI_TRANSMIT_BYTE(0x95); SD_R1(R1); SPI_SET_SS
//-----------------------------------------------------------------------------
// CMD1: SEND_OP_COND (initialization process)
//-----------------------------------------------------------------------------
#define SD_CMD1(R1) SPI_LOWER_SS; SPI_TRANSMIT_BYTE(0x41);\
	for(int SD_K = 0; SD_K < 4; SD_K++) SPI_TRANSMIT_BYTE(0x00);\
	SPI_TRANSMIT_BYTE(0x01); SD_R1(R1); SPI_SET_SS
//-----------------------------------------------------------------------------
// CMD24: WRITE_BLOCK (write a block)
//-----------------------------------------------------------------------------
#define SD_CMD24(ADDRESS, R1, DATA) SPI_LOWER_SS;\
	SPI_TRANSMIT_BYTE(0x40 + 24);\
	for(int SD_K = 0; SD_K < 4; SD_K++) SPI_TRANSMIT_BYTE(ADDRESS[SD_K]);\
	SPI_TRANSMIT_BYTE(0x01); SD_R1(R1); SPI_TRANSMIT_BYTE(0xff);\
	SPI_TRANSMIT_BYTE(0xfe); SPI_TRANSMIT_DATA_BLOCK(DATA);\
	SPI_TRANSMIT_BYTE(0x00); SPI_TRANSMIT_BYTE(0x00);\
#endif