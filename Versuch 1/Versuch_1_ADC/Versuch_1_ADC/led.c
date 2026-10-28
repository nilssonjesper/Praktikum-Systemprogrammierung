#include "led.h"

#include <avr/io.h>
#include <util/delay.h>

uint16_t activateLedMask = 0xFFFF;

/*!
 *  Initializes the led bar. Note: All PORTs will be set to output.
 */
void initLedBar(void) {
	DDRD |= (activateLedMask & 0xFF);
	DDRA |= (activateLedMask & 0xFF);
	
	PORTA  &= ~activateLedMask;
	PORTD  &= ~activateLedMask;
	
	_delay_ms(500);
	
	PORTA |= activateLedMask;
	PORTD |= activateLedMask;
}

/*!
 *  Sets the passed value as states of the led bar (1 = on, 0 = off).
 */
void setLedBar(uint16_t value) {
	PORTA |= activateLedMask;
	PORTD |= activateLedMask;
	
	PORTA &= ~(activateLedMask & value);
	PORTD &= ~(activateLedMask & value) >> 8;
}
