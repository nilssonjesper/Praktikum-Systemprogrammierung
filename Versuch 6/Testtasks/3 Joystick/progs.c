//-------------------------------------------------
//          TestTask: Joystick
//-------------------------------------------------

#include "defines.h"
#include "joystick.h"
#include "lcd.h"
#include "led_draw.h"
#include "led_paneldriver.h"
#include "led_patterns.h"
#include "os_core.h"
#include "os_process.h"
#include "os_scheduler.h"
#include "util.h"

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <math.h>
#include <stdlib.h>
#include <util/atomic.h>

#if VERSUCH < 6
#warning "Please fix the VERSUCH-define"
#endif

const Color I11_BLUE = {.r = 0x00, .g = 0x54, .b = 0x9F};
const Color I11_ORANGE = {.r = 0xFF, .g = 0x40, .b = 0x00};

#define CODELENGTH (sizeof(secretCode) / sizeof(Direction))
Direction secretCode[] = {JS_UP, JS_LEFT, JS_DOWN, JS_RIGHT};

#define PSTR_CLEAR_LINE PSTR("                ")

void secretAnimation();
#define TRAIL 6
typedef struct {
    long double xPos;
    long double yPos;
    long double xVel;
    long double yVel;
    Color cl;
    Color trailCl[TRAIL];
    uint8_t lastx[TRAIL];
    uint8_t lasty[TRAIL];
    bool active;
    uint8_t trailInd;
    bool activeTrail[TRAIL];
} Particle;

void applyGravity(Particle *p, long double t);

void draw_coord(uint16_t coord, uint8_t x, uint8_t y, Color coordColor) {
    uint8_t thous = coord / 1000;
    uint8_t hundr = (coord % 1000) / 100;
    uint8_t tens = (coord % 100) / 10;
    uint8_t ones = coord % 10;

    draw_decimal(thous, x, y, coordColor, true, 0);
    draw_decimal(hundr, x + (LED_CHAR_WIDTH_SMALL + 1), y, coordColor, true, 0);
    draw_decimal(tens, x + 2 * (LED_CHAR_WIDTH_SMALL + 1), y, coordColor, true, 0);
    draw_decimal(ones, x + 3 * (LED_CHAR_WIDTH_SMALL + 1), y, coordColor, true, 0);
}

/*!
 * This program tracks the joystick inputs.
 * It uses the LCD on line 1 only to avoid conflicts with the main program
 */
void pattern_program(void) {
    Direction codeMemory[CODELENGTH];
    uint8_t ind = 0;
    bool codeCorrect;
    bool codeError = false;

    Direction curr = JS_NEUTRAL, prev;
    while (1) {
        os_enterCriticalSection();
        prev = curr;
        curr = js_getDirection();
        os_leaveCriticalSection();

        if (curr == prev && !js_getButton()) {
            os_yield();
            continue;
        }


        if (js_getButton()) {
            for (uint8_t i = 0; i < CODELENGTH; i++) {
                codeMemory[i] = JS_NEUTRAL;
            }
            ind = 0;
            codeError = false;
            os_enterCriticalSection();
            lcd_line1();
            lcd_writeProgString(PSTR_CLEAR_LINE);
            lcd_line1();
            lcd_writeProgString(PSTR("Memory cleared!"));
            os_leaveCriticalSection();
            delayMs(DEFAULT_OUTPUT_DELAY * 3);
            while (js_getButton()) continue;

            os_enterCriticalSection();
            lcd_line1();
            lcd_writeProgString(PSTR_CLEAR_LINE);
            os_leaveCriticalSection();
        }
        if (curr != prev) {
            if (ind < CODELENGTH) {
                if (curr != JS_NEUTRAL) {
                    codeMemory[ind] = curr;

                    if ((codeMemory[ind] == secretCode[ind]) && (codeError == false)) {
                        os_enterCriticalSection();
                        lcd_line1();
                        lcd_writeProgString(PSTR_CLEAR_LINE);
                        lcd_line1();
                        lcd_writeProgString(PSTR("Secret: "));
                        lcd_writeDec(ind + 1);
                        lcd_writeChar('/');
                        lcd_writeDec(CODELENGTH);
                        os_leaveCriticalSection();
                    } else {
                        codeError = true;
                        os_enterCriticalSection();
                        lcd_line1();
                        lcd_writeProgString(PSTR_CLEAR_LINE);
                        os_leaveCriticalSection();
                    }
                    ind++;
                }
            }

            codeCorrect = true;
            for (uint8_t i = 0; i < CODELENGTH; i++) {
                if (codeMemory[i] != secretCode[i]) {
                    codeCorrect = false;
                }
            }

            if (codeCorrect) {
                lcd_clear();
                lcd_writeProgString(PSTR("Winner!"));
                os_enterCriticalSection();
                while (1) {
                    draw_clearDisplay();
                    secretAnimation();
                }
            }
        }
    }
}


