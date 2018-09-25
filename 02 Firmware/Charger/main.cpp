/*
 * main.cpp
 *
 *  Created on: 14 Dec 2015
 *      Author: eddie
 */

/* TIME-CRITICAL THINGS TO CHECK BEFORE RELEASING!
 * 1.) Is there enough time between the quick fuse reset (setting the fuseTrip pin to low impedance, and the initialisation routine (setting the fuseTrip pin back to high impedance), for the fuse to have tripped? Otherwise need to insert a delay.
 * 2.) With a 1MHz clock time 100 cycles is a long enough pulse duration to trip/reset the fuse. So 200 is used. But if clock cycle is increased then this will also need to be increased proportionately!
 *
 *
 * */

#include <msp430.h>
#include "header.h"



// Declaration of global variables that cross source files
unsigned int ChargingCurrent;
unsigned int BatteryVoltage;
bool Stat1;
bool Stat2;
char BatteryStatus;  // Starts at 0!
unsigned int loop_counter;

// Calibrated voltage thresholds
unsigned int minBattV;
unsigned int restartDischV;

// Pointer definition for loading ADC calibration data from flash memory
// (for some reason these are not pre-defined in the header file)
unsigned int *CAL_ADC_25VREF_FACTOR = (unsigned int *) 0x10E6;
unsigned int *CAL_ADC_GAIN_FACTOR = (unsigned int *) 0x10DC;
int *CAL_ADC_OFFSET = (int *) 0x10DE;

int main(void) {

	// Trip the fuse immediately, because if the contact
	// bounces right the capacitor holding the fuse in tripped
	// status can get charged, resetting the fuse on boot.
	initialFuseTrip();

	// Initialise all the internal systems
	initialise();

	// Delay a short while, as this prevents the "FuseTripped"
	// input from being misread, as may be caused if booted from
	// a hot, bouncy connection to a battery.
	// Also nice to flash LEDs a bit just for debugging purposes.
	flashLED(1,3,1,1,3);

	/* The following loop is carried out where:
	 * 1. Data is measured from the system
	 * 2. Decisions are make as to whether actions need to be taken
	 * 3. If actions need to be taken, enter a "dynamic mode" signaling to the rest of the loop that actions need to be taken
	 * 4. If we're already in a "dynamic mode", then a loop has already carried out all the actions, return to a "static mode"
	 * 5. If we've just entered a "dynamic mode" then all the other methods carry out the necessary actions required.
	 * */

	for(;;) {
		// Pat watchdog to avoid unintentional reset
		patWatchdog();
		// Measure data from ADCs and digital inputs
		getData();
		// Use battery voltage data to check for deep-
		// discharge, or to check when deep-discharge
		// protection can be safely switched off.
		// Also, exit "dynamic modes" after a single
		// cycle carries out all necessary activities.
		refreshBatteryStatus();
		refreshJouleCounter();
		refreshCharge();
		refreshDischarge();
		refreshLEDs();
	}

	return 0;
}

