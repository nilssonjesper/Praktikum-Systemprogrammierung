#include "os_memheap_drivers.h"
#include "atmega644Constants.h"
#include "defines.h"

Heap intHeap__ = {
    .driver = intSRAM,
    .startMap = intSRAM->start + HEAPOFFSET,
    .sizeMap = HEAPMAPSIZE,
    .startUse = ,
    .sizeUse = HEAPUSESIZE,
    .strategy = OS_MEM_FIRST,
    .name = "internal Heap"
};

void os_initHeaps(void){
    intHeap__.
}