REGISTER_AUTOSTART(program1)
void program1(void) {
    // Fastest strategy
    os_setSchedulingStrategy(OS_SS_EVEN);
    // Init panel and timer
    panel_init();
    panel_initTimer();
    panel_startTimer();
    js_init();
    Direction temp = ~JS_NEUTRAL;
    Direction prev;
    uint16_t xcoord = 0;
    uint16_t ycoord = 0;
    double dotx = 0, dotx_prev;
    double doty = 0, doty_prev;

    os_exec(pattern_program, DEFAULT_PRIORITY);

    while (1) {
        os_enterCriticalSection();
        xcoord = js_getHorizontal();
        ycoord = js_getVertical();

        prev = temp;
        temp = js_getDirection();
        os_leaveCriticalSection();

        draw_letter('X', 0, 0, COLOR_WHITE, true, false);
        draw_coord(xcoord, 5, 0, COLOR_WHITE);

        draw_letter('Y', 0, 6, COLOR_GREEN, true, false);
        draw_coord(ycoord, 5, 6, COLOR_GREEN);

        uint8_t x = 12;
        uint8_t y = 20;

        dotx_prev = dotx;
        doty_prev = doty;

        // calculate new dot
        dotx = x - 3 + 6.4;
        doty = y - 3 + 6.4;
        if (temp != JS_NEUTRAL) {
            double dotAngle = atan2(-((double)ycoord - 1023.0 / 2.0), (double)xcoord - 1023.0 / 2.0);
            dotx += cos(dotAngle) * 6.1;
            doty += sin(dotAngle) * 6.1;
        }
        if (prev != JS_NEUTRAL) {
            draw_filledRectangle((uint8_t)dotx_prev, (uint8_t)doty_prev, (uint8_t)dotx_prev + 1, (uint8_t)doty_prev + 1, COLOR_BLACK); // clear old dot
        }

        if (temp != JS_NEUTRAL) {
            draw_filledRectangle((uint8_t)dotx, (uint8_t)doty, (uint8_t)dotx + 1, (uint8_t)doty + 1, COLOR_RED); // draw dot
        }
        uint64_t arrowPattern;
        Color arrowColor = js_getButton() ? COLOR_BLUE : COLOR_WHITE;
        // draw dot and arrows
        switch (temp) {
            case JS_UP:
                arrowPattern = PATTERN_ARROW_UP;
                break;
            case JS_DOWN:
                arrowPattern = PATTERN_ARROW_DOWN;
                break;
            case JS_LEFT:
                arrowPattern = PATTERN_ARROW_LEFT;
                break;
            case JS_RIGHT:
                arrowPattern = PATTERN_ARROW_RIGHT;
                break;
            case JS_NEUTRAL:
                // patterns are just bit-masks, so we can OR them to simulate drawing patterns after one another
                arrowPattern = PATTERN_ARROW_UP | PATTERN_ARROW_DOWN | PATTERN_ARROW_LEFT | PATTERN_ARROW_RIGHT;
                break;
            default:
                arrowPattern = PATTERN_INVALID;
                break;
        }
        draw_pattern(12, 20, 8, 8, arrowPattern, arrowColor, true);


        if (prev != temp) {
            os_enterCriticalSection();
            lcd_line2();

            switch (temp) {
                case JS_UP:
                    lcd_writeProgString(PSTR("UP     "));
                    break;
                case JS_DOWN:
                    lcd_writeProgString(PSTR("DOWN   "));
                    break;
                case JS_LEFT:
                    lcd_writeProgString(PSTR("LEFT   "));
                    break;
                case JS_RIGHT:
                    lcd_writeProgString(PSTR("RIGHT  "));
                    break;
                case JS_NEUTRAL:
                    lcd_writeProgString(PSTR("NEUTRAL"));
                    break;
                default:
                    lcd_writeProgString(PSTR("INVALID"));
                    break;
            }
            os_leaveCriticalSection();
        }
    }
}

