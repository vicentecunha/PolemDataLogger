//-----------------------------------------------------------------------------
// POLEM DataLogger
//	Author: Vicente Cunha
//	Date: june 2015
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
	// PD2 is input with pull-up.
	DDRD  &= ~(1 << DDD2);
	PORTD |= (1 << PORTD2);
	MCUCR &= ~(1 << PUD);
	
	// The falling edge of INT0 generates an interrupt request.
	EICRA |=  (1 << ISC01);
	EICRA &= ~(1 << ISC00);
}
//-----------------------------------------------------------------------------
// Enable
//-----------------------------------------------------------------------------
void pluviometerEnable()
{	
	// External Interrupt Request 0 Enable.
	EIMSK |= (1 << INT0);
}
//-----------------------------------------------------------------------------
// Interrupt Handlers
//-----------------------------------------------------------------------------
ISR(INT0_vect)
{
	pluviometerCounter++;
}

/* Tensiometer, Resources: Analog-to-Digital Converter */
//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------
uint16_t tensiometerMeasure = 0;
uint8_t conversionComplete = 0;
//-----------------------------------------------------------------------------
// Configuration
//-----------------------------------------------------------------------------
void tensiometerConfig()
{
	// Internal 1.1V Voltage Reference with external capacitor at AREF pin.
	ADMUX |= (1 << REFS1)|(1 << REFS0);
	
	// // ADC Conversion Result is right adjusted. Select channel input ADC0.
	ADMUX &= ~((1 << ADLAR)|(1 << MUX3)|(1 << MUX2)|(1 << MUX1)|(1 << MUX0));
	
	// ADC Prescaler of 128. 
	ADCSRA |= (1 << ADPS2)|(1 << ADPS1)|(1 << ADPS0);
	
	// Digital Input Disable.
	DIDR0 |= (1<< ADC5D)|(1<< ADC4D)|(1<< ADC3D)|(1<< ADC2D)|(1<< ADC1D)|(1<< ADC0D);
}
//-----------------------------------------------------------------------------
// Enable
//-----------------------------------------------------------------------------
void tensiometerEnable()
{
	// ADC Enable. ADC Conversion Complete Interrupt Enable.
	ADCSRA |= (1 << ADEN)|(1 << ADIE);
}
//-----------------------------------------------------------------------------
// Disable
//-----------------------------------------------------------------------------
void tensiometerDisable()
{
	// ADC Disable. ADC Conversion Complete Interrupt Disable.
	ADCSRA &= ~((1 << ADEN)|(1 << ADIE));
}
//-----------------------------------------------------------------------------
// Single Analog-to-Digital Conversion
//-----------------------------------------------------------------------------
void adSingleConversion()
{
	// ADC Start Conversion.
	ADCSRA |= (1 << ADSC);
	
	// Wait conversion.
	conversionComplete = 0;
	while(!conversionComplete);
}
//-----------------------------------------------------------------------------
// Interrupt Handlers
//-----------------------------------------------------------------------------
ISR(ADC_vect)
{
	tensiometerMeasure = (ADCH << 8) + ADCL;
	conversionComplete = 1;
}

/* Debug Leds, Resources: IO pins 3 and 4 */
//-----------------------------------------------------------------------------
// Configuration
//-----------------------------------------------------------------------------
void debugLedsConfig()
{
	// Setting pins as output
	DDRD |= (1 << DDD4)|(1 << DDD3);
	
	// Initial state: turned off
	PORTD |= (1 << PORTD4)|(1 << PORTD3);
}
//-----------------------------------------------------------------------------
// OK (green) LED
//-----------------------------------------------------------------------------
void raiseOKLed()
{
	PORTD |=  (1 << PORTD4);
	PORTD &= ~(1 << PORTD3);
};
//-----------------------------------------------------------------------------
// Warning (red) LED
//-----------------------------------------------------------------------------
void raiseWarningLed()
{
	PORTD &= ~(1 << PORTD4);
	PORTD |=  (1 << PORTD3);
};

