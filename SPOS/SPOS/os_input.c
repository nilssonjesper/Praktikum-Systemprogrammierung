#include "os_input.h"
#include <avr/io.h>
#include <stdint.h>

/*! \file

Everything that is necessary to get the input from the Buttons in a clean format.

*/

/*!
 *  A simple "Getter"-Function for the Buttons on the evaluation board.\n
 *
 *  \returns The state of the button(s) in the lower bits of the return value.\n
 *  example: 1 Button:  -pushed:   00000001
 *                      -released: 00000000
 *           4 Buttons: 1,3,4 -pushed: 000001101
 *
 */
uint8_t os_getInput(void) {
    uint8_t pinState = PINC & 0b11000011;
    uint8_t upperButtons = (pinState >> 4) & 0b00001100; // -> 0000BB00
	uint8_t lowerButtons = pinState & 0b00000011;
	
	uint8_t result = upperButtons | lowerButtons; // -> 0000BB00 + 000000BB = 0000BBBB

	return ~result & 0b00001111; //1111bbbb
}

/*!
 *  Initializes DDR and PORT for input
 */
void os_initInput() {
	// Set PIN to incoming
	DDRC &= 0b00111100;
	
	// Set PORT to 1
	PORTC |= 0b11000011;
	return;
}

/*!
 *  Endless loop as long as at least one button is pressed.
 */
void os_waitForNoInput() {
while((os_getInput() & 0b00001111) != 0){
	continue;
	}
return;
}

/*!
 *  Endless loop until at least one button is pressed.
 */
void os_waitForInput() {
	while ((os_getInput() & 0b00001111) == 0) {
		continue;
	}
}