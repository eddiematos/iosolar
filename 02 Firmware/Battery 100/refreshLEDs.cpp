#include <msp430.h>
#include "header.h"

// 0: off, 1: red, 2: yellow, 3: green
void setLEDs(char _colour) {
	switch (_colour) {
	// off
	case 0:
		P1OUT &= ~BIT6;
		P2OUT &= ~BIT7;
		break;
	// red
	case 1:
		P1OUT |= BIT6;
		P2OUT &= ~BIT7;
		break;
	// yellow
	case 2:
		P1OUT |= BIT6;
		P2OUT |= BIT7;
		break;
	// green
	case 3:
		P1OUT &= ~BIT6;
		P2OUT |= BIT7;
		break;
	}
}

// Flash LEDs, locking MCU in the process, ends with LED off
void flashLED(char colour_on, char colour_off, char blinkDuration_on, char blinkDuration_off, unsigned int blinkNumber) {
	// Save current clock state
	char DCOCTL_old = DCOCTL;
	char BCSCTL1_old = BCSCTL1;
	// Set 1MHZ clock state (could be any other speed, it's just to make sure loops are always the same length)
	DCOCTL = CALDCO_1MHZ;
	BCSCTL1 = (BCSCTL1 & ~0x0f) | (0x0f & CALBC1_1MHZ);
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
	// Waiting is over, set clock back to whatever it was before
	DCOCTL = DCOCTL_old;
	BCSCTL1 = BCSCTL1_old;
}

void refreshLEDs(void) {
	// Consider LED colour depending on current LEDStatus
	// 0: off, 1: red, 2: yellow, 3: green
	switch (LEDStatus) {
	case 0:
		// If discharge gate has opened (by refreshDisharge()) then upgrade to red
		if ( batteryStatus == 0 )
			LEDStatus++;
		break;
	case 1:
 		// If cell voltage is too high for this status, upgrade to yellow
		if (av_cell_values[minCell] >= (LEDthresh2 + LEDthreshHyst) )
			LEDStatus++;
		// If discharge gate has shut due to low-batt voltage
		// again then do some red/off flashing and downgrade to off
		if ( batteryStatus == 2 ) {
			flashLED(1,0,shortCircuitBlinkDuration,shortCircuitBlinkDuration,standardBlinkNumber);
			LEDStatus--;
		}
		break;
	case 2:
 		// If min cell is above LED thresh1, AND we start doing any cell bleeding, or if we stop charging because of full battery, then upgrade to green
		if ( (av_cell_values[minCell] >= (LEDthresh1 + LEDthreshHyst) ) && ( (batteryStatus == 1) || cell_bleedingOn[0] || cell_bleedingOn[1] || cell_bleedingOn[2] || cell_bleedingOn[3] ) )
			LEDStatus++;
 		// If cell voltage is too low for this status, downgrade to red
		if (av_cell_values[minCell] < LEDthresh2 )
			LEDStatus--;
		break;
	case 3:
 		// If cell voltage is too low for this status, downgrade to yellow
		if (av_cell_values[minCell] < LEDthresh1 )
			LEDStatus--;
		break;
	}
	// Now change LED colour according to LED status
	setLEDs(LEDStatus);
}

