#include <msp430.h>
#include "header.h"


// Commands to open and close the discharge gate
// (P2.3 - Check initialise file for changes)
void openGate(void) {P2OUT |= BIT3;}
void closeGate(void) {P2OUT &= ~BIT3;}

void refreshDischarge(void) {
	switch (batteryStatus) {
	case 0:
		// Battery normal, keep gate open
		openGate();
		break;
	case 1:
		// Battery is full, stop charging, keep gate open
		openGate();
		break;
	case 2:
		// Low battery, close gate
		closeGate();
		break;
	case 3:
		// Short circuit, close gate, stop charging,
		// flash red LED, wait a bit, and return to
		// normal battery status. (If couldn't be "empty",
		// and if it was "full" before, then setting it
		// back to normal won't do any harm.
		// Start by giving the MOSFET a few ms to cool down after
		// handling such a big current surge.
		//__delay_cycles(2400000ul);
		// No, actually, don't do it. It might just result in repeated current
		// surges if PTC fuse break causes load to disconnect, in turn causing
		// PTC fuse to recover again, causing wildly oscillating currents.
		closeGate();
		TACCR1 = maxDuty;
		// Flashing is handled here instead of in refreshLEDs()
		// because that function is already quite complicated to
		// account for state transitions and hysteresis - so it does
		// not need the possibility of a short at any time, in any
		// state to make it more so!
		flashLED(1,0,lowBattBlinkDuration,lowBattBlinkDuration,standardBlinkNumber);
		batteryStatus = 0;
		openGate();
		break;
	}
}
