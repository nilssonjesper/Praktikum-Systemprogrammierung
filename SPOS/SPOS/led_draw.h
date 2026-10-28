
/*! \file
 *  \brief High level functions to draw premade things on the LED Panel
 *  \author Lehrstuhl Informatik 11 - RWTH Aachen
 */
#ifndef _LED_DRAW_H
#define _LED_DRAW_H
#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Color;

//! Used by Testtask to draw Arrow Symbol
void draw_pattern(uint8_t x, uint8_t y, uint8_t height, uint8_t width, uint64_t pattern, Color color, bool overwrite);

//! Draws Decimal (0..9) on panel
void draw_decimal(uint8_t dec, uint8_t x, uint8_t y, Color color, bool overwrite, bool large);

//! Draws capital letter on panel
void draw_letter(char letter, uint8_t x, uint8_t y, Color color, bool overwrite, bool large);

//! Draws an integer on panel
void draw_number(uint32_t number, bool right_align, uint8_t x, uint8_t y, Color color, bool overwrite, bool large);

void draw_setPixel(uint8_t x, uint8_t y, Color color);
Color draw_getPixel(uint8_t x, uint8_t y);
void draw_fillPanel(Color color);
void draw_clearDisplay();
void draw_filledRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, Color color);

#define COLOR_WHITE ((Color){.r = 0xFF, .g = 0xFF, .b = 0xFF})
#define COLOR_RED ((Color){.r = 0xFF, .g = 0x00, .b = 0x00})
#define COLOR_DARKRED ((Color){.r = 0x40, .g = 0x00, .b = 0x00})
#define COLOR_GREEN ((Color){.r = 0x00, .g = 0xFF, .b = 0x00})
#define COLOR_DARKGREEN ((Color){.r = 0x00, .g = 0x40, .b = 0x00})
#define COLOR_BLUE ((Color){.r = 0x00, .g = 0x00, .b = 0xFF})
#define COLOR_DARKBLUE ((Color){.r = 0x00, .g = 0x00, .b = 0x40})
#define COLOR_PINK ((Color){.r = 0xFF, .g = 0x00, .b = 0xFF})
#define COLOR_YELLOW ((Color){.r = 0xFF, .g = 0xFF, .b = 0x00})
#define COLOR_TURQUOISE ((Color){.r = 0x00, .g = 0xFF, .b = 0xFF})
#define COLOR_BLACK ((Color){.r = 0x00, .g = 0x00, .b = 0x00})
#endif
