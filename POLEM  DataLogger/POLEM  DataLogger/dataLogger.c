//-----------------------------------------------------------------------------
// POLEM DataLogger
//	Author: Vicente Cunha
//	Date: june 2015
//	License: --
//-----------------------------------------------------------------------------
#include <avr/io.h>
#include <avr/interrupt.h>

/* Pluviometer */
//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------
uint64_t pluviometerCounter = 0;
//-----------------------------------------------------------------------------
// Initialization
//	Resources: external interrupt 0 (INT0)
//-----------------------------------------------------------------------------
void pluviometerInit()
{
	// PD2 is input with pull-up.
	DDRD  &= ~(1 << DDD2);
	PORTD |=  (1 << PORTD2);
	MCUCR &= ~(1 << PUD);
	
	// The falling edge of INT0 generates an interrupt request.
	EICRA |=  (1 << ISC01);
	EICRA &= ~(1 << ISC00);
	
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

/* Tensiometer */
//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------
uint16_t tensiometerMeasure = 0;
uint8_t conversionComplete = 0;
//-----------------------------------------------------------------------------
// Initialization
//	Resources: Analog-to-Digital Converter
//-----------------------------------------------------------------------------
void tensiometerInit()
{
	// Internal 1.1V Voltage Reference with external capacitor at AREF pin.
	ADMUX |= (1 << REFS1)|(1 << REFS0);
	
	// ADC Conversion Result is right adjusted.
	ADMUX &= ~(1 << ADLAR);
	
	// Select channel input ADC0.
	ADMUX &= ~((1 << MUX3)|(1 << MUX2)|(1 << MUX1)|(1 << MUX0));
		
	// ADC Prescaler of 128.
	ADCSRA |= (1 << ADPS2)|(1 << ADPS1)|(1 << ADPS0);
	
	// Digital Input Disable.
	DIDR0 |= (1<< ADC5D)|(1<< ADC4D)|(1<< ADC3D)|(1<< ADC2D)|(1<< ADC1D)|(1<< ADC0D);
	
	// ADC Enable.
	ADCSRA |= (1 << ADEN);
	
	// ADC Conversion Complete Interrupt Enable.
	ADCSRA |= (1 << ADIE);
}
//-----------------------------------------------------------------------------
// Single Analog-to-Digital Conversion
//-----------------------------------------------------------------------------
void adConversion()
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

/* SD Card */
//-----------------------------------------------------------------------------
//	Resources: SPI - Serial Peripheral Interface
//-----------------------------------------------------------------------------
void SDCardInit()
{
	// The MSB of the data word is transmitted first.
	SPCR &= ~(1 << DORD);
	
	// Master SPI mode.
	SPCR |= (1 << MSTR);
	
	// SPI mode 0.
	SPCR &= ~((1 << CPOL)|(1 << CPHA));
	
	// CLock rate of 250 Hz (prescaler of 64).
	SPSR &= ~(1 << SPI2X);
	SPCR |=  (1 << SPR1);
	SPR0 &= ~(1 << SPR0);
	
	// SPI Interrupt Enable.
	SPCR |= (1 << SPIE);
	
	// SPI Enable.
	SPCR |= (1 << SPE);
}
//-----------------------------------------------------------------------------
// Single Analog-to-Digital Conversion
//-----------------------------------------------------------------------------
void adConversion()
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



/* Hibernate */
//-----------------------------------------------------------------------------
//	Resources: Timer 1
//-----------------------------------------------------------------------------
void hibernate(int hours)
{
	
}

/* Main */
int main()
{
	// Initialization
	pluviometerInit();
	tensiometerInit();
	SDCardInit();
	sei();
	
	// 
	
	return 0;
};