//------------------------------------------------------------
//          TestTask: Allocation Strategies
//------------------------------------------------------------

#include "lcd.h"
#include "os_core.h"
#include "os_input.h"
#include "os_memory.h"
#include "os_scheduler.h"
#include "util.h"

#include <avr/pgmspace.h>
#include <stdbool.h>
#include <stdlib.h>
#include <util/atomic.h>

#if VERSUCH < 3
#error "Please fix the VERSUCH-define"
#endif

//---- Adjust here what to test -------------------
#define TEST_OS_MEM_FIRST 1
#define TEST_OS_MEM_NEXT (VERSUCH > 3)  // Optional in Versuch 3
#define TEST_OS_MEM_BEST (VERSUCH > 3)  // Optional in Versuch 3
#define TEST_OS_MEM_WORST (VERSUCH > 3) // Optional in Versuch 3
#define DRIVER intHeap
//-------------------------------------------------

#ifndef WRITE
#define WRITE(str) lcd_writeProgString(PSTR(str))
#endif
#define TEST_PASSED                    \
    do {                               \
        ATOMIC {                       \
            lcd_clear();               \
            WRITE("  TEST PASSED   "); \
        }                              \
    } while (0)
#define TEST_FAILED(reason)  \
    do {                     \
        ATOMIC {             \
            lcd_clear();     \
            WRITE("FAIL  "); \
            WRITE(reason);   \
        }                    \
    } while (0)
#ifndef CONFIRM_REQUIRED
#define CONFIRM_REQUIRED 1
#endif


#define BLOCK_SIZE 5
#define ALLOCATIONS 3
#define ALLOC_SIZE (2 * BLOCK_SIZE)

void writeBlocks(const uint8_t s[5], const MemAddr p[5], const MemAddr a[ALLOCATIONS]) {
    for (uint8_t i = 0; i < 5; i++) {
        uint8_t size = s[i] / BLOCK_SIZE;
        MemAddr start = p[i];
        bool allocatedByTest = (i % 2) == 1;
        uint8_t allocatedByStrat = 0;
        for (uint8_t j = 0; j < ALLOCATIONS; j++) {
            if (a[j] == start) allocatedByStrat = j + 1;
        }
        for (uint8_t block = 0; block < size; block++) {
            if (allocatedByStrat && block < ALLOC_SIZE / BLOCK_SIZE) {
                lcd_writeChar('0' + allocatedByStrat - 1);
            } else if (allocatedByTest) {
                lcd_writeProgString(PSTR("▮"));
            } else {
                lcd_writeChar('-');
            }
        }
    }
}

void writeStrategyName(AllocStrategy strategy) {
    switch (strategy) {
        case OS_MEM_FIRST:
            lcd_writeProgString(PSTR("FirstFit"));
            break;
        case OS_MEM_NEXT:
            lcd_writeProgString(PSTR("NextFit"));
            break;
        case OS_MEM_BEST:
            lcd_writeProgString(PSTR("BestFit"));
            break;
        case OS_MEM_WORST:
            lcd_writeProgString(PSTR("WorstFit"));
            break;
        default:
            lcd_writeChar('e');
            break;
    }
}

uint8_t error = 0;

void validateStrategy(AllocStrategy strategy, bool valid) {
    lcd_line1();
    writeStrategyName(strategy);
    lcd_writeChar(' ');
    if (valid) {
        lcd_writeProgString(PSTR("OK"));
        delayMs(10 * DEFAULT_OUTPUT_DELAY);
    } else {
        error = 1;
        lcd_writeProgString(PSTR("Error"));
        delayMs(20 * DEFAULT_OUTPUT_DELAY);
    }
}

