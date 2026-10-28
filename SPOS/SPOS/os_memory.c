#include "os_memory.h"
#include "os_memheap_drivers.h"
#include "os_mem_drivers.h"
#include "os_scheduler.h"
#include "os_memory_strategies.h"
#include "os_core.h"

#include "lcd.h"

// ----------------------------------------------------------------------------
// Helper functions
// ----------------------------------------------------------------------------

void setMapEntry(Heap const* heap, MemAddr addr, MemValue nibble){
    MemAddr diff = (addr - heap->startUse);
	nibble &= 0x0F;

    if(diff % 2 == 0){

		MemAddr current = heap->startMap + (diff / 2);
        heap->driver->write(current, (heap->driver->read(current) & 0x0F) | (nibble << 4));
        return;
    }

    else{
        MemAddr current = heap->startMap + ((diff - 1) / 2);
        heap->driver->write(current, (heap->driver->read(current) & 0xF0) | nibble);
        return;
    }
}

MemAddr getMapEntry(Heap const* heap, MemAddr addr){
    MemAddr diff = (addr - heap->startUse);

    if(diff % 2 == 0){
        MemAddr current = heap->startMap + (diff / 2);
        return (heap->driver->read(current) & 0xF0) >> 4;
    }

    else{
        MemAddr current = heap->startMap + ((diff - 1) / 2);
        return heap->driver->read(current) & 0x0F;
    }

    return 0;
}

MemAddr os_startOfChunk(Heap const* heap, MemAddr addr){
	while(getMapEntry(heap, addr) == 0x0F){
		addr--;
	}
	
	return addr;
}

MemAddr os_endOfChunk(Heap const* heap, MemAddr addr){
	while(getMapEntry(heap, addr) == 0x0F){
		addr++;
	}
	
	return addr;
}

MemAddr os_paintMapSection(Heap* heap, MemAddr addr, uint16_t length, MemValue nibble){
	for(uint16_t i = 0; i < length; i++){
		setMapEntry(heap, addr, nibble);
		addr++;
	}
	return addr;
}

// Internal helper function to free a given chunk
static MemAddr freeChunk(Heap* heap, MemAddr addr){
	setMapEntry(heap, addr, heap->defaultMapValue);
	addr++;

	while(getMapEntry(heap, addr) == 0x0F && addr < (heap->startUse + heap->sizeUse)){
		setMapEntry(heap, addr, heap->defaultMapValue);
		addr++;
	}
	
	return addr;
}

void copyChunk(Heap* heap, MemAddr from, MemAddr to, uint16_t size){
	MemValue value = 0;
	
	for(uint16_t i = 0; i < size; i++){
		value = heap->driver->read(from);
		heap->driver->write(to, value);
		from++;
		to++;
	}
}

MemAddr findNewLowerBound(Heap* heap, MemAddr lowerBound, ProcessID pid){
	while(getMapEntry(heap, lowerBound) != (pid & 0x0F) && lowerBound < heap->lastUse){
		lowerBound++;
	}
	
	return lowerBound;
}

MemAddr findNewUpperBound(Heap* heap, MemAddr upperBound, ProcessID pid){
	while(getMapEntry(heap, upperBound) != (pid & 0x0F) && upperBound > heap->startUse){
		upperBound--;
	}
	
	return upperBound;
}

//============================================================================
// OS memory functions
//============================================================================

//----------------------------------------------------------------------------
// Base memory allocation functions
//----------------------------------------------------------------------------

