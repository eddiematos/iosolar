/*
 *
 * logTemp() does the following things in relation to the MCU temperature:
 *  - runs only once every 0xFFFF cycles to avoid burdening the system too much, and only if not snoozing
 *  - checks the MCU internal temperature sensor, and calculates a rolling average
 *  - determines whether that's too hot, and does a thermal shutdown if so
 *  - saves the maximum historical temperature in persistent flash, for field checking
 *
 */

#include <msp430.h>
#include "header.h"

void flashWriteMaxTemp(unsigned int value) {
	FCTL1 = FWKEY + ERASE;                    // Set Erase bit
	FCTL3 = FWKEY;                            // Clear Lock bit
	*maxTemp_FLASH = 0;                       // Dummy write to erase Flash segment
	FCTL1 = FWKEY + WRT;                      // Set WRT bit for write operation
	*maxTemp_FLASH = value;            	      // Write value to flash
	FCTL1 = FWKEY;                            // Clear WRT bit
	FCTL3 = FWKEY + LOCK;                     // Set LOCK bit
}

void checkReboot(void) {
	// If we haven't got a copy of the highest temp recorded in RAM
	// (i.e. if we've just had a reboot) then get one from Flash
	if ( maxTemp_RAM == 0 ) {
		// Update the RAM copy of the max temp with the copy saved
		// in information memory.
		maxTemp_RAM = *maxTemp_FLASH;
		// If this is the default value (i.e. the chip has just been
		// programmed, then set both to 1.
		if (maxTemp_RAM == 0xFFFF) {
			// Write to RAM version first (easy)
			maxTemp_RAM = 1;
			// Now write to Flash version
			flashWriteMaxTemp(1);
		}
	}
}

void getAvTemp(void) {
	// Slow down the ADC to bring the sampling time within the 30us limit.
	// Easiest way is simply to change the ADC clock source to ACLK (12kHz VLO).
	// Let's also change the Vref down to 1.5V, to improve resolution
	// Start by saving current ADC10CTL1 status (for the clock) and current ADC10CTL0 status (for the voltage reference)
	unsigned int ADC10CTL0_current = ADC10CTL0;
	unsigned int ADC10CTL1_current = ADC10CTL1;
	// Now change the voltage reference to 1.5V
	// And change clock source to ACLK
	ADC10CTL0 &= ~REF2_5V;
	ADC10CTL1 = (ADC10CTL1 & ~0b11000) | ADC10SSEL_1;
	// Give voltage reference at least 30us to settle
	__delay_cycles(250);
	// Get the ADC reading from internal temperature sensor
	unsigned int tempADC = readADCChannel(10);  // Channel 10 is the internal temperature sensor
	// Great, now that's done put the ADC settings back to how they were
	ADC10CTL0 = ADC10CTL0_current;
	ADC10CTL1 = ADC10CTL1_current;
	// Give voltage reference at least 30us to settle
	__delay_cycles(250);
	// Get the rolling average for the tempADC reading
	// (see updateAverages() in ADCs.cpp for explanation)
	av_tempADC *= 3;
	av_tempADC += tempADC;
	av_tempADC += temp_dropped_bits;
	temp_dropped_bits = av_tempADC & 0b11;
	av_tempADC = av_tempADC >> 2;

}

void logTemp(void) {
	// Only check max temp every time the counter is reset
	// because it takes ages to do...
	if (flashReady != 0) {
		flashReady--;
	}
	// And also, only if we're not snoozing...
	else if (P2SEL != 0) {
		// Make sure our RAM and Flash values for the maximum recorded temperature
		// are correctly set since a reboot
		checkReboot();

		// Get the average temperature
		getAvTemp();

		// Now compare the current average with the maximum average
		// value in RAM
		if (av_tempADC > maxTemp_RAM) {
			// Then update maxTemp, first in RAM
			maxTemp_RAM = av_tempADC;
			// And secondly in Flash (so that it survives a reset/sleep)
			flashWriteMaxTemp(av_tempADC);
		}

		// Finally, check if the temp is so high that we need to do a shutdown!
		if (av_tempADC >= shutdownTemp) {
			// Start by slowing down the CPU to save battery during this shutdown
			// and to force stop charging.
			goToSnooze();
			// Also stop discharge as this also adds to heat, and should draw attention
			// to problem
			closeGate();
			// Flash red LEDs slowly for while
			flashLED(1,0,tempShutdownBlinkDuration,tempShutdownBlinkDuration,tempShutdownBlinkNumber);
			// Now it has had a chance to cool, we can start discharge again
			openGate();
			// Also we can speed up the CPU again and resume charging.
			wakeUpFromSnooze();
		}

		// Now reset the counter to make sure we don't run this method too often
		flashReady = 0xFFFF;

	}
}
