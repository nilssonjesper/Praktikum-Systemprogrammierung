#include "os_spi.h"
#include "defines.h"
#include "os_scheduler.h"
#include "util.h"
#include "util/atomic.h"

void os_slave_enable(void){
	PORTB &= ~(1 << 4);
}

void os_slave_disable(void){
	PORTB |= (1 << 4);
}

void os_spi_init() {
	ATOMIC {
		// Configure B-Port
		DDRB |= (1 << 4) | (1 << 5) | (1 << 7);
		DDRB &= ~(1 << 6);
	
		// Set control register
		SPCR |= 0b01010000;
		SPCR &= 0b01110011;
		// Set status register (16MHz max. clock speed) + clear WCO1
		SPSR |= 0b00000001;
		cbi(SPSR, 6);
		os_slave_disable();
	
		// Bytewise transmission
		os_slave_enable();
		os_spi_send(WRITE_MODE_REGISTER);
		os_spi_send(BYTE_MODE);
		os_slave_disable();
	};
    return;
}

MemValue os_spi_send(MemValue data){
    os_enterCriticalSection();

    // Start transmission
    SPDR = data;

    // Wait until transmission is complete
    while(!(SPSR & (1 << SPIF)));

    // Transmission complete
    os_leaveCriticalSection();
    return SPDR;
}

MemValue os_spi_receive(){
    return os_spi_send(0xFF);
}