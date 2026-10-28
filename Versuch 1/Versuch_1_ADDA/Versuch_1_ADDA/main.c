#include <avr/io.h>
#include <util/delay.h>
#include "os_input.h"
#define programm 2

void manuell (void){
	while(1){
		// Auslesen von PIND
		uint8_t pinState = ~PIND;
		
		//Einlesen in A & B
		PORTA = ~pinState;
		PORTB = pinState;
	}
}

void trackWandler (void){
	// Veränderung nach PORT A & B schreiben
	// Dann an PIN C0 ablesen ob 0 --> drunter
	//							1 --> drüber
	//--> Wenn 0 PORT A und B (bzw Wert darin) um 1 erhöhen
	// Wenn 1 dann fertig
	os_initInput();
	while (1){
		os_waitForInput();
		os_waitForNoInput();
		//PORTB = 0b01010101;
			
		
		uint8_t approx = PORTB;	
		_delay_ms(50);
		uint8_t dir = PINC & 0b00000001;
		while (1){
			// Aktuelle Approx durch Komparator schicken
			PORTB = approx;
			_delay_ms(50);
		
			uint8_t pinState = PINC & 0b00000001;
		 
			// auslesen von Pin C0
			if (dir > 0){
				if (pinState > 0){
					if (approx <= 0){
						break;
					}
					approx--;
					PORTA = ~approx;
				} else{
					break;
				}
			}else{
				if (pinState > 0){
					break;
				} else{
					if (approx >= 0b11111111){
						break;
					}
					approx++;
					PORTA = ~approx;
				}
			}
		}
	}
}

void sukAppWandler (void){
	os_initInput();
	while (1){
		os_waitForInput();
		os_waitForNoInput();

		// Bit das aktuell verändert wird
		uint8_t currentBit = 7;
	
		uint8_t approx = 0b10000000;	
		while (1){
			PORTB = approx;
			_delay_ms(50);

			// auslesen von Pin C0 
			uint8_t pinState = PINC & 00000001;

			if (pinState > 0){
				approx = approx & ~(1 << currentBit);
			}
			
			if (currentBit <= 0){
				PORTA = ~approx;
				break;
			}
			approx = approx | (1 << (currentBit-1));
			PORTA = ~approx;
			
			currentBit--;
			_delay_ms(50);
		}
	}
}


void innit(void){
	// D auf Eingang
	DDRD = 0b00000000;
	PORTD = 0b11111111;
	
	// A & B auf Ausgang
	DDRA = 0b11111111;
	
	DDRB = 0b11111111;
	
	// Pin C konfig
	DDRC &= 0b11111110;
	PORTC |= 0b00000001;
}

int main(void) {
	/* Replace with your application code */
	innit();
	switch(programm) {
		case 0:
			manuell();
			break;
		case 2:
			sukAppWandler();
			break;
		case 1:
			trackWandler();
			break;
	}
}