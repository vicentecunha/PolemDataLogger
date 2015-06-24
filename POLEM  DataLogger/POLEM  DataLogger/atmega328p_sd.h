//-----------------------------------------------------------------------------
// ATmega328p SD Card
//	Author: Vicente Cunha
//	Date: june 2015
//	License: --
//-----------------------------------------------------------------------------
// This include file does not cover CRC generation. Therefore, CMD_59 (which
//	activates CRC verification) is not defined, and should not be used.
//-----------------------------------------------------------------------------
#include <avr/io.h>
#include "atmega328p_spi.h"
#ifndef SD_H
#define SD_H
//-----------------------------------------------------------------------------
// Power on to native operating mode routine:
//-----------------------------------------------------------------------------
#define SD_NATIVE SPI_CLOCK_DIVIDER_64; SPI_SET_SS;\
	for(int SD_K=0;K<10;k++) SPI_TRANSMIT_BYTE(0xff)
//-----------------------------------------------------------------------------
//	Command responses:
//-----------------------------------------------------------------------------
char SD_DUMMY;
#define SD_R1(R1) SPI_TRANSMIT_BYTE(0xff); R1 = SPI_RECEIVED_BYTE
#define SD_IDLE(R1)
#define SD_PARAMETER_ERROR(R1)
#define SD_ADDRESS_ERROR(R1)
#define SD_ERASE_SEQUENCE_ERROR(R1)
#define SD_CRC_ERROR(R1)
#define SD_ILLEGAL_COMMAND(R1)
#define SD_ERASE_RESET(R1)
//-----------------------------------------------------------------------------
//	Command specific macros:
//-----------------------------------------------------------------------------
#define SD_CMD0	SPI_LOWER_SS; SPI_TRANSMIT_BYTE(0x40);\
	for(int SD_K=0;K<4;k++) SPI_TRANSMIT_BYTE(0x00); SPI_TRANSMIT_BYTE(0x95);
