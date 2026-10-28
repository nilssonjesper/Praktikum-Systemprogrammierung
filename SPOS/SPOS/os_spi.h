
#ifndef _OS_SPI_H_
#define _OS_SPI_H_

#include "os_mem_drivers.h"

// Functions for spi
void os_spi_init();
MemValue os_spi_send(MemValue data);
MemValue os_spi_receive();

void os_slave_enable();
void os_slave_disable();
#endif