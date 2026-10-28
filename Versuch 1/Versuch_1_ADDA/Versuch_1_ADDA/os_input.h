/*! \file
 *  \brief Button input library.
 *
 *  Contains functionalities to read user input.
 *
 *  \author Lehrstuhl Informatik 11 - RWTH Aachen
 */

#ifndef _OS_INPUT_H
#define _OS_INPUT_H

#include <stdint.h>

//----------------------------------------------------------------------------
// Function headers
//----------------------------------------------------------------------------

//! Initializes the respective Pins for the buttons, i.e. sets DDR and Pull ups
void os_initInput(void);

//! Refreshes the button states
uint8_t os_getInput(void);

//! Waits for all buttons to be released
void os_waitForNoInput(void);

//! Waits for at least one button to be pressed
void os_waitForInput(void);

#endif