void applyGravity(Particle *p, long double t) {
    long double g = 20;
    long double addum = g * t * t * (1 / 2);
    long double xPosNew = p->xPos + p->xVel * t;
    long double yPosNew = p->yPos + addum + p->yVel * t;

    double yVelNew = p->yVel + g * t;
    double xVelNew = p->xVel;

    for (uint8_t i = 0; i < TRAIL - 1; i++) {
        p->lastx[TRAIL - 1 - i] = p->lastx[TRAIL - 2 - i];
        p->lasty[TRAIL - 1 - i] = p->lasty[TRAIL - 2 - i];
        p->activeTrail[TRAIL - 1 - i] = p->activeTrail[TRAIL - 2 - i];
    }

    p->lastx[0] = (uint8_t)xPosNew;
    p->lasty[0] = (uint8_t)yPosNew;
    if (p->lastx[0] >= 32) p->activeTrail[0] = false;
    if (p->lasty[0] >= 32) p->activeTrail[0] = false;

    if (p->trailInd < TRAIL) {
        p->trailInd = p->trailInd + 1;
    }

    p->xPos = xPosNew;
    p->yPos = yPosNew;
    p->xVel = xVelNew;
    p->yVel = yVelNew;

    if (p->xPos < 0 || (uint8_t)p->xPos >= 32 || p->yPos < 0 || (uint8_t)p->yPos >= 32) {
        p->active = false;
    }
}

long double variance(long double value, long double variance) {
    return value - variance + ((long double)rand() / (long double)RAND_MAX) * 2 * variance;
}

