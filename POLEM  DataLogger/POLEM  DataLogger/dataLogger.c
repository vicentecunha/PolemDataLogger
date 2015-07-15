//-----------------------------------------------------------------------------
// POLEM DataLogger
//	Author: Vicente Cunha
//	Date: july 2015
//	License: --
//-----------------------------------------------------------------------------
#include <avr/interrupt.h>
#include <avr/sleep.h>

/* Pluviometer, Resources: external interrupt 0 (INT0) */
//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------
volatile uint64_t pluviometerCounter = 0;
//-----------------------------------------------------------------------------
// Configuration
//-----------------------------------------------------------------------------
void pluviometerConfig()
{
	// PD2 is input with pull-up
	DDRD  &= ~(1 << DDD2);
	PORTD |= (1 << PORTD2);
	MCUCR &= ~(1 << PUD);	
	// The falling edge of INT0 generates an interrupt request
	EICRA |=  (1 << ISC01); EICRA &= ~(1 << ISC00);
}
//-----------------------------------------------------------------------------
// External Interrupt 0 Enable
//-----------------------------------------------------------------------------
void pluviometerInterruptEnable()
{	
	EIMSK |= (1 << INT0);
}
//-----------------------------------------------------------------------------
// External Interrupt 0 Handler
//-----------------------------------------------------------------------------
ISR(INT0_vect)
{
	pluviometerCounter++;
}

/* Analog-to-Digital Converter */
//-----------------------------------------------------------------------------
// Configuration
//-----------------------------------------------------------------------------
void ADCConfig()
{
	// Internal 1.1V Voltage Reference with external capacitor at AREF pin
	ADMUX |= (1 << REFS1)|(1 << REFS0);	
	// ADC Conversion Result is right adjusted.
	ADMUX &= ~(1 << ADLAR);
	// ADC Prescaler of 128
	ADCSRA |= (1 << ADPS2)|(1 << ADPS1)|(1 << ADPS0);	
	// Digital Input Disable
	DIDR0 |= (1<< ADC5D)|(1<< ADC4D)|(1<< ADC3D)|(1<< ADC2D)|(1<< ADC1D)|(1<< ADC0D);
}
//-----------------------------------------------------------------------------
// ADC Enable
//-----------------------------------------------------------------------------
void ADCEnable()
{
	ADCSRA |= (1 << ADEN);
}
//-----------------------------------------------------------------------------
// ADC Disable
//-----------------------------------------------------------------------------
void ADCDisable()
{
	ADCSRA &= ~(1 << ADEN);
}

/* Irrometer, Resources: Analog-to-Digital Converter 0 */
//-----------------------------------------------------------------------------
// Single Analog-to-Digital Conversion of Irrometer
//-----------------------------------------------------------------------------
uint16_t irrometerSingleConversion()
{
	// Select channel input ADC0
	ADMUX &= ~((1 << MUX3)|(1 << MUX2)|(1 << MUX1)|(1 << MUX0));	
	ADCSRA |= (1 << ADSC); // ADC Start Conversion
	while(!(ADCSRA & (1 << ADIF))); // Wait conversion
	ADCSRA |= (1 << ADIF); // Clear flag
	return((ADCH << 8) + ADCL);
}

/* Thermometer, Resources: Analog-to-Digital Converter 1 */
//-----------------------------------------------------------------------------
// Single Analog-to-Digital Conversion of Thermometer
//-----------------------------------------------------------------------------
uint16_t thermometerSingleConversion()
{
	// Select channel input ADC1
	ADMUX &= ~((1 << MUX3)|(1 << MUX2)|(1 << MUX1)); ADMUX |= (1 << MUX0);
	ADCSRA |= (1 << ADSC); // ADC Start Conversion
	while(!(ADCSRA & (1 << ADIF))); // Wait conversion
	ADCSRA |= (1 << ADIF); // Clear flag
	return((ADCH << 8) + ADCL);
}

