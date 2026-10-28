/*! \file
 *  \brief Functions to draw premade things on the LED Panel
 *  \author Lehrstuhl Informatik 11 - RWTH Aachen
 */
#include "led_paneldriver.h"

#include "defines.h"
#include "util.h"

#include <avr/interrupt.h>
#include <stdbool.h>
#include <util/delay.h>


//! \brief Enable compare match interrupts for Timer 1
void panel_startTimer() {
    sbi(TIMSK1, OCIE1A);
}

//! \brief Disable compare match interrupts for Timer 1
void panel_stopTimer() {
    cbi(TIMSK1, OCIE1A);
}

//! \brief Initialization function of Timer 1
void panel_initTimer() {
    // Configuration TCCR1B register
    sbi(TCCR1B, WGM12); // Clear on timer compare match
    sbi(TCCR1B, CS12);  // Prescaler 256  1
    cbi(TCCR1B, CS11);  // Prescaler 256  0
    cbi(TCCR1B, CS10);  // Prescaler 256  0

    // Output Compare register 256*7 = 1792 tics => interrupt interval approx 0.0896 ms
    OCR1A = 0x0007;
}

//! \brief Initializes used ports of panel
void panel_init(){
#error IMPLEMENT STH. HERE
}


//! \brief ISR to refresh LED panel, trigger 1 compare match interrupts
ISR(TIMER1_COMPA_vect) {
#error IMPLEMENT STH. HERE
}
