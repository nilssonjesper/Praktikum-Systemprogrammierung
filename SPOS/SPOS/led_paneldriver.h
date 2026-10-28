/*! \file
 *  \brief Low level functions to draw premade things on the LED Panel
 *  \author Lehrstuhl Informatik 11 - RWTH Aachen
 */
#ifndef _LED_PANELDRIVER_H
#define _LED_PANELDRIVER_H
#include <avr/io.h>
#include <stdbool.h>


//! Initializes registers
void panel_init();

//! Starts interrupts
void panel_startTimer(void);

//! Stops interrupts
void panel_stopTimer(void);

//! Initalizes interrupt timer
void panel_initTimer(void);
#endif