/* SD Card, Resources: SPI - Serial Peripheral Interface */
//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------
uint8_t spiTransferComplete = 0;
//-----------------------------------------------------------------------------
// Config
//-----------------------------------------------------------------------------
void SDCardConfig()
{
	// The MSB of the data word is transmitted first. SPI mode 0.
	SPCR &= ~((1 << DORD)|(1 << CPOL)|(1 << CPHA));
	
	// Master SPI mode.
	SPCR |= (1 << MSTR);
	
	// Pin configurations.
	DDRB |= (1<<DDB2)|(1<<DDB3)|(1<<DDB5);
	DDRB &= ~(1 << DDB4);
	
	// CLock rate of 250 Hz (prescaler of 64).
	SPSR &= ~(1 << SPI2X);
	SPCR |=  (1 << SPR1);
	SPCR &= ~(1 << SPR0);
}
//-----------------------------------------------------------------------------
// Send byte to MOSI line, wait transmission, and return received byte by MISO
//-----------------------------------------------------------------------------
uint8_t spiTransfer(uint8_t byte)
{
	spiTransferComplete = 0;
	SPDR = byte;
	while(!spiTransferComplete);
	return SPDR;
}
//-----------------------------------------------------------------------------
// Interrupt Handlers
//-----------------------------------------------------------------------------
ISR(SPI_STC_vect)
{
	spiTransferComplete = 1;
}
//-----------------------------------------------------------------------------
// Enable
//-----------------------------------------------------------------------------
void SDCardEnable()
{
	// SPI Interrupt Enable. SPI Enable.
	SPCR |= (1 << SPIE)|(1 << SPE);
}
//-----------------------------------------------------------------------------
// Disable
//-----------------------------------------------------------------------------
void SDCardDisable()
{
	// SPI Interrupt Enable. SPI Enable.
	SPCR &= ~((1 << SPIE)|(1 << SPE));
}
//-----------------------------------------------------------------------------
// Initialization
//-----------------------------------------------------------------------------
void SDCardInit()
{
	// Power on to native SD.
	PORTB |= (1 << PORTB2);
	for(uint8_t k=0; k < 10; k++)
		SPDR = 0xFF;
	
	// Software reset (CMD0).
	uint8_t R1 = 0;
	while(!R1)
	{
		PORTB &= ~(1 << PORTB2);
		
		spiTransfer(0x40);
		for(uint8_t k = 0; k < 4; k++)
			spiTransfer(0x00);
		spiTransfer(0x95);		
		R1 = spiTransfer(0xFF);
		
		PORTB |= (1 << PORTB2);
	}
	
	// Initialization process (CMD1).
	while(R1)
	{
		PORTB &= ~(1 << PORTB2);
		
		spiTransfer(0x40 + 0x01);
		for(uint8_t k = 0; k < 4; k++)
			spiTransfer(0x00);
		spiTransfer(0x01);
		R1 = spiTransfer(0xFF);
		
		PORTB |= (1 << PORTB2);
	}
	
	//// Maximize SPI clock speed (8 MHz, prescaler of 2).
	//SPSR |= (1 << SPI2X);
	//SPCR &= ~((1 << SPR1)|(1 << SPR0));
}
//-----------------------------------------------------------------------------
// Write a single block of 512 bytes
//-----------------------------------------------------------------------------
void writeSingleBlock(uint8_t* address, uint8_t* data)
{
	PORTB &= ~(1 << PORTB2);
	
	// CMD24
	spiTransfer(0x40 + 0x24);
	for(uint8_t k = 0; k < 4; k++)
		spiTransfer(address[k]);
	spiTransfer(0x00);
	uint8_t R1 = spiTransfer(0xFF);
	
	// Wait 1 byte
	spiTransfer(0xFF);
	
	// Data Packet
	spiTransfer(0xFE); // Data Token
	for(uint8_t k = 0; k < 512; k++)
		spiTransfer(data[k]);
	spiTransfer(0x00); spiTransfer(0x00); // CRC
	uint8_t dataResponse = spiTransfer(0xFF);
	
	PORTB |= (1 << PORTB2);
}

/* Sleep, Resources: Timer 1 */
//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------
uint64_t ovfCounter = 0;
//-----------------------------------------------------------------------------
// Configuration
//-----------------------------------------------------------------------------
void sleepConfig()
{
	// Normal port operation, OC1A/OC1B disconnected.
	TCCR1A &= ~((1 << COM1A1)|(1 << COM1A0)|(1 << COM1B0)|(1 << COM1B0));
	
	// Normal mode (counter)
	TCCR1B &= ~((1 << WGM13)|(1 << WGM12));
	TCCR1A &= ~((1 << WGM11)|(1 << WGM10));
	
	// Input Capture Noise Canceler disabled.
	TCCR1B &= ~(1 << ICNC1);
	
	// Input Capture, Output Compare B/A Match Interrupts disabled.
	TIMSK1 &= ~((1 << ICIE1)|(1 << OCIE1B)|(1 << OCIE1A));
	
	// Timer1 Overflow Interrupt Enable.
	TIMSK1 |= (1 << TOIE1);
}
//-----------------------------------------------------------------------------
// Start counter
//-----------------------------------------------------------------------------
void sleepStartCounter()
{
	// Set clock prescaler to 1024 (15.625 kHz, or T = 64 ms).
	TCCR1B |= (1 << CS12)|(1 << CS10);
	TCCR1B &= ~(1 << CS11);
}
//-----------------------------------------------------------------------------
// Stop counter
//-----------------------------------------------------------------------------
void sleepStopCounter()
{
	TCCR1B &= ~((1 << CS12)|(1 << CS11)|(1 << CS10));
}
//-----------------------------------------------------------------------------
// Sleep
//-----------------------------------------------------------------------------
void sleep(int hours)
{
	//TODO
}
//-----------------------------------------------------------------------------
// Interrupt Handlers
//-----------------------------------------------------------------------------
ISR(TIMER1_OVF_vect)
{
	ovfCounter++;
}

/* Main */
int main()
{
	// Initialization
	pluviometerConfig();
	pluviometerEnable();
	
	tensiometerConfig();
	tensiometerEnable();
	
	debugLedsConfig();
	sleepConfig();
	
	SDCardConfig();
	SDCardEnable();
	SDCardInit();
	
	sei();
	
	//TODO: main loop
	//1) capture data
	//2) write a data block
	//3) disable tensiometer and SD Card
	//4) sleep
	//5) enable tensiometer and SD Card
	
	return 0;
};