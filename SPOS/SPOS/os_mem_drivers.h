#ifndef _MEM_DRIVERS_H
#define _MEM_DRIVERS_H

#include <stdint.h>
#define INVALID_ADDRESS 0

// Memory address type
typedef uint16_t MemAddr;

// Memory value type
typedef uint8_t MemValue;

// Read function pointer type
typedef MemValue (readFunction)(MemAddr addr);

// Write function pointer type
typedef void (writeFunction)(MemAddr addr, MemValue value);

// Init function pointer type
typedef void (initFunction)(void);

// Memory driver struct
typedef struct {
	const uint16_t size;
	MemAddr upperBound;
    MemAddr start;
    MemAddr last;
    readFunction *read;
    writeFunction *write;
	initFunction *init;
} MemDriver;

// Allow easy access to the internal SRAM
#define intSRAM (&intSRAM__)
extern MemDriver intSRAM__;

// Allow easy access to the external SRAM
#define extSRAM (&extSRAM__)
extern MemDriver extSRAM__;

// ----------------------------------------------------------------------------
// Memory driver functions
// ----------------------------------------------------------------------------

// Initialize the memory drive
void init(void);

#endif