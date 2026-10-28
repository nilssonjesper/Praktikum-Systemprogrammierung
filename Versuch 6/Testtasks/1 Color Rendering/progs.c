//-------------------------------------------------
//          TestTask: Color Rendering
//-------------------------------------------------

#include "lcd.h"
#include "led_draw.h"
#include "led_paneldriver.h"
#include "led_patterns.h"
#include "os_core.h"
#include "os_input.h"
#include "os_memory.h"
#include "os_scheduler.h"
#include "util.h"

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>

#if VERSUCH < 6
#warning "Please fix the VERSUCH-define"
#endif

//---- Adjust here what to test -------------------
#define PHASE_1 1
#define PHASE_2 1
#define PHASE_3 1

// if set to any of 1 - 3, the test will only show that phase and freeze.
// This overrides the selection above.
// #define SHOW_AND_HALT_PHASE 3
//-------------------------------------------------

/*
 * The PANEL_SHOW macro will enable the timer to show the changed framebuffer
 * on the screen, wait for some amount of time and then disable the timer again
 * This ensures that the screen is not refreshed while we are in the middle
 * of drawing something to the framebuffer, which can produce artifacts
 * If SHOW_AND_HALT_PHASE is defined to a value other than 0,
 * this will halt indefinitely while showing the display
 */
#if SHOW_AND_HALT_PHASE
#define PANEL_SHOW      \
    panel_startTimer(); \
    HALT;               \
    panel_stopTimer();
#else
#define PANEL_SHOW                      \
    panel_startTimer();                 \
    delayMs(DEFAULT_OUTPUT_DELAY * 50); \
    panel_stopTimer();
#endif

#define NUM_COLORS (sizeof(colors) / sizeof(Color))
const Color colors[] = {
    COLOR_WHITE,
    COLOR_RED,
    COLOR_DARKRED,
    COLOR_GREEN,
    COLOR_DARKGREEN,
    COLOR_BLUE,
    COLOR_DARKBLUE,
    COLOR_PINK,
    COLOR_YELLOW,
    COLOR_TURQUOISE,
    COLOR_BLACK};

REGISTER_AUTOSTART(program1)
void program1(void) {
    os_setSchedulingStrategy(OS_SS_RUN_TO_COMPLETION);
    panel_init();
    panel_initTimer();

    lcd_clear();
    lcd_writeProgString(PSTR("Color Rendering "));

#if PHASE_1 && (!SHOW_AND_HALT_PHASE || SHOW_AND_HALT_PHASE == 1)
    lcd_line2();
    lcd_writeProgString(PSTR("Phase 1"));

    // Set all pixels (white)
    for (uint8_t c = 0; c < 32; c++) {
        for (uint8_t r = 0; r < 32; r++) {
            draw_setPixel(c, r, COLOR_WHITE);
        }
    }
    PANEL_SHOW;

#endif

#if PHASE_2 && (!SHOW_AND_HALT_PHASE || SHOW_AND_HALT_PHASE == 2)
    lcd_line2();
    lcd_writeProgString(PSTR("Phase 2"));

    // Different brightness levels
    for (uint8_t c = 0; c < 32; c++) {
        for (uint8_t r = 0; r < 32; r++) {
            /*
             * We require the display to be able to show at least 4 different intensities
             * by using 2 layers. Even with weighted layers, it is still hard to differentiate
             * between bright colors. Therefore, we split the panel into 4 quadrants, where
             * each gets a different intensity, such that we maximize intersections between
             * different colors and make it a bit easier to see.
             * The intended result will look a little like this (bigger characters -> more intense)
             *                 ................
             *                 ................
             *                 ................
             *                 ................
             *                 ................
             *                 ................
             *                 ................
             * ****************################
             * ****************################
             * ****************################
             * ****************################
             * ****************################
             * ****************################
             * ****************################
             * we use a little bit-shifting to put the colors into the quadrants
             * MSB (7th bit) of color will be the 4th bit of the row (counted from 0)
             * this will be 1 if row >= 16, 0 otherwise
             * second-MSB (6th bit) of color will be 4th bit of the column (counted from 0)
             */

            // r: 00010000
            // c: 00010000
            //  : rc
            /*
                0b00000000 0b01000000
                0b10000000 0b11000000
            */

            uint8_t intensity = ((r << 3) & (1 << 7)) | ((c << 2) & (1 << 6));
            draw_setPixel(c, r, (Color){intensity, intensity, intensity});
        }
    }
    PANEL_SHOW;
#endif

#if PHASE_3 && (!SHOW_AND_HALT_PHASE || SHOW_AND_HALT_PHASE == 3)
    lcd_line2();
    lcd_writeProgString(PSTR("Phase 3"));

    draw_clearDisplay();

    // Display available colors
    // colors
    for (uint8_t r = 0; r < 32; r++) {
        for (uint8_t c = 0; c < NUM_COLORS - 1; c++) {
            draw_setPixel(3 * c, r, colors[c]);
            draw_setPixel(3 * c + 1, r, colors[c]);
            draw_setPixel(3 * c + 2, r, COLOR_BLACK);
        }
    }

    PANEL_SHOW;
#endif

    // last phase should be displayed indefinitely
    panel_startTimer();
    HALT;
}