void secretAnimation() {
    Time interval = 25;
    Time lastTime = 0;

    uint8_t x, y = variance(10, 4);

    Particle rocket = {
        .xPos = variance(15, 10),
        .yPos = 31,
        .xVel = variance(0, 12),
        .yVel = variance(-34, 5),
        .cl = COLOR_RED,
        .trailCl = {[0 ... TRAIL - 1] = COLOR_RED},
        .lastx = {[0 ... TRAIL - 1] = 0},
        .lasty = {[0 ... TRAIL - 1] = 0},
        .active = true,
        .trailInd = 0,
        .activeTrail = {[0 ... TRAIL - 1] = true}};

    if (rocket.xPos < 16) {
        rocket.xVel = abs(rocket.xVel);
    } else {
        rocket.xVel = -abs(rocket.xVel);
    }

    while (y <= rocket.yPos && rocket.yPos < 32 && 0 <= rocket.xPos && rocket.xPos < 32) {
        Time t = os_systemTime_precise();
        if (t - lastTime < interval) continue;
        lastTime = t;

        if (rocket.active) {
            draw_setPixel((uint8_t)rocket.xPos, (uint8_t)rocket.yPos, COLOR_BLACK);
        }

        // delete trail
        for (uint8_t j = 0; j < rocket.trailInd; j++) {
            if (rocket.activeTrail[j]) {
                draw_setPixel((uint8_t)rocket.lastx[j], (uint8_t)rocket.lasty[j], COLOR_BLACK);
            }
        }

        applyGravity(&rocket, interval / 1000.0);

        // draw new head
        if (rocket.active) {
            draw_setPixel((uint8_t)rocket.xPos, (uint8_t)rocket.yPos, rocket.cl);
        }

        for (uint8_t j = 1; j < rocket.trailInd; j++) {
            if (rocket.activeTrail[j]) {
                draw_setPixel((uint8_t)rocket.lastx[j], (uint8_t)rocket.lasty[j], rocket.trailCl[j]);
            }
        }
        draw_setPixel((uint8_t)rocket.xPos, (uint8_t)rocket.yPos, COLOR_BLACK);
    }

    x = rocket.xPos;
    y = rocket.yPos;

    draw_clearDisplay();

    Particle p_1 = {x, y, variance(-10, 6), variance(-10, 6), COLOR_WHITE, {[0 ... TRAIL - 1] = I11_ORANGE}, {[0 ... TRAIL - 1] = 0}, {[0 ... TRAIL - 1] = 0}, true, 0, {[0 ... TRAIL - 1] = true}};
    Particle p_2 = {x, y, variance(-10, 6), variance(0, 6), COLOR_WHITE, {[0 ... TRAIL - 1] = I11_BLUE}, {[0 ... TRAIL - 1] = 0}, {[0 ... TRAIL - 1] = 0}, true, 0, {[0 ... TRAIL - 1] = true}};
    Particle p_3 = {x, y, variance(10, 6), variance(0, 6), COLOR_WHITE, {[0 ... TRAIL - 1] = I11_ORANGE}, {[0 ... TRAIL - 1] = 0}, {[0 ... TRAIL - 1] = 0}, true, 0, {[0 ... TRAIL - 1] = true}};
    Particle p_4 = {x, y, variance(6, 6), variance(0, 6), COLOR_WHITE, {[0 ... TRAIL - 1] = I11_BLUE}, {[0 ... TRAIL - 1] = 0}, {[0 ... TRAIL - 1] = 0}, true, 0, {[0 ... TRAIL - 1] = true}};
    Particle p_5 = {x, y, variance(14, 6), variance(-14, 6), COLOR_WHITE, {[0 ... TRAIL - 1] = I11_BLUE}, {[0 ... TRAIL - 1] = 0}, {[0 ... TRAIL - 1] = 0}, true, 0, {[0 ... TRAIL - 1] = true}};

    Particle *p_arr[] = {&p_1, &p_2, &p_3, &p_4, &p_5};
    uint8_t pcount = sizeof(p_arr) / sizeof(Particle *);
    lastTime = 0;

    bool anyActive = true;
    do {
        Time t = os_systemTime_precise();
        if (t - lastTime < interval) continue;
        lastTime = t;

        anyActive = false;
        for (uint8_t k = 0; k < pcount; k++) {
            Particle *p = p_arr[k];
            if (p == NULL) continue;
            anyActive = true;
            bool visible = false;

            // delete head
            if (p->active) {
                draw_setPixel((uint8_t)p->xPos, (uint8_t)p->yPos, COLOR_BLACK);
            }

            // delete trail
            for (uint8_t j = 0; j < p->trailInd; j++) {
                if (p->activeTrail[j]) {
                    draw_setPixel((uint8_t)p->lastx[j], (uint8_t)p->lasty[j], COLOR_BLACK);
                }
            }

            applyGravity(p, interval / 1000.0);

            // draw new head
            if (p->active) {
                visible = true;
                draw_setPixel((uint8_t)p->xPos, (uint8_t)p->yPos, p->cl);
            }

            for (uint8_t j = 1; j < p->trailInd; j++) {
                if (p->activeTrail[j]) {
                    visible = true;
                    draw_setPixel((uint8_t)p->lastx[j], (uint8_t)p->lasty[j], p->trailCl[j]);
                }
            }

            if (!visible) p_arr[k] = NULL;
        }
    } while (anyActive);
}