MemAddr os_malloc(Heap* heap, uint16_t size){
    os_enterCriticalSection();
    MemAddr addr = INVALID_ADDRESS;
    switch(heap->strategy){
        case OS_MEM_FIRST:
            addr = os_alloc_mem_first(heap, size);
            break;
        case OS_MEM_NEXT:
            addr = os_alloc_mem_next(heap, size);
			break;
        case OS_MEM_BEST:
            addr = os_alloc_mem_best(heap, size);
			break;
        case OS_MEM_WORST:
            addr = os_alloc_mem_worst(heap, size);
			break;
        default:
            addr = 0;
    }
	
	if(addr == 0 || addr > heap->lastUse || addr < heap->startUse){
		os_leaveCriticalSection();
		return 0;
	}
	
	MemAddr current = addr;
	setMapEntry(heap, current, (os_getCurrentProc() & 0x0F));
	current++;
	for(uint16_t i = 1; i < size; i++){
		setMapEntry(heap, current, 0x0F);
		current++;
	}
	
	// Update bounds of process
	ProcessID pid = os_getCurrentProc();
	
	heap->lowerBounds[pid] = (heap->lowerBounds[pid] > addr) ? addr:heap->lowerBounds[pid];
	heap->upperBounds[pid] = (heap->upperBounds[pid] < (addr + size - 1)) ? (addr + size - 1):heap->upperBounds[pid];
	
    os_leaveCriticalSection();
    return addr;
}

MemAddr os_realloc(Heap* heap, MemAddr addr, uint16_t size){
	os_enterCriticalSection();
	MemAddr current = addr;
	addr = os_startOfChunk(heap, addr);
	ProcessID pid = os_getCurrentProc();
	
	if(getMapEntry(heap, addr) != (pid & 0x0F)){
		os_error("Unallowed memory relocation!");
		os_leaveCriticalSection();
		return 0;
	}
	
	uint16_t chunkSize = os_getChunkSize(heap, current);
	
	uint16_t* upperBound = &(heap->upperBounds[pid]);
	uint16_t* lowerBound = &(heap->lowerBounds[pid]);
	
	// Case: Reduce chunk size
	if (size < chunkSize){
		// Move to start of chunk
		current = os_startOfChunk(heap, current);
		// Move to section of chunk that is to be painted/deleted
		current += size;
		os_paintMapSection(heap, current, (chunkSize - size), heap->defaultMapValue);
		
		// Update upper-bound
		if(*upperBound == (addr + chunkSize - 1)){
			*upperBound = current - 1;
		}
		
		os_leaveCriticalSection();
		return current - size;
		
	}else {
		uint16_t diff = size - chunkSize;
		
		// Check if space to the right of
		current++;
		// Move to end of chunk
		current = os_endOfChunk(heap, current);
		uint16_t counterR = 0;
		while (getMapEntry(heap,current + counterR) == heap->defaultMapValue && counterR < diff) {
			counterR++;
		}
		
		// Case: Increase chunk size to the right
		if (counterR >= diff){
			os_paintMapSection(heap, current, diff, 0x0F);
			
			// Update upper-bound
			if(*upperBound == (addr + chunkSize - 1)){
				*upperBound = addr + size - 1;
			}
			
			os_leaveCriticalSection();
			return addr;
		}
		
		// Move to start of chunk
		current = current - chunkSize;
		
		// Check space to the left
		current--;
		uint16_t counterL = 0;
		while (getMapEntry(heap, current - counterL) == heap->defaultMapValue && current >= heap->startUse){
			counterL++;
		}
		
		// Check if space nearby is insufficient
		if((counterL +  counterR + chunkSize) < size){
			
			// Malloc adjusts bounds
			MemAddr newCurrent = os_malloc(heap, size);

			// Check if allocation was successful
			if(newCurrent == 0){
				os_leaveCriticalSection();
				return 0;
			}

			copyChunk(heap, addr, newCurrent, size);
			freeChunk(heap, addr);
			
			os_leaveCriticalSection();
			return newCurrent;
		}
		
		// Space left and right is sufficient => shift chunk left
		copyChunk(heap, addr, (current - counterL + 1), chunkSize);
		freeChunk(heap, current + 1);
		
		// Move to start of new chunk
		current -= counterL - 1;
		
		setMapEntry(heap, current++, os_getCurrentProc() & 0x0F);
		os_paintMapSection(heap, current, size - 1, 0x0F);
		
		current--;
		
		// Adjust bounds
		*lowerBound = (*lowerBound > addr) ? addr:*lowerBound;
		*upperBound = (*upperBound < (addr + size - 1)) ? (addr + size - 1):*upperBound;
		
		os_leaveCriticalSection();
		return current;
	}
}

