/*
 * This is the "awake" mode of the unit, during which it runs a game loop:
 * read data, take action, update display. A flag exists to keep the loop
 * going and is set to false if the battery voltages go too low. In which
 * case the loop is broken and the function is returned.
 *
*/

#include <msp430.h>
#include "header.h"

// Update the rolling average value of the given index, with the given new ADC value
void updateAverage(unsigned int _ADC_value, char _i) {
	// Gives a bit of "inertia" to the cell voltage readings.
	// Each new reading is 1 of 32 samples for a new average
	// each time.
	// 31 of those samples are the last value.
	av_ADC_values[_i] *= 31;
	// The last sample is the new value.
	av_ADC_values[_i] += _ADC_value;
	// Add the previous cycle's dropped bits; better than a remainder! (see "Rolling average problem.xlsx")
	av_ADC_values[_i] += dropped_bits[_i];
	// Save the bits that are about to be dropped in this cycle, for adding on again next cycle
	dropped_bits[_i] = av_ADC_values[_i] & 0b11111;
	// Drop the bits to complete the division
	av_ADC_values[_i] = av_ADC_values[_i] >> 5;
}

void backToSleep(void) {
	// Shutdown ADC (in case it's not done automatically)
	ADC10CTL0 = 0;
	// Pat watchdog
	patWatchdog();
	// Enter Low Power Mode 3 (turns off CPU and stops code
	// here until Watchdog timer kicks in and reboots the MCU)
	LPM3;
}

unsigned int readADCChannel(char channel) {
	// Select ADC channel to sample and convert.
	// Channel to convert is defined by INCHx, in the upper 4 bits in ADCCTRL1
	ADC10CTL1 = (ADC10CTL1 & ~0xf000) | ( (channel << 12) & 0xf000);
	// Issue instruction to sample and convert, by setting ADC10SC and ENC
	// bits in ADCCTRL0
	ADC10CTL0 |= ADC10SC + ENC;
	// Wait whilst sample is being taken, converted, and stored in ADC10MEM
	// by checking the ADC10BUSY bit in ADCCTRL1
	while( (ADC10CTL1 & ADC10BUSY) );
	// Reset ENC bit to allow later modification of ADC10CTL registers
	// (ADC10SC resets itself)
	ADC10CTL0 ^= ENC;
	// Return ADC reading
	return ADC10MEM;
}

// Quickly check PV voltage to see if it's time to wake up
void checkPV(void) {
	// As we have just woken up, and we have only just setup the ADC
	// we need to enable a 30us delay to allow the internal reference
	// voltage to settle
	// __delay_cycles(250); // Disabled to see if we can get away without it!! It's only supposed to be rough after all...

	// Read PV ADC channel and quickly compare to PV minimum threshold
	// to decide whether or not to go back to sleep.
	if( readADCChannel(PV) < lowPV_uncalib)		// We are using an uncalibrated version of lowPV here, to minimise time spent awake, but it's only meant to be rough anyway!
		backToSleep();

	// Otherwise return to main and wake-up fully!
}

// Read analog inputs of all cell voltages, the panel input voltage, and the
// fuse (i.e. discharge current) voltage and save to global variable array.
// Could also check temp?
void refreshADCs(void) {
	// Reinitialise max/min cell values
	minCell = 0;
	maxCell = 0;
	// Start looping through the channels
	for (char i = 0; i < 6; i++) {
		// If it's a battery cell channel, then:
		// - calculate the rolling average
		// - update cell values as relevant
		// - determine which is min and max cell
		if (i < 4) {
			// Read the ADC channel and update the rolling average
			updateAverage(readADCChannel(ADC_CH_numbers[i]), i);
			// Now calculate cell value as relevant
			// and also determine max/min cell
			av_cell_values[i] = av_ADC_values[i];
			if (i != 0) {
				av_cell_values[i] -= av_ADC_values[i - 1];			// Subtract previous absolute value to cell value
				if (av_cell_values[i] < av_cell_values[minCell])	// See if this cell value is smaller than the previous
					minCell = i;
				if (av_cell_values[i] > av_cell_values[maxCell])			// See if this cell value is larger than the previous
					maxCell = i;
			}
		}
		// If it's not a battery cell channel, then simply
		// save the value for later (without rolling average
		// because we need a faster response.
		else {
			av_ADC_values[i] = readADCChannel(ADC_CH_numbers[i]);
		}
	}

// Code for getting temperature and comparing with max version, placed in "refreshADCs()", in ADCs.cpp
#ifdef enableMaxTempLog
	logTemp();
#endif

}
