/*
 * CFile1.c
 *
 * Created: 30.10.2024 22:06:24
 *  Author: dy633455
 */ 
 #include "lcd.h"
 #include <stdint.h>
 
 
void printBinary(uint16_t number) {
	for (int i = 15; i >= 0; i--) {        // Schleife von Bit 15 bis Bit 0
		uint16_t output = (number >> i) & 1;
		lcd_writeDec(output);               // Ausgabe des aktuellen Bits
	}
}