#include <msp430.h>
#include "header.h"

// Setup functions to turn cell bleedingMOSFETs on,
// basically just set the corresponding port bit,
// except for Cell 1 which is the opposite.
// Check initialise file for changes to pin numbers.
void bleedOn(char cellIndex) {
	switch (cellIndex) { 		// Note it's index, not cell number, i.e. CELL1 is 0!
	case 0:
		P2OUT &= ~BIT1;
		break;
	case 1:
		P2OUT |= BIT5;
		break;
	case 2:
		P2OUT |= BIT4;
		break;
	case 3:
		P2OUT |= BIT2;
		break;
	}
}

// To turn cell bleeding MOSFETS off just do exactly
// the opposite
void bleedOff(char cellIndex) {
	switch (cellIndex) { 		// Note it's index, not cell number, i.e. CELL1 is 0!
	case 0:
		P2OUT |= BIT1;
		break;
	case 1:
		P2OUT &= ~BIT5;
		break;
	case 2:
		P2OUT &= ~BIT4;
		break;
	case 3:
		P2OUT &= ~BIT2;
		break;
	}
}

void balanceCells(void) {
	for (char i = 0; i < 4; i++) {
		// Check each cell and start bleeding them
		// if appropriate, i.e.:
		// - cell is not currently being bled AND...
		// - PV voltage is present AND...
		// - cell voltage is too high
		if ( ~cell_bleedingOn[i] && (av_cell_values[i] >= maxCellV) && (av_ADC_values[4] >= lowPV) ) {
			bleedOn(i);
			cell_bleedingOn[i] = true;
		}
		// Also check if it is appropriate to stop
		// bleeding them, i.e.:
		// - cell is currently being bled AND...
		// - PV voltage has gone OR...
		// - cell voltage has decreased to minBleedV
		// - batteryFull has been flagged
		if ( cell_bleedingOn[i] && ((av_cell_values[i] < minBleedV) || (av_ADC_values[4] < lowPV) || batteryStatus == 1) ) {
			bleedOff(i);
			cell_bleedingOn[i] = false;
		}
	}
}

void refreshCharge(void) {
	// If cell voltages are too high or PV voltage is too low, or battery is full
	// then increase duty cycle to throttle voltage!
	// (Unless we have already hit the highest permissable duty cycle already, if so then skip...)
	if ( ((av_cell_values[maxCell] >= maxCellV) || (av_ADC_values[4] < PVmpp) || (batteryStatus == 1) ) && (TACCR1 < maxDuty) ) {
		TACCR1++;
	}
	// Otherwise (as long as we have not already hit duty cycle == 0)
	// decrease duty cycle to increase voltage!
	else if (TACCR1 != 0) {
		TACCR1--;
	}
	// Now bleed some current out of fully charged cells if required
	balanceCells();
}
