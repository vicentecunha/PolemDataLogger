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
// MAIN.
//-----------------------------------------------------------------------------
int main(void)
{
	SPI_init();
	
	//SD_init();
}