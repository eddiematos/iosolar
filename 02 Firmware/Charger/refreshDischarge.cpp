/*
 * refreshDischarge.cpp
 *
 *  Created on: 7 Jan 2016
 *      Author: eddie
 */


#include <msp430.h>
#include "header.h"

// This gets its own method so that it can be done first
// thing as soon the MCU is booted. In theory the capacitor on
// the "fuse trip" line should be an analog way to ensure
// the fuse powers up already tripped. In practice it is
// still possible (e.g. if the contact connecting the battery
// bounces and connects again about one second after initial
// connection which can apparently charge the capacitor) for
// the fuse to start up untripped.
void initialFuseTrip(void) {
	P2OUT &= ~BIT4;		// Set fuse setting pin port to ground ( TODO: only necessary as this register's factory setting is undefined, so relevant for first boot. Neither flip-flop status pin will ever be set to high we could also just make sure these are set to zero by direct manipulation/double-check of the hex binary file.
	P2DIR = BIT4;		// Make it a low impedance output
	__delay_cycles(FLIPFLOP_DELAY);
	P2DIR = 0;
}

void tripFuse(void) {
	// The pin output has already been set to zero during initialisation
	// so just need to make it low impedance for a few cycles.
	P2DIR |= BIT4;
	__delay_cycles(FLIPFLOP_DELAY);
	// Switch it back to high impedance
	P2DIR &= ~BIT4;
}

void resetFuse(void) {
	// The pin output has already been set to zero during initialisation
	// so just need to make it low impedance for a few cycles.
	P2DIR |= BIT5;
	__delay_cycles(FLIPFLOP_DELAY);
	// Switch it back to high impedance
	P2DIR &= ~BIT5;
}

void refreshDischarge(void) {

	// Open or close gate according to battery status
	switch (BatteryStatus) {
		case 1:		// Low battery! Gate needs closing!
			tripFuse();
		case 4:		// Battery recovered from deep discharge! Lets open the gate.
			resetFuse();
	}

	// Check for short-circuits (good to check straight after reseting a fuse, though it could also be too quick!)
	// and if so (re-) trip the fuse (good to drain any remaining charge if already tripped by analog
	// and immediately relevant if tripped by ground bus fuse) and flash some lights fast!
	if ( !(P2IN & BIT0) || (P1IN & BIT0) ) {  // "FuseTripped" OR "GroundBusTripped"
		tripFuse();
		flashLED(1, 0, SHORT_FLASH_TIME / 4, SHORT_FLASH_TIME / 4, SHORT_FLASH_NUMBER * 4);
		resetFuse();
	}

}
