/*! \file
 *  \brief Analog digital converter
 *
 *  \author Lehrstuhl Informatik 11 - RWTH Aachen
 */

#include "lcd.h"
#include "menu.h"
#include "os_input.h"
#include "led.h"
#include <util/delay.h>

int main(void) {
    // 1. Initialize the buttons
    os_initInput();

    // 2. Initialize LCD
    lcd_init();
    // 3. Show menu
	showMenu();
}
