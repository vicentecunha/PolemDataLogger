//-----------------------------------------------------------------------------
// POLEM DataLogger
//	Author: Vicente Cunha
//	Date: july 2015
//	License: --
//-----------------------------------------------------------------------------
#include <avr/interrupt.h>

/* DEBUG: USART */
void usartInit()
{
	UBRR0L = 103; // 9600 bps
	UCSR0C = (1 << UCSZ00) | (1 << UCSZ01);	// Dados de 8 bits
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);
}

unsigned char usartReceive()
{
	while(!(UCSR0A & (1 << RXC0)));
	return(UDR0);
}

void usartTransmit(unsigned char data)
{
	while(!(UCSR0A & (1 << UDRE0)));
	UDR0 = data;
}

/* Pluviometer, Resources: external interrupt 0 (INT0) */
//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------
uint64_t pluviometerCounter = 0;
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
void adcConfig()
{
	// Internal 1.1V Voltage Reference with external capacitor at AREF pin
	ADMUX |= (1 << REFS1)|(1 << REFS0);	
	// // ADC Conversion Result is right adjusted.
	ADMUX &= ~(1 << ADLAR);
	// ADC Prescaler of 128
	ADCSRA |= (1 << ADPS2)|(1 << ADPS1)|(1 << ADPS0);	
	// Digital Input Disable
	DIDR0 |= (1<< ADC5D)|(1<< ADC4D)|(1<< ADC3D)|(1<< ADC2D)|(1<< ADC1D)|(1<< ADC0D);
}
//-----------------------------------------------------------------------------
// ADC Enable
//-----------------------------------------------------------------------------
void adcEnable()
{
	ADCSRA |= (1 << ADEN);
}
//-----------------------------------------------------------------------------
// ADC Disable
//-----------------------------------------------------------------------------
void adcDisable()
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
	return((ADCH << 8) + ADCL);
}

/* SD Card, Resources: SPI - Serial Peripheral Interface */
//-----------------------------------------------------------------------------
// Constant CRC value used, corresponding to CMD0
//-----------------------------------------------------------------------------
const uint8_t CRC = 0x95;
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
	// CLock rate of 250 Hz (prescaler of 64)
	SPSR &= ~(1 << SPI2X);
	SPCR |=  (1 << SPR1); SPCR &= ~(1 << SPR0);
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
	usartTransmit('0');
	
	// Power on to native SD
	PORTB |= (1 << PORTB2);
	for(uint8_t k=0; k < 10; k++) SPITransfer(0xFF);
	
	usartTransmit('1');
		
	// Software reset (CMD0)
	PORTB &= ~(1 << PORTB2);
		
	SPITransfer(0x40);
	for(uint8_t k = 0; k < 4; k++) SPITransfer(0x00);
	SPITransfer(CRC);
	
	uint8_t R1 = 0;
	while(!R1)
	{
		R1 = SPITransfer(0xFF);
		usartTransmit(R1);
	}
	
	PORTB |= (1 << PORTB2);	
	
	usartTransmit('2');
	
	// Initialization process (CMD1)
	PORTB &= ~(1 << PORTB2);
	
	SPITransfer(0x40 + 0x01);
	for(uint8_t k = 0; k < 4; k++) SPITransfer(0x00);
	SPITransfer(CRC);
	
	while(R1)
	{
		R1 = SPITransfer(0xFF);
		usartTransmit(R1);
	}
	
	PORTB |= (1 << PORTB2);
	
	usartTransmit('3');
}
//-----------------------------------------------------------------------------
// Write a single block of 512 bytes to the SD Card
//-----------------------------------------------------------------------------
void SDCardWriteSingleBlock(uint8_t* address, uint8_t* data)
{
	PORTB &= ~(1 << PORTB2);
	
	// CMD24
	SPITransfer(0x40 + 0x24);
	for(uint8_t k = 0; k < 4; k++) SPITransfer(address[k]);
	SPITransfer(CRC);
	uint8_t R1 = SPITransfer(0xFF);
	
	usartTransmit(R1);
	
	// Wait 1 byte
	SPITransfer(0xFF);
	
	// Data Packet
	SPITransfer(0xFE); // Data Token
	for(uint8_t k = 0; k < 512; k++) SPITransfer(data[k]);
	SPITransfer(CRC); SPITransfer(CRC);
	uint8_t dataResponse = SPITransfer(0xFF);
	
	usartTransmit(dataResponse);
	
	PORTB |= (1 << PORTB2);
}

