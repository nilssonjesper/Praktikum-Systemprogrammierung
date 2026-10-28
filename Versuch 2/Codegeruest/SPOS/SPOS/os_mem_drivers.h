#ifndef _MEM_DRIVERS_H
#define _MEM_DRIVERS_H

#include <stdint.h>

#define INVALID_ADDRESS 0

// Allow easy access to the internal SRAM
extern MemDriver intSRAM__;
#define intSRAM &intSRAM__

// Memory address type
typedef uint16_t memAddr;

// Memory value type
typedef uint8_t memValue;

// Read function pointer type
typedef memValue (readFunction)(memAddr addr);

// Write function pointer type
typedef void (writeFunction)(memAddr addr, memValue value);

// Memory driver struct
typedef struct {
    const memAddr start;
    const memAddr last;
    readFunction *read;
    writeFunction *write;
} MemDriver;

// ----------------------------------------------------------------------------
// Memory driver functions
// ----------------------------------------------------------------------------

// Initialize the memory drive
void init(void);

#endif