void os_free(Heap* heap, MemAddr addr){
    os_enterCriticalSection();
	
	ProcessID pid = os_getCurrentProc();
	uint16_t* upperBound = &(heap->upperBounds[pid]);
	uint16_t* lowerBound = &(heap->lowerBounds[pid]);

    // Find the start of the memory block to be freed
    addr = os_startOfChunk(heap, addr);

    // Check wether the process owns the memory to be freed (or is shared memory)
    if(getMapEntry(heap, addr) != pid || gbi(getMapEntry(heap, addr), 3)){
        os_error("Unallowed memory freeing!");
        os_leaveCriticalSection();
        return;
    }
	
	// Free chunk
    freeChunk(heap, addr);
	
	addr = os_startOfChunk(heap, addr);
	
	if(*lowerBound == addr){
		*lowerBound = findNewLowerBound(heap, *lowerBound, pid);
	}
	
	addr++;
	addr = os_endOfChunk(heap, addr);
	addr--;
	
	if(*upperBound == addr){
		*upperBound = findNewUpperBound(heap, *upperBound, pid);
	}

    os_leaveCriticalSection();
}

// Free all allocated chunks of a given process
void os_freeProcessMemory(Heap* heap, ProcessID pid){
	os_enterCriticalSection();
	MemAddr current = heap->lowerBounds[pid];
	
	// Look through heap for allocated memory of pid
	while(current < (heap->upperBounds[pid] + 1)){
		if(getMapEntry(heap, current) == (pid & 0x0F)){
			// Free chunk of pid
			current = freeChunk(heap, current);
		}
		
		else{
			current++;
		}
	}
	os_leaveCriticalSection();
	return;
}

//----------------------------------------------------------------------------
// Shared memory allocation functions
//----------------------------------------------------------------------------

// Protocoll: 1WRR (Shared, Write, Read, Read)

uint8_t is_shared(Heap const* heap, MemAddr addr){
	return getMapEntry(heap, addr) & 0b1000;
}

MemValue getReadFlag(Heap const* heap, MemAddr addr){
	return getMapEntry(heap, addr) & 0b0011;
}

MemValue getWriteFlag(Heap const* heap, MemAddr addr){
	return getMapEntry(heap, addr) & 0b0100;
}

void loadSharedMemory(Heap const* heap, MemAddr const from, MemValue* to, uint16_t size){
	MemValue value = 0;
	
	for(uint16_t i = 0; i < size; i++){
		value = heap->driver->read(from + i);
		*(to + i) = value;
	}
}

void storeSharedMemory(Heap const* heap, MemValue const* from, MemAddr const to, uint16_t size){
	for(uint16_t i = 0; i < size; i++){
		MemValue value = *(from + i);
		heap->driver->write((to + i), value);
	}
}

MemAddr os_sh_malloc(Heap* heap, uint16_t size){
	os_enterCriticalSection();
	MemAddr addr = os_malloc(heap, size);

	if(addr == INVALID_ADDRESS){
		os_leaveCriticalSection();
		return INVALID_ADDRESS;
	}

	// Convert to shared memory
	setMapEntry(heap, addr, 0x08);

	os_leaveCriticalSection();
	return addr;
}

void os_sh_free(Heap* heap, MemAddr* addr){
	os_enterCriticalSection();
	MemAddr current = os_startOfChunk(heap, *addr);

	if(!is_shared(heap, current)){
		os_leaveCriticalSection();
		os_error("Unallowed memory access!");
		return;
	}

	// Wait till memory is not in use
	while(getMapEntry(heap, current) & 0x07) os_yield();

	// Free chunk
	freeChunk(heap, current);

	os_leaveCriticalSection();
	return;
}

MemAddr os_sh_readOpen(Heap const* heap, MemAddr const* addr){
	os_enterCriticalSection();
	
	// Save dereferenced address
	MemAddr current = os_startOfChunk(heap, *addr);

	// Check if shared memory
	if(!is_shared(heap, current)){
		os_leaveCriticalSection();
		os_error("Unallowed memory access!");
		return 0;
	}
	
	// Wait till memory is not in use (write or read-flag full)
	while(getWriteFlag(heap, current) || getReadFlag(heap, current) >= 3){
		os_yield(); 
		current = os_startOfChunk(heap, *addr);
		if(!is_shared(heap, current)){
			os_leaveCriticalSection();
			os_error("Invalid shared address");
			return INVALID_ADDRESS;
		}
	}
	
	// Increase read flag
	MemValue flag = getReadFlag(heap, current);
	flag++;
	
	setMapEntry(heap, current, 0b1000 | flag);

	os_leaveCriticalSection();
	return current;

}

