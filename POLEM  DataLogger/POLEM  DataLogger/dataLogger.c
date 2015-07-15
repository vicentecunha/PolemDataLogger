//-----------------------------------------------------------------------------
// POLEM DataLogger
//	Author: Vicente Cunha
//	Date: july 2015
//	License: --
//-----------------------------------------------------------------------------
#include <avr/interrupt.h>

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
	SPITransfer(0xFF); //Data Response
	PORTB |= (1 << PORTB2);
	SPITransfer(0xFF);
}

/* Sleep, Resources: Timer 1 */
//-----------------------------------------------------------------------------
// Overflow Counter
//-----------------------------------------------------------------------------
uint16_t ovfCounter = 0;
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
// Sleep for One Hour
//-----------------------------------------------------------------------------
void sleepOneHour()
{
	// Reset counters
	TCNT1H = 0x00; TCNT1L = 0x00; ovfCounter = 0;
	// Start counter, prescaler to 1024 (15625 Hz, or T = 64 us)
	TCCR1B |= (1 << CS12)|(1 << CS10); TCCR1B &= ~(1 << CS11);
	// To sleep for 1 hour, with T = 64 ns, it takes 56250000 counts, 858 ovf
	while(ovfCounter < 858);
	// Stop counter
	TCCR1B &= ~((1 << CS12)|(1 << CS11)|(1 << CS10));
}
//-----------------------------------------------------------------------------
// Timer 1 Overflow Interrupt Handler
//-----------------------------------------------------------------------------
ISR(TIMER1_OVF_vect)
{
	ovfCounter++;
}


/* Main */
int main()
{
	// Initialization
	pluviometerConfig(); pluviometerInterruptEnable(); sei();
	ADCConfig(); ADCEnable();
	SPIConfig(); SDCardEnable(); SDCardInit();
	sleepConfig();
	
	uint32_t currentBlockAddress = 0;
	uint8_t SDCardBlockAddress[4];
	uint8_t SDCardDataBlock[512];
	for(uint16_t k = 20; k < 512; k++) SDCardDataBlock[k] = 0x00;

	while(1)
	{
		// Block Address ("Timestamp", one per hour)
		SDCardDataBlock[0] = (currentBlockAddress >> 24);
		SDCardDataBlock[1] = (currentBlockAddress >> 16);
		SDCardDataBlock[2] = (currentBlockAddress >> 8);
		SDCardDataBlock[3] = (currentBlockAddress >> 0);
		SDCardDataBlock[4] = '\t';
		
		// Pluviometer
		SDCardDataBlock[5] = (pluviometerCounter >> 56);
		SDCardDataBlock[6] = (pluviometerCounter >> 48);
		SDCardDataBlock[7] = (pluviometerCounter >> 40);
		SDCardDataBlock[8] = (pluviometerCounter >> 32); 
		SDCardDataBlock[9] = (pluviometerCounter >> 24); 
		SDCardDataBlock[10] = (pluviometerCounter >> 16);
		SDCardDataBlock[12] = (pluviometerCounter >> 8);
		SDCardDataBlock[13] = (pluviometerCounter >> 0);
		SDCardDataBlock[14] = '\t';
		
		// Irrometer
		uint16_t irrometerMeasuer = irrometerSingleConversion();
		
		SDCardDataBlock[15] = (irrometerMeasuer >> 8);
		SDCardDataBlock[16] = (irrometerMeasuer >> 0);
		SDCardDataBlock[17] = '\t';
		
		// Thermometer
		uint16_t thermometerMeasure = thermometerSingleConversion();
		
		SDCardDataBlock[18] = (thermometerMeasure >> 8);
		SDCardDataBlock[19] = (thermometerMeasure >> 0);
		
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
		ADCDisable(); SDCardDisable();
		sleepOneHour();
		ADCEnable(); SDCardEnable();
	}

	return 0;
};