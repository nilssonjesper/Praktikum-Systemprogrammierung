#ifndef _MEMHEAP_DRIVERS_H
#define _MEMHEAP_DRIVERS_H

#include <stdint.h>
#include "os_mem_drivers.h"
#include "atmega644constants.h"

#define intHeap (&intHeap__)
#define extHeap (&extHeap__)

#include <stddef.h>
#include "defines.h"

typedef enum {
    OS_MEM_FIRST,
    OS_MEM_NEXT,
    OS_MEM_BEST,
    OS_MEM_WORST
} AllocStrategy;

typedef struct {
    MemDriver* driver;
	const MemValue defaultMapValue;
	uint16_t size;
    MemAddr startMap;
    uint16_t sizeMap;
	MemAddr lastMap;
    MemAddr startUse;
    uint16_t sizeUse;
	MemAddr lastUse;
    AllocStrategy strategy;
	MemAddr lastAlloc;
	uint16_t lowerBounds[MAX_NUMBER_OF_PROCESSES];
	uint16_t upperBounds[MAX_NUMBER_OF_PROCESSES];
    char* name;
} Heap;

extern Heap intHeap__;
extern Heap extHeap__;

// Function headers
Heap* os_lookupHeap(uint8_t index);
void os_initHeaps(void);
uint8_t os_getHeapListLength(void);

#endif