/* Sleep, Resources: Timer 1 */
//-----------------------------------------------------------------------------
// Timer 1 Configuration
//-----------------------------------------------------------------------------
void sleepConfig()
{
	// Normal port operation, OC1A/OC1B disconnected
	TCCR1A &= ~((1 << COM1A1)|(1 << COM1A0)|(1 << COM1B0)|(1 << COM1B0));
	// Normal mode (counter)
	TCCR1B &= ~((1 << WGM13)|(1 << WGM12));
	TCCR1A &= ~((1 << WGM11)|(1 << WGM10));	
	// Input Capture Noise Canceler disabled
	TCCR1B &= ~(1 << ICNC1);	
	// Input Capture, Output Compare B/A Match Interrupts disabled
	TIMSK1 &= ~((1 << ICIE1)|(1 << OCIE1B)|(1 << OCIE1A));	
	// Timer1 Overflow Interrupt Enable
	TIMSK1 |= (1 << TOIE1);
}
//-----------------------------------------------------------------------------
// Sleep for one hour
//-----------------------------------------------------------------------------
void sleepOneHour()
{
	// Reset counter
	TCNT1H = 0x00; TCNT1L = 0x00;	
	// To sleep for 1 hour, with T = 64 ms, it takes 56250 (0xDBBA) counts
	OCR1AH = 0xDB; OCR1AL = 0xBA;	
	// Start counter, prescaler to 1024 (15.625 kHz, or T = 64 ms)
	TCCR1B |= (1 << CS12)|(1 << CS10); TCCR1B &= ~(1 << CS11);
	// Wait for compare match A
	while(!(TIFR1 & (1 << OCF1A)));	
	// Stop counter
	TCCR1B &= ~((1 << CS12)|(1 << CS11)|(1 << CS10));
}

/* Main */
int main()
{
	// DEBUG
	usartInit();
	
	// Initialization
	pluviometerConfig(); pluviometerInterruptEnable(); sei();
	adcConfig(); adcEnable();
	SPIConfig(); SDCardEnable(); SDCardInit();
	sleepConfig();
	
	uint32_t currentBlockAddress = 0;
	uint8_t SDCardBlockAddress[4];
	uint8_t SDCardDataBlock[512];
	
	for(uint8_t k = 12; k < 512; k++)
	{
		SDCardDataBlock[k] = 0x00;
	}
	
	while(1)
	{
		// DEBUG: Hello World!
		SDCardDataBlock[0] = 'H';
		SDCardDataBlock[1] = 'e';
		SDCardDataBlock[2] = 'l';
		SDCardDataBlock[3] = 'l';
		SDCardDataBlock[4] = 'o';
		SDCardDataBlock[5] = ' ';
		SDCardDataBlock[6] = 'W';
		SDCardDataBlock[7] = 'o';
		SDCardDataBlock[8] = 'r';
		SDCardDataBlock[9] = 'l';
		SDCardDataBlock[10] = 'd';
		SDCardDataBlock[11] = '!';
		
		//// Pluviometer
		//SDCardDataBlock[0] = (pluviometerCounter >> 56);
		//SDCardDataBlock[1] = (pluviometerCounter >> 48);
		//SDCardDataBlock[2] = (pluviometerCounter >> 40);
		//SDCardDataBlock[3] = (pluviometerCounter >> 32);
		//SDCardDataBlock[4] = (pluviometerCounter >> 24);
		//SDCardDataBlock[5] = (pluviometerCounter >> 16);
		//SDCardDataBlock[6] = (pluviometerCounter >> 8);
		//SDCardDataBlock[7] = (pluviometerCounter >> 0);
		//SDCardDataBlock[8] = 0x00;
		//
		//// Irrometer
		//uint16_t irrometerMeasuer = irrometerSingleConversion();
		//
		//SDCardDataBlock[9] = (irrometerMeasuer >> 8);
		//SDCardDataBlock[10] = (irrometerMeasuer >> 0);
		//SDCardDataBlock[11] = 0x00;
		//
		//// Thermometer
		//uint16_t thermometerMeasure = thermometerSingleConversion();
		//
		//SDCardDataBlock[12] = (thermometerMeasure >> 8);
		//SDCardDataBlock[13] = (thermometerMeasure >> 0);
		//SDCardDataBlock[14] = 0x00;
		
		// Block Address
		SDCardBlockAddress[0] = (currentBlockAddress >> 24);
		SDCardBlockAddress[1] = (currentBlockAddress >> 16);
		SDCardBlockAddress[2] = (currentBlockAddress >> 8);
		SDCardBlockAddress[3] = (currentBlockAddress >> 0);
		
		// Write Data Block
		SDCardWriteSingleBlock(SDCardBlockAddress, SDCardDataBlock);
		
		// Increment Block Address
		currentBlockAddress++;
		
		// Sleep
		adcDisable(); SDCardDisable();
		sleepOneHour();
		adcEnable(); SDCardEnable();
	}

	return 0;
};