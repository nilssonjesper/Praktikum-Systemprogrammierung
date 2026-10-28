/*! \file
 *  \brief Memory drivers for the OS.
 * 
 * Contains the memory drivers for the OS.
 *
 *  \author Jannis von der Bank & Jesper Nilsson
 */


#include "os_mem_drivers.h"
#include "atmega644constants.h"

// Create driver instance
MemDriver intSRAM__ = {
    .start = AVR_SRAM_START,
    .last = AVR_SRAM_LAST,
    .read = read,
    .write = write
};

// Declare memory driver functions
memValue read(memAddr addr);
void write(memAddr addr, memValue value);

// ----------------------------------------------------------------------------
// Memory driver functions
// ----------------------------------------------------------------------------

void init(void){
    return;
}

memValue read(memAddr addr){
    if(addr < intSRAM__.start || addr > intSRAM__.last){
        return;
    }

    return *((memValue*) addr);
}

void write(memAddr addr, memValue value){
    if(addr < intSRAM__.start || addr > intSRAM__.last){
        return;
    }
    
    *((memValue*) addr) = value;
    return;
}