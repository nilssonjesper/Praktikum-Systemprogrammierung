#ifndef _MEMHEAP_DRIVERS_H
#define _MEMHEAP_DRIVERS_H

#include "os_mem_drivers.h"
#include "atmega644constants.h"

// Heap constants in bytes
#define HEAPMAPSIZE (AVR_MEMORY_SRAM / 8) - HEAPOFFSET
#define HEAPUSESIZE ((AVR_MEMORY_SRAM / 2) - HEAPMAPSIZE - HEAPOFFSET)

typedef enum {
    OS_MEM_FIRST,
    OS_MEM_NEXT,
    OS_MEM_BEST,
    OS_MEM_WORST
} AllocStrategy;

typedef struct {
    MemDriver* driver;
    memAddr startMap;
    size_t sizeMap;
    memAddr startUse;
    size_t sizeUse;
    AllocStrategy strategy;
    char* name;
} Heap;