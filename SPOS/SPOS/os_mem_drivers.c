/*! \file
 *  \brief Memory drivers for the OS.
 * 
 * Contains the memory drivers for the OS.
 *
 *  \author Jannis von der Bank & Jesper Nilsson
 */


#include "os_mem_drivers.h"
#include "atmega644constants.h"
#include "os_spi.h"
#include "defines.h"
#include "os_core.h"
#include "os_scheduler.h"
#include "util.h"
#include "util/atomic.h"

extern uint8_t const __heap_start;
#define heap_start ((MemAddr) &__heap_start)

// ----------------------------------------------------------------------------
// Memory driver functions
// ----------------------------------------------------------------------------

// READ & WRITE ANPASSEN!!!!

static void extWrite(volatile MemAddr addr, MemValue value) {
	ATOMIC {
		os_slave_enable();
		
		os_spi_send(WRITE_INSTRUCTION);
		os_spi_send(0x00);
		os_spi_send(addr >> 8);
		os_spi_send(addr >> 0);
		
		os_spi_send(value); // MemAddr should be 1 byte
		
		os_slave_disable();
	}
}

static MemValue extRead(MemAddr addr) {
	MemValue value = 0x00;
	ATOMIC {
		os_slave_enable();
		
		os_spi_send(READ_INSTRUCTION);
		os_spi_send(0x00);
		os_spi_send(addr >> 8);
		os_spi_send(addr >> 0); // MemAddr should be 1 byte
		value = os_spi_receive();
		
		os_slave_disable();
	}
	return value;
}

void intInit(void){
	if(heap_start >= (HEAPOFFSET + intSRAM->start)){
		os_error("Heap offset is too small! Heap collides with .data section!");
	}
}

MemValue intRead(MemAddr addr){
	if(addr > intSRAM->last) return INVALID_ADDRESS;

    return *((MemValue*) addr);
}

void intWrite(MemAddr addr, MemValue value){
	if(intSRAM->upperBound && addr > intSRAM->upperBound){
		return;
	}
	
	if(addr > intSRAM->last) return;
    
    *((MemValue*) addr) = value;
    return;
}

// Create driver instance
MemDriver intSRAM__ = {
	.size = AVR_MEMORY_SRAM,
	.start = AVR_SRAM_START,
	.last = AVR_SRAM_LAST,
	.read = intRead,
	.write = intWrite,
	.init = intInit
};

// Create external driver instance
MemDriver extSRAM__ = {
	.size = 0b1111111111111111,
	.start = 0,
	.last = 0b1111111111111111,
	.read = extRead,
	.write = extWrite,
	.init = os_spi_init
};