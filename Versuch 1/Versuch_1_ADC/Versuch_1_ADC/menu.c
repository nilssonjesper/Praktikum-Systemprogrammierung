#include "menu.h"

#include "adc.h"
#include "bin_clock.h"
#include "lcd.h"
#include "led.h"
#include "os_input.h"
#include "help.h"

#include <avr/io.h>
#include <stdint.h>
#include <stdio.h>
#include <util/delay.h>

/*!
 *  Hello world program.
 *  Shows the string 'Hello World!' on the display.
 */
void helloWorld(void) {
	while((os_getInput() & (1 << 7)) == 0){
		lcd_writeProgString(PSTR("Hallo Welt!"));
		for(int i = 0; i < 50 && (os_getInput() & (1 << 7)) == 0; i++){
			_delay_ms(10);
		}
		lcd_clear();
		for(int i = 0; i < 50 && (os_getInput() & (1 << 7)) == 0; i++){
			_delay_ms(10);
		}
	}
	return;
}

/*!
 *  Shows the clock on the display and a binary clock on the led bar.
 */
void displayClock(void) {
	while((os_getInput() & (1 << 7)) == 0){
		char buffer[16];
		uint8_t hours = getTimeHours();
		uint8_t minutes = getTimeMinutes();
		uint8_t seconds = getTimeSeconds();
		uint16_t milliseconds = getTimeMilliseconds();
		
		uint16_t clockVal = hours << 12;
		clockVal |= (minutes & 0x3F) << 6;
		clockVal |= seconds & 0x3F;
		
		setLedBar(clockVal);
		
 		sprintf(buffer, "%02d:%02d:%02d:%03d", hours, minutes, seconds, milliseconds);
 		
		lcd_clear();
		lcd_writeString(buffer);
		_delay_ms(50);
	}
}

/*!
 *  Shows the stored voltage values in the second line of the display.
 */
void displayVoltageBuffer(uint8_t displayIndex) {

	char buffer[16];
	lcd_line2();
	sprintf(buffer ,"%03u/100: ", displayIndex);
	lcd_writeString(buffer);
	lcd_writeVoltage(getStoredVoltage(displayIndex), 1023, 5);
}

/*!
 *  Shows the ADC value on the display and on the led bar.
 */
void displayAdc(void) {
	uint8_t pageIndex = 1;

	while((os_getInput() & (1 << 7)) == 0){
		uint16_t ledValue = 0;
		uint16_t adcResult = getAdcValue();
	
		for(uint16_t i = adcResult; i >= 68; i -= 68){
			ledValue <<= 1;
			ledValue |= 2;
		}
	
		setLedBar(ledValue);
		lcd_clear();
		lcd_writeString("Voltage: ");
		lcd_writeVoltage(adcResult, 1023, 5);
		displayVoltageBuffer(pageIndex);
		
		if (os_getInput() == 0b00000001) { // Enter
			storeVoltage();
			} 
			
		else if (os_getInput() == 0b01000000) { // Up
			pageIndex = (pageIndex) % 100 + 1;
			} 
			
		else if (os_getInput() == 0b00000010) { // Down
			if (pageIndex == 1) {
				pageIndex = 100;
				} else {
				pageIndex--;
			}
		}
		_delay_ms(100);
	}
}

/*! \brief Starts the passed program
 *
 * \param programIndex Index of the program to start.
 */
void start(uint8_t programIndex) {
    // Initialize and start the passed 'program'
    switch (programIndex) {
        case 0:
            lcd_clear();
            helloWorld();
            break;
        case 1:
            activateLedMask = 0xFFFF; // Use all LEDs
            initLedBar();
            initClock();
            displayClock();
			setLedBar(0);
            break;
        case 2:
            activateLedMask = 0xFFFE; // Don't use LED 0
            initLedBar();
            initAdc();
            displayAdc();
			setLedBar(0);
            break;
        default:
            break;
    }

    // Do not resume to the menu until all buttons are released
    os_waitForNoInput();
}

/*!
 *  Shows a user menu on the display which allows to start subprograms.
 */
void showMenu(void) {
    uint8_t pageIndex = 0;

    while (1) {
        lcd_clear();
        lcd_writeProgString(PSTR("Select:"));
        lcd_line2();

        switch (pageIndex) {
            case 0:
                lcd_writeProgString(PSTR("1: Hello world"));
                break;
            case 1:
                lcd_writeProgString(PSTR("2: Binary clock"));
                break;
            case 2:
                lcd_writeProgString(PSTR("3: Internal ADC"));
                break;
            default:
                lcd_writeProgString(PSTR("----------------"));
                break;
        }

        os_waitForInput();
        if (os_getInput() == 0b00000001) { // Enter
            os_waitForNoInput();
            start(pageIndex);
        } else if (os_getInput() == 0b01000000) { // Up
            os_waitForNoInput();
            pageIndex = (pageIndex + 1) % 3;
        } else if (os_getInput() == 0b00000010) { // Down
            os_waitForNoInput();
            if (pageIndex == 0) {
                pageIndex = 2;
            } else {
                pageIndex--;
            }
        }
    }
}
