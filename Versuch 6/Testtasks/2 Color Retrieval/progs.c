//-------------------------------------------------
//          TestTask: Color Retrieval
//-------------------------------------------------

#include "defines.h"
#include "lcd.h"
#include "led_draw.h"
#include "led_paneldriver.h"
#include "led_patterns.h"
#include "os_core.h"
#include "os_process.h"
#include "os_scheduler.h"
#include "util.h"

#include <avr/interrupt.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>

#if VERSUCH < 6
#warning "Please fix the VERSUCH-define"
#endif

/*
 * Map colors to pixels. For this test to be effective, every color value must be shown at least once for
 * y < 16 and y >= 16. Also, col_for_px(x, y) should be different from col_for_px(x, y = 16).
 * This is due to the double-row structure - we want to test whether a given color can be retrieved successfully from
 * either part of a double row
 */
static inline Color col_for_px(uint8_t x, uint8_t y) {
    uint8_t col_packed = x | y << 5;
    if (y > 15) col_packed ^= 0b00111111;

    return (Color){col_packed << 2 & 0b11000000, col_packed << 4 & 0b11000000, col_packed << 6 & 0b11000000};
}

void show_fail_screen(uint8_t fail_x, uint8_t fail_y, Color expected, Color retrieved) {
    draw_clearDisplay();
    lcd_clear();
    fprintf_P(lcdout, PSTR("FAIL X:%02d Y:%02d\nE%02x%02x%02x  R%02x%02x%02x"), fail_x, fail_y, expected.r, expected.g, expected.b, retrieved.r, retrieved.g, retrieved.b);

    uint8_t x = 0, y = 0;
    for (const char *curr = "EXPECT"; *curr; curr++) {
        draw_letter(*curr, x, y, COLOR_WHITE, false, false);
        x += LED_CHAR_WIDTH_SMALL + 1;
    }

    y = 26;
    x = 17;

    for (const char *curr = "READ"; *curr; curr++) {
        draw_letter(*curr, x, y, COLOR_WHITE, false, false);
        x += LED_CHAR_WIDTH_SMALL + 1;
    }

    draw_filledRectangle(7, 7, 24, 24, COLOR_WHITE);
    draw_filledRectangle(8, 8, 15, 23, expected);
    draw_filledRectangle(16, 8, 23, 23, retrieved);
    draw_pattern(0, 8, 8, 7, LED_CUSTOM_CHAR(0b01000000, 0b01000000, 0b01000000, 0b01010000, 0b01001000, 0b01111100, 0b00001000, 0b00010000), COLOR_WHITE, false);

    draw_pattern(25, 17, 8, 7, LED_CUSTOM_CHAR(0b00010000, 0b00100000, 0b01111100, 0b00100100, 0b00010100, 0b00000100, 0b00000100, 0b00000100), COLOR_WHITE, false);
    HALT;
}

void show_pass_screen(void) {
    lcd_clear();

    // draw_filledRectangle(3, 9, 3 + 6 * LED_CHAR_WIDTH_SMALL + 6, 9 + 2 * LED_CHAR_HEIGHT_SMALL + 2, COLOR_BLACK);

    uint8_t x = 7, y = 10;
    for (const char *curr = "TEST"; *curr; curr++) {
        draw_letter(*curr, x, y, COLOR_BLACK, false, false);
        x += LED_CHAR_WIDTH_SMALL + 2;
    }

    y += LED_CHAR_HEIGHT_SMALL + 1;
    x = 2;

    for (const char *curr = "PASSED"; *curr; curr++) {
        draw_letter(*curr, x, y, COLOR_BLACK, false, false);
        x += LED_CHAR_WIDTH_SMALL + 2;
    }

    lcd_writeProgString(PSTR("  TEST PASSED   "));
    HALT;
}

REGISTER_AUTOSTART(program1)
void program1(void) {
    os_setSchedulingStrategy(OS_SS_RUN_TO_COMPLETION);

    panel_init();
    panel_initTimer();
    panel_startTimer();

    lcd_clear();
    lcd_writeProgString(PSTR("Color Retrieval "));

    for (uint8_t x = 0; x < 32; x++) {
        for (uint8_t y = 0; y < 32; y++) {
            Color expected = col_for_px(x, y);
            draw_setPixel(x, y, expected);
        }
    }


    // wait for some delay so the test pattern is visible
    delayMs(DEFAULT_OUTPUT_DELAY * 10);

    for (uint8_t x = 0; x < 32; x++) {
        for (uint8_t y = 0; y < 32; y++) {
            Color expected = col_for_px(x, y);
            // save the received color for the unlikely case that the student implementation
            // succeeds the second time when it failed at first
            Color received = draw_getPixel(x, y);

            if (memcmp(&expected, &received, sizeof(Color))) {
                show_fail_screen(x, y, expected, received);
            }
        }
    }


    show_pass_screen();
}
