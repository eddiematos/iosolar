/*
 * refreshLEDs.cpp
 *
 *  Created on: 7 Jan 2016
 *      Author: eddie
 */

#include <msp430.h>
#include "header.h"

// 0: off, 1: red, 2: yellow, 3: green
void setLEDs(char _colour) {
	switch (_colour) {
	// off
	case 0:
		P1OUT &= ~BIT6;
		P1OUT &= ~BIT7;
		break;
	// red
	case 1:
		P1OUT &= ~BIT6;
		P1OUT |= BIT7;
		break;
	// yellow
	case 2:
		P1OUT |= BIT6;
		P1OUT |= BIT7;
		break;
	// green
	case 3:
		P1OUT |= BIT6;
		P1OUT &= ~BIT7;
		break;
	}
}

// Flash LEDs, locking MCU in the process, ends with LED off
void flashLED(char colour_on, char colour_off, char blinkDuration_on, char blinkDuration_off, unsigned int blinkNumber) {
	// Carry out some waiting loops and do some flashing whilst we're at it!
	for (unsigned int i = 0; i < blinkNumber; i++) {
		// Don't forget to pat watchdog or we'll reset!
		patWatchdog();
		// Set on LED
		setLEDs(colour_on);
		// Delay loop
		for (char j = 0; j < blinkDuration_on; j++) {
			__delay_cycles(125000ul);  // About 1/8th of a second
		}
		// Don't forget to pat watchdog or we'll reset!
		patWatchdog();
		// Set off LED
		setLEDs(colour_off);
		// Delay loop
		for (char j = 0; j < blinkDuration_off; j++) {
			__delay_cycles(125000ul);  // About 1/8th of a second
		}
	}
	// Don't forget to pat watchdog or we'll reset!
	patWatchdog();
}



// Off if in deep discharge
// Red if at rest
// Orange if charging
// Green if fully charged
void refreshLEDs(void) {
	switch (BatteryStatus) {
		case 1:		// Just reached deep discharge disconnect, flash the LED and turn it off!
			flashLED(1, 0, SHORT_FLASH_TIME, SHORT_FLASH_TIME, SHORT_FLASH_NUMBER);
			setLEDs(0);
			break;
		case 2:		// We're charging, turn orange LED on (this is a bit of an exception with regards to the "dynamic/static mode" paradigm as we're constantly checking this) sorry about that inconsistency.
			setLEDs(2);
			break;
		case 3:		// We're at rest, turn red LED on (this is a bit of an exception with regards to the "dynamic/static mode" paradigm as we're constantly checking this) sorry about that inconsistency.
			setLEDs(1);
			break;
		case 5:		// We've reached full capacity, switch to green
			setLEDs(3);
			break;
		}
}

