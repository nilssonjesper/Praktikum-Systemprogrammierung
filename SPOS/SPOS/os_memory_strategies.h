#ifndef _OS_MEMORY_STRATEGIES_H
#define _OS_MEMORY_STRATEGIES_H

#include "os_memheap_drivers.h"
#include "os_process.h"

// ----------------------------------------------------------------------------
// Memory allocation strategies
// ----------------------------------------------------------------------------

// First fit strategy
MemAddr os_alloc_mem_first(Heap* heap, size_t size);
MemAddr os_alloc_mem_next(Heap* heap, size_t size);
MemAddr os_alloc_mem_worst(Heap* heap, size_t size);
MemAddr os_alloc_mem_best(Heap* heap, size_t size);
#endif