REGISTER_AUTOSTART(program1)
void program1(void) {

    // Struct-Array with Alloc.Strats
    const AllocStrategy strategies[] = {
#if TEST_OS_MEM_FIRST
        OS_MEM_FIRST,
#endif
#if TEST_OS_MEM_NEXT
        OS_MEM_NEXT,
#endif
#if TEST_OS_MEM_BEST
        OS_MEM_BEST,
#endif
#if TEST_OS_MEM_WORST
        OS_MEM_WORST,
#endif
    };
    const uint8_t strategyCount = sizeof(strategies) / sizeof(AllocStrategy);

    lcd_clear();
    lcd_writeProgString(PSTR("Starting"));
    delayMs(6 * DEFAULT_OUTPUT_DELAY);

    /*
     * Create following pattern in memory: first big, second small, rest huge
     * X = allocated
     * __________________________________
     * |   |   |   | X |   |   | X |   |   |   |   | ...
     * 0   5   10  15  20  25  30  35  40  45  50  55
     *
     * malloc and free must be working correctly
     */
    uint8_t s[5] = {3 * BLOCK_SIZE, 1 * BLOCK_SIZE, ALLOC_SIZE, 1 * BLOCK_SIZE, 4 * BLOCK_SIZE};
    MemAddr p[5];

    // Precheck heap size
    uint16_t sum = 0;
    for (uint8_t i = 0; i < 5; i++) {
        sum += s[i];
    }
    if (os_getUseSize(DRIVER) < 2 * sum) {
        TEST_FAILED("Heap too  small");
        HALT;
    }

    uint16_t start = os_getMapStart(DRIVER);

    // Check if map is clean
    for (size_t i = 0; i < os_getMapSize(DRIVER); i++) {
        if (DRIVER->driver->read(start + i)) {
            TEST_FAILED("Map not free");
            HALT;
        }
    }

    // Check overalloc for all strategies
    for (uint8_t strategyIndex = 0; strategyIndex < strategyCount; strategyIndex++) {
        os_setAllocationStrategy(DRIVER, strategies[strategyIndex]);
        if (os_malloc(DRIVER, os_getUseSize(DRIVER) + 1) != 0) {
            TEST_FAILED("Overalloc");
            HALT;
        }
    }

    // The test for next fit depends on not creating the memory pattern with OS_MEM_NEXT
    os_setAllocationStrategy(DRIVER, OS_MEM_FIRST);
    // Create pattern in memory
    for (uint8_t i = 0; i < 5; i++) {
        p[i] = os_malloc(DRIVER, s[i]);
    }
    for (uint8_t i = 0; i < 5; i += 2) {
        os_free(DRIVER, p[i]);
    }
    lcd_clear();

    // Check strategies
    for (uint8_t strategyIndex = 0; strategyIndex < strategyCount; strategyIndex++) {
        AllocStrategy strategy = strategies[strategyIndex];
        MemAddr a[ALLOCATIONS], current[ALLOCATIONS] = {0};

        os_setAllocationStrategy(DRIVER, strategy);
        writeStrategyName(strategy);
        lcd_line2();
        writeBlocks(s, p, current);
        delayMs(4 * DEFAULT_OUTPUT_DELAY);

        /*
         * We allocate, and directly free two times (except for BestFit)
         * for NextFit we should get different addresses
         * for FirstFit, both addresses are equal to first segment
         * for BestFit, first address equals second segment an second address equals first segment
         * for WorstFit we get the first byte of the rest memory
         * otherwise we found an error
         */
        for (uint8_t i = 0; i < ALLOCATIONS; i++) {
            a[i] = os_malloc(DRIVER, ALLOC_SIZE);
            current[i] = a[i];
            lcd_line2();
            writeBlocks(s, p, current);
            delayMs(4 * DEFAULT_OUTPUT_DELAY);

            if (strategy != OS_MEM_BEST) {
                os_free(DRIVER, a[i]);
                current[i] = 0;
            }
        }

        bool valid;
        switch (strategy) {
            case OS_MEM_FIRST:
                valid = (a[0] == a[0] && a[1] == p[0] && a[2] == p[0]);
                break;
            case OS_MEM_NEXT:
                valid = (a[0] == p[0] && a[1] == p[2] && a[2] == p[4]);
                break;
            case OS_MEM_BEST:
                valid = (a[0] == p[2] && a[1] == p[0] && a[2] == p[4]);
                break;
            case OS_MEM_WORST:
                valid = (a[0] == p[4] && a[1] == p[4] && a[2] == p[4]);
                break;
            default:
                valid = false;
                break;
        }

        validateStrategy(strategy, valid);

        if (strategy == OS_MEM_BEST) {
            // Free manually as it wasn't done before
            for (uint8_t i = 0; i < ALLOCATIONS; i++) {
                if (a[i]) {
                    os_free(DRIVER, a[i]);
                }
            }
        }

        lcd_clear();
    }

    // Remove pattern
    for (uint8_t i = 1; i < 5; i += 2) {
        os_free(DRIVER, p[i]);
    }

    // Check if map is clean
    for (size_t i = 0; i < os_getMapSize(DRIVER); i++) {
        if (DRIVER->driver->read(start + i)) {
            TEST_FAILED("Map not free afterwards");
            HALT;
        }
    }

    // Special NextFit test
    if (TEST_OS_MEM_NEXT) {
        os_setAllocationStrategy(DRIVER, OS_MEM_NEXT);
        writeStrategyName(OS_MEM_NEXT);
        lcd_line2();
        lcd_writeProgString(PSTR("Special"));
        delayMs(4 * DEFAULT_OUTPUT_DELAY);
        size_t gapSize = os_getUseSize(DRIVER) - sum;
        // Skip space left over from the previous test
        os_free(DRIVER, os_malloc(DRIVER, s[4] - ALLOC_SIZE));
        // Skip the gap
        os_free(DRIVER, os_malloc(DRIVER, gapSize - (s[3] + s[4])));
        // Allocate till end
        p[3] = os_malloc(DRIVER, s[3]);
        p[4] = os_malloc(DRIVER, s[4]);
        // Allocate at start
        p[0] = os_malloc(DRIVER, s[0]);
        p[1] = os_malloc(DRIVER, s[1]);
        p[2] = os_malloc(DRIVER, s[2]);
        // Free space before pointer
        os_free(DRIVER, p[2]);
        // Try to allocate all space around pointer
        bool valid = (os_malloc(DRIVER, s[2] + gapSize) != 0);
        // The map should now be full
        for (size_t i = 0; i < os_getMapSize(DRIVER); i++) {
            MemValue value = DRIVER->driver->read(start + i);
            if (!(value & 0xF0 && value & 0x0F)) {
                valid = false;
                break;
            }
        }
        validateStrategy(OS_MEM_NEXT, valid);
        lcd_clear();
    }

    if (error) {
        TEST_FAILED("Check failed");
        HALT;
    }

// SUCCESS
#if CONFIRM_REQUIRED
    lcd_clear();
    lcd_writeProgString(PSTR("  PRESS ENTER!  "));
    os_waitForInput();
    os_waitForNoInput();
#endif
    TEST_PASSED;
    lcd_line2();
    lcd_writeProgString(PSTR(" WAIT FOR IDLE  "));
    delayMs(DEFAULT_OUTPUT_DELAY * 6);
}
