//-----------------------------------------------------------------------------
// POLEM DataLogger
//	Author: Vicente Cunha
//	Date: june 2015
//	License: --
//-----------------------------------------------------------------------------
#include <avr/io.h>
#include "atmega328p_spi.h"
#include "atmega328p_sd.h"
//-----------------------------------------------------------------------------
// SPI configuration and enable.
//-----------------------------------------------------------------------------
void SPI_init()
{
	SPI_MSB_FIRST;
	SPI_MASTER;
	SPI_CLOCK_DIVIDER_64;
	SPI_MODE_0;
	SPI_PIN_CONFIG;
	SPI_ENABLE;
}
//-----------------------------------------------------------------------------
// SD initialization.
//-----------------------------------------------------------------------------
void SD_init()
{
	SD_NATIVE;
	
	unsigned char R1 = 0;
	do {SD_CMD0(R1);}
	while(!IS_SD_IDLE(R1));

	do {SD_CMD1(R1);}
	while(R1);
	
	SPI_CLOCK_DIVIDER_2;
}
//-----------------------------------------------------------------------------
// Write block to SD.
//-----------------------------------------------------------------------------
void writeBlock(unsigned char* address, unsigned char* data)
{
	SD_CMD24(address)
}
//-----------------------------------------------------------------------------
// MAIN
//-----------------------------------------------------------------------------
int main(void)
{
	// Initializations:
	SPI_init();
	SD_init();
	
	unsigned char address[] = {0x00,0x00,0x00,0x10};
	//Hello World!
	unsigned char data[SD_DATABLOCK_LENGTH] =
		{72,101,108,108,111,32,87,111,114,108,100,33};
	
	writeBlock(address,data);
}