/* SD Card, Resources: SPI - Serial Peripheral Interface */
//-----------------------------------------------------------------------------
// SPI Bus Configuration
//-----------------------------------------------------------------------------
void SPIConfig()
{
	// The MSB of the data word is transmitted first. SPI mode 0.
	SPCR &= ~((1 << DORD)|(1 << CPOL)|(1 << CPHA));
	// Master SPI mode
	SPCR |= (1 << MSTR);	
	// SPI bus pins configuration
	/*
		SCK:  pin 13 (PB5)
		MISO: pin 12 (PB4)
		MOSI: pin 11 (PB3)
		SS:   pin 10 (PB2)
	*/
	DDRB |= (1<<DDB2)|(1<<DDB3)|(1<<DDB5); DDRB &= ~(1 << DDB4);
	// CLock rate of 125 Hz (prescaler of 128)
	SPSR &= ~(1 << SPI2X);
	SPCR |=  (1 << SPR1)|(1 << SPR0);
}
//-----------------------------------------------------------------------------
// Send byte to MOSI line, wait transmission, and return received byte by MISO
//-----------------------------------------------------------------------------
uint8_t SPITransfer(uint8_t byte)
{
	SPDR = byte;
	while(!(SPSR & (1 << SPIF)));
	return SPDR;
}
//-----------------------------------------------------------------------------
// SPI Bus Enable
//-----------------------------------------------------------------------------
void SDCardEnable()
{
	SPCR |= (1 << SPE);
}
//-----------------------------------------------------------------------------
// SPI Bus Disable
//-----------------------------------------------------------------------------
void SDCardDisable()
{
	SPCR &= ~(1 << SPE);
}
//-----------------------------------------------------------------------------
// SD Card Initialization
//-----------------------------------------------------------------------------
void SDCardInit()
{
	// Power on to native SD
	PORTB |= (1 << PORTB2);
	for(int8_t k = 0; k < 10; k++) SPITransfer(0xFF);
		
	// Software reset (CMD0)
	PORTB &= ~(1 << PORTB2);	
	SPITransfer(0x40); SPITransfer(0x00); SPITransfer(0x00);
	SPITransfer(0x00); SPITransfer(0x00); SPITransfer(0x95);
	uint8_t R1 = 0;
	do {R1 = SPITransfer(0xFF);} while(R1 == 0xFF);
	PORTB |= (1 << PORTB2);
	SPITransfer(0xFF);
	
	// CMD8
	PORTB &= ~(1 << PORTB2);
	SPITransfer(0x48); SPITransfer(0x00); SPITransfer(0x00);
	SPITransfer(0x01); SPITransfer(0xAA); SPITransfer(0x87);
	do {R1 = SPITransfer(0xFF);} while(R1 == 0xFF);
	for(int8_t k = 0; k < 4; k++) SPITransfer(0xFF);
	PORTB |= (1 << PORTB2);
	SPITransfer(0xFF);
	
	// Initialization process (CMD58 + CMD55 + ACMD41)
	do {
		PORTB &= ~(1 << PORTB2);
		SPITransfer(0x40 + 58); SPITransfer(0x00); SPITransfer(0x00);
		SPITransfer(0x00); SPITransfer(0x00); SPITransfer(0xFF);
		do {R1 = SPITransfer(0xFF);} while(R1 == 0xFF);
		for(int8_t k = 0; k < 4; k++) SPITransfer(0xFF);
		PORTB |= (1 << PORTB2);
		SPITransfer(0xFF);
		
		PORTB &= ~(1 << PORTB2);
		SPITransfer(0x40 + 55); SPITransfer(0x00); SPITransfer(0x00);
		SPITransfer(0x00); SPITransfer(0x00); SPITransfer(0xFF);
		do {R1 = SPITransfer(0xFF);} while(R1 == 0xFF);
		PORTB |= (1 << PORTB2);
		SPITransfer(0xFF);
		
		PORTB &= ~(1 << PORTB2);
		SPITransfer(0x40 + 41); SPITransfer(0x40); SPITransfer(0x00);
		SPITransfer(0x00); SPITransfer(0x00); SPITransfer(0xFF);
		do {R1 = SPITransfer(0xFF);} while(R1 == 0xFF);
		PORTB |= (1 << PORTB2);
		SPITransfer(0xFF);
	} while(R1);
	
	// CMD58
	PORTB &= ~(1 << PORTB2);
	SPITransfer(0x40 + 58); SPITransfer(0x00); SPITransfer(0x00);
	SPITransfer(0x00); SPITransfer(0x00); SPITransfer(0xFF);
	do {R1 = SPITransfer(0xFF);} while(R1 == 0xFF);
	for(int8_t k = 0; k < 4; k++) SPITransfer(0xFF);
	PORTB |= (1 << PORTB2);
	SPITransfer(0xFF);
}
	
//-----------------------------------------------------------------------------
// Write a single block of 512 bytes to the SD Card
//-----------------------------------------------------------------------------
void SDCardWriteSingleBlock(uint8_t* address, uint8_t* data)
{
	// Write Single Block (CMD24)
	PORTB &= ~(1 << PORTB2);
	SPITransfer(0x40 + 24);
	for(int8_t k = 0; k < 4; k++) SPITransfer(address[k]);
	SPITransfer(0xFF);
	uint8_t R1 = 0;
	do {R1 = SPITransfer(0xFF);} while(R1 == 0xFF);
	PORTB |= (1 << PORTB2);
	SPITransfer(0xFF);

	// Data Packet
	PORTB &= ~(1 << PORTB2);
	SPITransfer(0xFE); // Data Token
	for(int16_t k = 0; k < 512; k++) SPITransfer(data[k]); // Data Block
	SPITransfer(0xFF); SPITransfer(0xFF); // CRC
	SPITransfer(0xFF); // Data Response
	while(!SPITransfer(0xFF)); // Busy
	PORTB |= (1 << PORTB2);
	SPITransfer(0xFF);
}

