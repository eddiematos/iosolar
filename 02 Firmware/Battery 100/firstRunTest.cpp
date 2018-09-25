#include <msp430.h>
#include "header.h"

// Method to write to the testResult char in Flash, so initialise can detect: (i) that a first run test
// has been carried out and (ii) what were the results of that test. The code is as follows:
// result 0: good test
// result 1: possible bad battery?
// result 2: possible bad solar panel?
// result 3: possible bad PCB?
void writeTestResult(char result) {
	FCTL1 = FWKEY + ERASE;                    // Set Erase bit
	FCTL3 = FWKEY;                            // Clear Lock bit
	*testResult = 0;                       	  // Dummy write to erase Flash segment
	FCTL1 = FWKEY + WRT;                      // Set WRT bit for write operation
	*testResult = result;            	      // Write value to flash
	FCTL1 = FWKEY;                            // Clear WRT bit
	FCTL3 = FWKEY + LOCK;                     // Set LOCK bit
}

// Use the existing functions to get a nice average value
// for the cell voltages and the PV voltage. Not much use
// to test output because the gate's not open yet. But we'll do that later.
// Return the baseline battery voltage value
unsigned int acquireBaselines(void) {
	for (unsigned int j = 0; j < acquireBaselinesLoops; j++) {
		for (char i = 0; i < 5; i++) {
			// Read the ADC channel and update average
			updateAverage(readADCChannel(ADC_CH_numbers[i]), i);
			// If it's a battery cell channel, then:
			// - update cell values as relevant
			if (i < 4) {
				av_cell_values[i] = av_ADC_values[i];
				if (i != 0)
					av_cell_values[i] -= av_ADC_values[i - 1];			// Subtract previous absolute value to cell value
			}
		}
		patWatchdog();
	}
	// Return the averaged full battery voltage value
	return av_ADC_values[3];
}

// Make sure cell voltages and PV voltage are within prescribed limits
// Returns true if failed
bool testBaselines(void) {
	// First for cell voltages
	for (char i = 0; i < 4; i++) {
		if (av_cell_values[i] < minCellV || av_cell_values[i] > maxCellV) {
			// Problem with cell readings, so write error number 4.
			writeTestResult(4);
			return true;
		}
	}
	// Now for the PV voltage
	if (av_ADC_values[4] < (PVmax - PVtolerance) || av_ADC_values[4] > (PVmax + PVtolerance) ) {
		// PV voltage not detected correctly, so write error number 3.
		writeTestResult(3);
		return true;
	}
	// If we made it here then so far so good!
	return false;
}

// Here we charge the battery for a bit using the power available to us
// and after a set period we check the battery voltage.
// If it's not increased then we fail the test by returning true.
bool testCharge(unsigned int _baselineBattV) {
	for (unsigned long j = 0; j < chargeTestLoops; j++) {
		// Run basic operations for charging with this prototype main loop
    	patWatchdog();
        refreshADCs();
        refreshCharge();
	}
	// Now disable charging (no need to worry about re-enabling charging
	// as we're going to reset before ever needing to charge again
	P2SEL = 0;
	// Check net change in cell diff. If the battery voltage has not increased
	// by a minimum amount as a result of the charging then fail the test.
	if ( av_ADC_values[3] < _baselineBattV + testChargeMargin )  {
		// Possible problem with PCB as seems to be not charging... but could also be solar panel
		// so write error code 5
		writeTestResult(5);
		return true;
	}
	// Otherwise, we're all good - so pass the test
	return false;
}


// Open the discharge gate and check that the fuse voltage is there!
// Otherwise fail the test by returning true.
bool testDischarge(void) {
	openGate();
	// Get some new voltage averages
	for (int i = 0; i < acquireBaselinesLoops; i++) {
		updateAverage(readADCChannel(ADC_CH_numbers[3]), 3);
		updateAverage(readADCChannel(ADC_CH_numbers[5]), 5);
		__delay_cycles(2000);
		patWatchdog();
	}
	// Test to make sure the voltage drop across the fuse isn't too big
	// during discharge
	if (av_ADC_values[3] > av_ADC_values[5] + maxFuseDrop ) {
		// Failed test, so close the gate
		closeGate();
		// Probably problem with PCB, so write error code 7
		writeTestResult(7);
		return true;
	}
	// If we passed, then that's basically the end of the test, so leave the gate open
	// during the "success" LED flashes so that the tester has a chance to continue testing the
	// LED lights until it turns off.
	return false;
}

// A series of tests are run. Any test can conclude a fail, which will write this to Flash
// before going to sleep. If we reach the end of this function without fail, then
// the test will be passed. It will go into an inactive loop for 10 mins, to give the tester
// a chance to remove the power supply, then it will go into the usual "sleep" with PV checks.
void firstRunTest(void) {
	// Turn on green LED to show that the test has started
	setLEDs(3);
	// Get the voltage baselines for the cells and the solar panel
	// Save the baseline battery voltage separately as we'll use
	// it later.
	unsigned int baselineBattV = acquireBaselines();		// This is instead of creating another global variable just for this purpose.
	// Test the voltage baselines and exit if we've already failed
	if ( testBaselines() )
		return;
	// Change LED colour to yellow to show we've moved to the next stage
	setLEDs(2);
	// Now do some charging and make sure cell voltages are increasing
	// Exit if we've already failed
	if ( testCharge(baselineBattV) )
		return;
	// Change LED colour to red to show we've moved to the next stage
	setLEDs(1);
	// Finally, open the discharge gate and check that we've got a discharge
	// voltage. Also, deduce exit voltage drop to make sure not too high
	// Exit if we've already failed
	if ( testDischarge() )
		return;
	// That's it! The test is over. Write success result to flash and return
	writeTestResult(0);
}

