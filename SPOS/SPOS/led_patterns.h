/*! \file
 *  \brief CUSTOM CHAR PATTERNS
 *  \author Lehrstuhl Informatik 11 - RWTH Aachen
 */

#include <avr/pgmspace.h>

#define LED_CHAR_HEIGHT_SMALL 5
#define LED_CHAR_WIDTH_SMALL 3
#define LED_CHAR_HEIGHT_LARGE 8
#define LED_CHAR_WIDTH_LARGE 5

//! Defines a custom CHAR out of eight rows passed as integer values
#define LED_CUSTOM_CHAR(cc0, cc1, cc2, cc3, cc4, cc5, cc6, cc7) 0 | (((uint64_t)(cc0)) << 0 * 8) | (((uint64_t)(cc1)) << 1 * 8) | (((uint64_t)(cc2)) << 2 * 8) | (((uint64_t)(cc3)) << 3 * 8) | (((uint64_t)(cc4)) << 4 * 8) | (((uint64_t)(cc5)) << 5 * 8) | (((uint64_t)(cc6)) << 6 * 8) | (((uint64_t)(cc7)) << 7 * 8)


//! Helper macros to define a large (7x5) and small(5x3) character without padding. Will be aligned in the top-left corner of the resulting custom char
#define LED_CUSTOM_CHAR_SMALL(row1, row2, row3, row4, row5) LED_CUSTOM_CHAR(row1 << 5, row2 << 5, row3 << 5, row4 << 5, row5 << 5, 0, 0, 0)
#define LED_CUSTOM_CHAR_LARGE(r0, r1, r2, r3, r4, r5, r6) LED_CUSTOM_CHAR(r0 << 3, r1 << 3, r2 << 3, r3 << 3, r4 << 3, r5 << 3, r6 << 3, 0)

#define MATRIX_1 (LED_CUSTOM_CHAR( \
    0b11111111,                    \
    0b10000001,                    \
    0b10000001,                    \
    0b10000001,                    \
    0b10000001,                    \
    0b10000001,                    \
    0b10000001,                    \
    0b11111111                     \
))

#define PATTERN_INVALID (LED_CUSTOM_CHAR( \
    0b11111111,                           \
    0b11000011,                           \
    0b10100101,                           \
    0b10011001,                           \
    0b10011001,                           \
    0b10100101,                           \
    0b11000011,                           \
    0b11111111                            \
))

#define PATTERN_HOURGLASS (LED_CUSTOM_CHAR( \
    0b01111110,                             \
    0b01000010,                             \
    0b00100100,                             \
    0b00011000,                             \
    0b00011000,                             \
    0b00100100,                             \
    0b01000010,                             \
    0b01111110                              \
))

#define PATTERN_TICKS_PER_SECOND (LED_CUSTOM_CHAR( \
    0b11111001,                                    \
    0b00100010,                                    \
    0b00100100,                                    \
    0b00101011,                                    \
    0b00010100,                                    \
    0b00100010,                                    \
    0b01000001,                                    \
    0b10000110                                     \
))

// - - -

#define PATTERN_ARROW_LEFT (LED_CUSTOM_CHAR( \
    0b00010000,                              \
    0b00110000,                              \
    0b01110000,                              \
    0b11111111,                              \
    0b11111111,                              \
    0b01110000,                              \
    0b00110000,                              \
    0b00010000                               \
))

#define PATTERN_ARROW_UP (LED_CUSTOM_CHAR( \
    0b00011000,                            \
    0b00111100,                            \
    0b01111110,                            \
    0b11111111,                            \
    0b00011000,                            \
    0b00011000,                            \
    0b00011000,                            \
    0b00011000                             \
))

#define PATTERN_ARROW_RIGHT (LED_CUSTOM_CHAR( \
    0b00001000,                               \
    0b00001100,                               \
    0b00001110,                               \
    0b11111111,                               \
    0b11111111,                               \
    0b00001110,                               \
    0b00001100,                               \
    0b00001000                                \
))

#define PATTERN_ARROW_DOWN (LED_CUSTOM_CHAR( \
    0b00011000,                              \
    0b00011000,                              \
    0b00011000,                              \
    0b00011000,                              \
    0b11111111,                              \
    0b01111110,                              \
    0b00111100,                              \
    0b00011000                               \
))

//! Patterns for the letters and numbers
// we store these in arrays since that makes it easier to find
// the corresponding pattern to an ascii char
// (which is how these are used 99% of the time)
const extern uint64_t PROGMEM led_small_letters[];
const extern uint64_t PROGMEM led_large_letters[];
const extern uint64_t PROGMEM led_small_numbers[];
const extern uint64_t PROGMEM led_large_numbers[];
