//////////////////////////////////////////////////////////////////////////
// POLEM "Data Logger"
//
// This software reads sensors and logs obtained data to an SD card.
// Logged data will be used to prove sensor-based algorithms.
//
// Vicente Cunha - 05/2015
//////////////////////////////////////////////////////////////////////////

#include <avr/io.h>


//////////////////////////////////////////////////////////////////////////
// SPI Pinout:
//	MOSI: PB5 (Arduino Pin 13)
//	MISO: PB4 (Arduino Pin 12)
//	SCK:  PB3 (Arduino Pin 11)
//	/SS:  PB2 (Arduino Pin 10)
//////////////////////////////////////////////////////////////////////////

const int MOSIpin = 5;
const int MISOpin = 4;
const int SCKpin  = 3;
const int SSpin   = 2;


//----------------------------------------------------------------------//
// SPI Functions:
//----------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////
// /SS pin control:
//	Sets /SS pin logic level (1 for HIGH, 0 for LOW)
//
// NOTE:
//	Argument "level" must be 1 or 0
//	Otherwise, this function does nothing
//////////////////////////////////////////////////////////////////////////

void SPI_setSSpin(int level)
{
	if(level == 1)
		PINB |= (1<<SSpin);
	else if(level == 0)
		PINB &= !(1<<SSpin);
}


//////////////////////////////////////////////////////////////////////////
// SPI Configuration:
//	SPI communication mode is 0 (CPOL = 0, CPHA = 0)
//	Clock rate is 250 kHz
//////////////////////////////////////////////////////////////////////////

void SPI_Init(void)
{
	DDRB |= (1<<MOSIpin)|(1<<MISOpin)|(1<<SCKpin);
	SPI_setSSpin(1);
	
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR1)|(1<<SPR0);
	SPSR |= (1<<SPI2X);
}


//////////////////////////////////////////////////////////////////////////
// SPI Transmission:
//	Send out 8 bits and wait for transmission to complete.
//////////////////////////////////////////////////////////////////////////

void SPI_Transmit(unsigned char data)
{
	SPDR = data;
	while(!(SPSR & (1<<SPIF)));
}


//----------------------------------------------------------------------//
// SD Card Functions:
//----------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////
// SD Card Command:
//	Commands are sequences of 6 bytes
//	Byte 1:    01(Command Index)
//	Bytes 2-5: (Arguments)
//	Byte 6:    (CRC)1
//
//	Command index is 6 bits long
//	CRC is 7 bits long
//
// NOTE:
//	By default, CRC is only checked in CMD8 (voltage supply check)
//	CRC checking can be enabled by CMD59, with arguments {0,0,0,1}
//////////////////////////////////////////////////////////////////////////

void SDCard_Command(unsigned int commandIndex, unsigned char* args, unsigned char crc)
{
	SPI_Transmit(0x40+commandIndex);
	for(int k = 0; k < 4; k++)
		SPI_Transmit(args[k]);
	SPI_Transmit(crc);
}


//////////////////////////////////////////////////////////////////////////
// SD Card Responses Type 1
//////////////////////////////////////////////////////////////////////////

unsigned char SDCard_ResponseT1(void)
{
	SPI_Transmit(0xff);
	return SPDR;
}


//////////////////////////////////////////////////////////////////////////
// SD Card Configuration:
//
// Power-on Routine:
//	MOSI and /SS must be set to high.
//	SCKL must send more than 74 clock pulses.
//	This is done by sending 10 times 0xFF words.
//	Card will then enter native operating mode.
//	CMD0 makes the card enter SPI mode.
//
//	Block length is set to 16 bytes
//
// IMPORTANT NOTE:
//	SD card must NOT be SDHC or SDXC
//	These cards have fixed blocks of 512 bytes each
//////////////////////////////////////////////////////////////////////////

void SDCard_PowerOn(void)
{
	unsigned char args[4] = {0,0,0,0};

	SPI_setSSpin(1);
	for(int k = 0; k < 10; k++)
		SPI_Transmit(0xff);
		
	SPI_setSSpin(0);
	SDCard_Command(0, args, 0);
	SDCard_ResponseT1();
	SPI_setSSpin(1);
	
	SPI_setSSpin(0);
	args[3] = 16;
	SDCard_Command(16, args, 0);
	SDCard_ResponseT1();
	SPI_setSSpin(1);
}

//----------------------------------------------------------------------//
// MAIN
//----------------------------------------------------------------------//

int main(void)
{
	SPI_Init();
	SDCard_PowerOn();
	
	//TODO: Write 48 65 6c 6c 6f 20 57 6f 72 6c 64 21 (Hello World!) to a file

	return 0;
}