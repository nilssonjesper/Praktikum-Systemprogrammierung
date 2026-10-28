#include "os_memory_strategies.h"
#include "os_process.h"
#include "os_memory.h"

#include "lcd.h"

// First fit strategy
MemAddr os_alloc_mem_first(Heap* heap, size_t size){
	if(size > heap->sizeUse){
		return 0;
	}
	
	MemAddr current = heap->startUse;
	uint16_t chunkSize = 0;
		
	while(current < (heap->startUse + heap->sizeUse)){
		if(getMapEntry(heap, current) == heap->defaultMapValue){
			chunkSize++;
		}
			
		else{
			chunkSize = 0;
		}
			
		if(chunkSize >= size){
			return current - chunkSize + 1;
		}
		
		current++;
	}
		
	return 0;
}

// MemAddr os_alloc_mem_next(Heap* heap, uint16_t size){
// 	if(size > heap->sizeUse){
// 		return 0;
// 	}
// 	
// 	if(heap->lastAlloc == INVALID_ADDRESS){
// 		heap->lastAlloc = heap->startUse;
// 	}
// 	
// 	MemAddr current = heap->lastAlloc;
// 	uint16_t chunkSize = 0;
// 	uint8_t firstChunk = 1;
// 	uint16_t firstChunkSize = 0;
// 	
// 	do {
// 		if(getMapEntry(heap, current) == heap->defaultMapValue){
// 			chunkSize++;
// 		}else{
// 			firstChunkSize = (firstChunk) ? chunkSize:firstChunkSize;
// 			firstChunk = 0;
// 			chunkSize = 0;
// 		}
// 		if(chunkSize >= size){
// 			heap->lastAlloc = (current == heap->lastUse) ? heap->startUse:(current + 1);
// 			return current - chunkSize + 1;
// 		}
// 		if(current >= heap->lastUse){
// 			firstChunkSize = (firstChunk) ? chunkSize:firstChunkSize;
// 			firstChunk = 0;
// 			chunkSize = 0;
// 			current = heap->startUse;
// 		}else{
// 			current++;
// 		}
// 	}while(current != heap->lastAlloc);
// 	
// 	if(chunkSize + firstChunkSize >= size){
// 		heap->lastAlloc = (current - chunkSize + size - 1 == heap->lastUse) ? heap->startUse:(current - chunkSize + size);
// 		return current - chunkSize + 1;
// 	}else{
// 		return 0;
// 	}
// }

MemAddr os_alloc_mem_next(Heap* heap, uint16_t size){
	if(size > heap->sizeUse){
		return 0;
	}
	
	if(heap->lastAlloc == INVALID_ADDRESS){
		heap->lastAlloc = heap->startUse;
	}
	
	MemAddr current = heap->lastAlloc;
	size_t chunkSize = 0;
	uint16_t tempSize = 0;
	uint8_t firstChunk = 1;
	
	do {
		
		if(getMapEntry(heap, current) == heap->defaultMapValue){
			chunkSize++;
		}
		
		else{
			tempSize = (firstChunk) ? chunkSize:tempSize;
			firstChunk = 0;
			chunkSize = 0;
		}
		
		if(chunkSize >= size){
			heap->lastAlloc = ((current + 1) > heap->lastUse) ? INVALID_ADDRESS:(current + 1);
			return current - chunkSize + 1;
		}
		
		if(++current > heap->lastUse){
			tempSize = (firstChunk) ? chunkSize:tempSize;
			firstChunk = 0;
			chunkSize = 0;
			current = heap->startUse;
		}
	} while(current != heap->lastAlloc);
	
	if(!firstChunk && (tempSize + chunkSize) >= size){
		heap->lastAlloc -= chunkSize;
		return heap->lastAlloc;
	}
	return 0;
}

MemAddr os_alloc_mem_best(Heap* heap, size_t size){
	if(size > heap->sizeUse){
		return 0;
	}
	
	MemAddr current = heap->startUse;
	MemAddr bestChunk = 0;
	size_t chunkSize = 0;
	size_t bestChunkSize = heap->sizeUse;
	
	while(current < (heap->lastUse + 1)){
		if(getMapEntry(heap, current) == heap->defaultMapValue){
			chunkSize++;
		}
		
		else if(size <= chunkSize && bestChunkSize > chunkSize){
			bestChunk = current - chunkSize;
			bestChunkSize = chunkSize;
			chunkSize = 0;
		}
		else{
			chunkSize = 0;
		}
		
		current++;
	}
	
	if(size <= chunkSize && bestChunkSize >= chunkSize){
		return current - chunkSize;
	}
	
	return bestChunk;
}


MemAddr os_alloc_mem_worst(Heap* heap, size_t size){
	if(size > heap->sizeUse){
		return 0;
	}
	
	MemAddr current = heap->startUse;
	MemAddr worstChunk = 0;
	size_t chunkSize = 0;
	size_t worstChunkSize = 0;
	
	while(current < (heap->lastUse + 1)){
		if(getMapEntry(heap, current) == heap->defaultMapValue){
			chunkSize++;
		}
		
		else if(size <= chunkSize && worstChunkSize < chunkSize){
			worstChunk = current - chunkSize;
			worstChunkSize = chunkSize;
			chunkSize = 0;
		}
		else{
			chunkSize = 0;
		}
		
		current++;
	}
	
	if(size <= chunkSize && worstChunkSize < chunkSize){
		return current - chunkSize;
	}
	
	return worstChunk;
}