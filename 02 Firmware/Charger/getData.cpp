/*
 * getData.cpp
 *
 *  Created on: 7 Jan 2016
 *      Author: eddie
 */

#include <msp430.h>
#include "header.h"

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


//TODO: Do a running average of the ADC readings!
void getData(void) {
	// Get the current going into the battery (positive) or
	// being discharged from the battery (negative). It's only
	// to be used for purposes of joule counting, so arbitrary
	// ADC units are fine to use.
	// Read both the reference voltage and output voltage for INA199
	// and use the difference as the current reading. The reference
	// voltage is provided by a potential divider, so this method
	// eliminates common-mode errors.)
	ChargingCurrent = readADCChannel(REF1V_PIN) - readADCChannel(CURRENTV_PIN);

	// Read the Battery Voltage
	BatteryVoltage = readADCChannel(BATTV_PIN);

	// Save a static version of the P2IN register before checking for Stat1 and Stat2 (to avoid strange results in unlikely case of switching at the same time as checking)
	char P2IN_saved = P2IN;

	// Check Stat1 (on when charging)
	Stat1 = !(P2IN_saved & BIT2);

	// Check Stat2 (on when charge complete)
	Stat2 = !(P2IN_saved & BIT1);

}


