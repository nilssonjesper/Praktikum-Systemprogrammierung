#include "os_memheap_drivers.h"
#include "atmega644constants.h"
#include "defines.h"
#include "os_core.h"
#include "os_mem_drivers.h"

Heap extHeap__ = {
	.driver = extSRAM,
	.size = 0,
	.defaultMapValue = 0,
	.startMap = 0,
	.startUse = 0,
	.sizeMap = 0,
	.sizeUse = 0,
	.strategy = OS_MEM_FIRST,
	.lastAlloc = 0,
	.name = "external Heap"
};

Heap intHeap__ = {
    .driver = intSRAM,
	.size = 0,
	.defaultMapValue = 0,
	.startMap = 0,
	.startUse = 0,
    .sizeMap = 0,
    .sizeUse = 0,
    .strategy = OS_MEM_FIRST,
	.lastAlloc = 0,
    .name = "internal Heap"
};

Heap* heaps[2] = {intHeap, extHeap};

// Heap list will have to be implemented in the future...
uint8_t os_getHeapListLength(void){
	return 2;
}

Heap* os_lookupHeap(uint8_t index){
	return heaps[index];
}

// Init all heaps inside heap list
void os_initHeaps(void){
	for(uint8_t i = 0; i < os_getHeapListLength(); i++){
		Heap* heap = os_lookupHeap(i);
		
		// Init memory driver
		heap->driver->init();
		
		MemAddr offset = (i == 0) ? HEAPOFFSET:0;
	
		// Initialize heap values
		if(i == 0){
			heap->size = (heap->driver->size / 2) - HEAPOFFSET;
		}
		
		else{
			heap->size = heap->driver->size;
		}
		
		// Adjust size so it can be evenly split in map and use
		heap->size -= (heap->size % 3);
		
		// Set map and use parameters
		heap->sizeMap = heap->size / 3;
		heap->sizeUse = heap->size - heap->sizeMap;
		heap->startMap = offset + heap->driver->start;
		heap->startUse = offset + heap->sizeMap + heap->driver->start;
		heap->lastMap = heap->startMap + heap->sizeMap - 1 + heap->driver->start;
		heap->lastUse = heap->startUse + heap->sizeUse - 1 + heap->driver->start;
		heap->driver->upperBound = heap->lastUse;
		
		for(uint8_t pid = 0; pid < MAX_NUMBER_OF_PROCESSES; pid++){
			heap->lowerBounds[pid] = heap->lastUse;
			heap->upperBounds[pid] = heap->startUse;
		}
	
		for(uint16_t j = 0; j < heap->sizeMap; j++){
			heap->driver->write(heap->startMap + j, 0);
		}
	
	}
	return;
}