/* Sleep, Resources: Timer 2 */
volatile uint32_t tim2OvfCounter = 0;
//-----------------------------------------------------------------------------
// Timer 2 Interrupt Enable
//-----------------------------------------------------------------------------
void timer2Enable()
{
	// Timer2 Overflow Interrupt Enable
	TIMSK2 |= (1 << TOIE2);
	// Start counter, prescaler to 1024, ovfPeriod = 16.4 ms
	TCCR2B |= (1 << CS22)|(1 << CS21)|(1 << CS20);
}
//-----------------------------------------------------------------------------
// Timer 2 Overflow Interrupt Handler
//-----------------------------------------------------------------------------
ISR(TIMER2_OVF_vect)
{
	tim2OvfCounter++;
}

/* Main */
int main()
{
	// Initialization
	pluviometerConfig(); pluviometerInterruptEnable();
	ADCConfig(); ADCEnable();
	SPIConfig(); SDCardEnable(); SDCardInit();
	timer2Enable();	

	set_sleep_mode(SLEEP_MODE_PWR_SAVE);	
	sei();
	uint32_t currentBlockAddress = 0;
	
	while(1)
	{
		sleep_mode();
		
		if (tim2OvfCounter > 219510) // One hour
		{
			tim2OvfCounter = 0;
		
			uint8_t SDCardBlockAddress[4];
			uint8_t SDCardDataBlock[512];
			for(uint16_t k = 32; k < 512; k++) SDCardDataBlock[k] = 0x00;
		
			// Timestamp (one per hour)
			SDCardDataBlock[0] = 0x00;
			SDCardDataBlock[1] = 0x00;
			SDCardDataBlock[2] = 0x00;
			SDCardDataBlock[3] = 0x00;
			SDCardDataBlock[4] = (currentBlockAddress >> 24);
			SDCardDataBlock[5] = (currentBlockAddress >> 16);
			SDCardDataBlock[6] = (currentBlockAddress >> 8);
			SDCardDataBlock[7] = (currentBlockAddress >> 0);
		
			// Pluviometer
			pluviometerCounter = 0x55AA55AA55AA55AA; //Test
			SDCardDataBlock[8] = (pluviometerCounter >> 56);
			SDCardDataBlock[9] = (pluviometerCounter >> 48);
			SDCardDataBlock[10] = (pluviometerCounter >> 40);
			SDCardDataBlock[11] = (pluviometerCounter >> 32);
			SDCardDataBlock[12] = (pluviometerCounter >> 24);
			SDCardDataBlock[13] = (pluviometerCounter >> 16);
			SDCardDataBlock[14] = (pluviometerCounter >> 8);
			SDCardDataBlock[15] = (pluviometerCounter >> 0);

			// Irrometer
			uint16_t irrometerMeasuer = irrometerSingleConversion();
		
			SDCardDataBlock[16] = 0x00;
			SDCardDataBlock[17]	= 0x00;
			SDCardDataBlock[18]	= 0x00;
			SDCardDataBlock[19]	= 0x00;
			SDCardDataBlock[20]	= 0x00;
			SDCardDataBlock[21] = 0x00;
			SDCardDataBlock[22] = (irrometerMeasuer >> 8);
			SDCardDataBlock[23] = (irrometerMeasuer >> 0);
		
			// Thermometer
			uint16_t thermometerMeasure = thermometerSingleConversion();
		
			SDCardDataBlock[24] = 0x00;
			SDCardDataBlock[25]	= 0x00;
			SDCardDataBlock[26]	= 0x00;
			SDCardDataBlock[27]	= 0x00;
			SDCardDataBlock[28]	= 0x00;
			SDCardDataBlock[29] = 0x00;
			SDCardDataBlock[30] = (thermometerMeasure >> 8);
			SDCardDataBlock[31] = (thermometerMeasure >> 0);
		
			// Block Address
			SDCardBlockAddress[0] = (currentBlockAddress >> 24);
			SDCardBlockAddress[1] = (currentBlockAddress >> 16);
			SDCardBlockAddress[2] = (currentBlockAddress >> 8);
			SDCardBlockAddress[3] = (currentBlockAddress >> 0);
		
			// Write Data Block
			SDCardWriteSingleBlock(SDCardBlockAddress, SDCardDataBlock);
		
			// Increment Block Address
			currentBlockAddress++;
		}
	}

	return 0;
};