MemAddr os_sh_writeOpen(Heap const* heap, MemAddr const* addr){
	os_enterCriticalSection();
	MemAddr current = os_startOfChunk(heap, *addr);

	// Check if shared memory
	if(!is_shared(heap, current)){
		os_leaveCriticalSection();
		os_error("Unallowed memory access!");
		return 0;
	}
	
	// Wait till memory is not in use (no flag at all)
	while(getReadFlag(heap, current) || getWriteFlag(heap, current)) {
		os_yield();
		current = os_startOfChunk(heap, *addr);
		if(!is_shared(heap, current)){
			os_leaveCriticalSection();
			os_error("Invalid shared address");
			return INVALID_ADDRESS;
		}
	}

	// Set entry
	setMapEntry(heap, current, 0b1100);

	os_leaveCriticalSection();
	return current;
}

void os_sh_close(Heap const* heap, MemAddr addr){
	os_enterCriticalSection();
	
	MemAddr current = os_startOfChunk(heap, addr);

	// Check if shared memory
	if(!is_shared(heap, current)){
		os_leaveCriticalSection();
		os_error("Unallowed memory access!");
		return;
	}

	// Check if in write mode
	if(getWriteFlag(heap, current)){
		setMapEntry(heap, current, 0b1000);
	}

	else if(getReadFlag(heap, current)){
		// Decrease read flag
		MemValue flag = getReadFlag(heap, current);

		flag--;
		setMapEntry(heap, current, 0b1000 | flag);
	}

	os_leaveCriticalSection();
	return;
}

void os_sh_read(Heap const* heap, MemAddr const* ptr, uint16_t offset, MemValue* dataDest, uint16_t length){
	// Open shared memory in read mode
	MemAddr current = os_sh_readOpen(heap, ptr);
	
	if(os_endOfChunk(heap, current + 1) <= (current + offset + length - 1)){
		os_sh_close(heap, current);
		os_error("Unallowed memory access!");
		return;
	}

	// Copy data to internal personal memory
	loadSharedMemory(heap, (current + offset), dataDest, length);

	// Close shared memory
	os_sh_close(heap, current);
	return;
}

void os_sh_write(Heap const* heap, MemAddr const* ptr, uint16_t offset, MemValue const* dataSrc, uint16_t length){
	// Open shared memory in write mode
	MemAddr current = os_sh_writeOpen(heap, ptr);
	
	if(os_endOfChunk(heap, current + 1) <= (current + offset + length - 1)){
		os_sh_close(heap, current);
		os_error("Unallowed memory access!");
		return;
	}

	// Store data in shared memory
	storeSharedMemory(heap, dataSrc, (current + offset), length);

	// Close shared memory
	os_sh_close(heap, current);
	return;
}

//----------------------------------------------------------------------------
// Get and set functions
//----------------------------------------------------------------------------
size_t os_getMapSize(Heap const* heap){
    return heap->sizeMap;
}

size_t os_getUseSize(Heap const* heap){
    return heap->sizeUse;
}

MemAddr os_getMapStart(Heap const* heap){
    return heap->startMap;
}

MemAddr os_getUseStart(Heap const* heap){
    return heap->startUse;
}

uint16_t os_getChunkSize(Heap const* heap, MemAddr addr){
    // Find start of the memory block
    while(getMapEntry(heap, addr) == 0x0F){
        addr--;
    }

    // Count the size of the memory block
    uint16_t size = 1;
    addr++;
    while(getMapEntry(heap, addr) == 0x0F){
        size++;
        addr++;
    }

    return size;
}

AllocStrategy os_getAllocationStrategy(Heap const* heap){
    return heap->strategy;
}

void os_setAllocationStrategy(Heap *heap, AllocStrategy allocStrat){
    heap->strategy = allocStrat;
}