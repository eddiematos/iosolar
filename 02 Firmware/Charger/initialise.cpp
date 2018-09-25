/*
 * initialise.cpp
 *
 *  Created on: 7 Jan 2016
 *      Author: eddie
 */

#include <msp430.h>
#include "header.h"

void patWatchdog(void) {
	/* Watchdog Timer+ Register settings:
	 * WDTPW		- Must preface any write to WDTCTL with this password
	 * WDTCNTCL		- Might as well take the opportunity to clear the "pat the dog" now
	 * WDTSSEL		- Set to ACLK so still counting when in LPM3
	 * WDTISx		- Not currently set, decreases the number of counts before reset kicks in, down from 2^15 (32768)
	 * */
    WDTCTL = WDTPW + WDTCNTCL + WDTSSEL;	// Set watchdog timer to do 32k clocks before resetting, with ACLCK on VLO/4 (see DIVA_X in BCSCTL1 register set): 12kHz / 4 = 3kHz, 32kSteps / 3kHz = 11s !!!
}

void initialiseClock(void) {

	// In general policy here is to keep default DCO settings and source MCLK (for CPU)
	// and SMCLK (for PWM) from it. ACLK (for the WDT) is given a divisor to further extend
	// the reset time.

	/* DCO Control Register settings:
	 * These settings modify the speed of the DCO clock. It varies from chip
	 * to chip so calibration values are provided in flash to get some accurate
	 * readings.
	 * The setting is saved in the header file so that it may be safely copied
	 * to the considerSnooze() function. */
	DCOCTL = CALDCO_1MHZ;

	/* Basic Clock System Control Register 1 settings:
	 * Modifies the DCO frequency along with DCOCTL (bits 0:3), and so another calibration
	 * value is provided. But as register also controls ACLK divider (which is
	 * used for watchdog timer here, we must also set those bits.
	 * DIVA_0		- Sets ACLK divider to 1 (32768 counts, ~12kHz clock speed (VLO), total Watchdog time (s) = DIVA * 32768 / 12000
	 * XT2OFF		- Keeps X2T off (though MSP430G2XX2 doesn't even have this functionality, but just in case)
	 * This setting is saved in the header file so that it may be safely copied
	 * to the considerSnooze() function. */
	BCSCTL1 = (XT2OFF + DIVA_0) | (0x0f & CALBC1_1MHZ);

	/* Basic Clock System Control Register 2 settings:
	 * Both MCLK and SMCLK source and divider are here if needed.
	 * For now let's keep the defaults for these (=0), i.e. both are
	 * sourced from DCO, with divider = 1. */

	/* Basic Clock System Control Register 3 settings:
	 * LFXT1S_2 	- Use VLO (12kHz) as the source for LFXT (low frequency crystal), and ACLK, thereby minimising power consumption during sleep mode */
	BCSCTL3 |= LFXT1S_2;
}

void initialiseIO(void) {
	/* IO Pin descriptions
	 * P1.0 - Low power ground bus fuse status digital input
	 * P1.3 - The battery voltage, ADC, configure as input
	 * P1.4 - The 1V reference being fed into the current sensor, ADC, configure as input
	 * P1.5 - Charge/discharge ADC current reading, ADC, configure as input
	 * P1.6 - Green LED output
	 * P1.7 - Red LED output
	 * P2.0 - Fuse status digital input
	 * P2.1 - STAT2, pulled low when "charge complete", high-impedance input with pull-up internal resistor enabled
	 * P2.2 - STAT1, pulled low when "charging", high-impedance input with pull-up internal resistor enabled
	 * P2.3	- Gate pin of MOSFET to enable/disable charging, initialised to 0 to enable charging from start
	 * P2.4 - Fuse trip signal, bring low to trip the fuse
	 * P2.5 - Fuse reset signal, bring low to reset the fuse
	 * */

	// Output states are not reset on powerup, so should reset at the start.
	// Also, tripFuse() and resetFuse() assume their pin outputs to be LOW.
	// STAT inputs are set high to set internal resistors to pull-up mode
	P1OUT = 0;
	P2OUT = BIT1 + BIT2;

	// Pull-up resistor enable on STAT inputs
	P2REN = BIT1 + BIT2;

	// Set port directions (1 means output, 0 means input), high-impedance digital outputs (pull-up/pull-down) stay defined as inputs.
	P1DIR = BIT6 + BIT7;
	P2DIR = BIT3;
}

void initialiseADC(void) {
	/* ADC Control register 1 settings:
	 * SHS_0		- "Sample and Hold Source", ADC sampling is triggered by setting ADC10SC (not triggered by timers)
	 * ADC10DIV_0	- Clock divisor set to 1
	 * ADC10SSEL_2	- ADC clock set to MCLK */
	ADC10CTL1 = SHS_0 + ADC10DIV_0 + ADC10SSEL_2; // Remember setting: SHS_0 + ADC10DIV_0 + ADC10SSEL_2
	/* Analog pin configuration
	 * "Analog Enable" - Disables port pin buffer on pins being used for analog
	 * sensing (eliminates risk of possible parasitic current increasing power
	 * consumptionfrom analog signals on these pins that are near the digital
	 * threshold level) */
	ADC10AE0 = (1<<BATTV_PIN) + (1<<REF1V_PIN) + (1<<CURRENTV_PIN);
	/* ADC Control register 0 settings:
	 * ADC10ON		- enables ADC, disable before going to sleep to save power
	 * REFON 		- enables use of internal voltage reference - switch off as using external reference
	 * SREF_1		- uses voltage reference as ADC reference, and Vss (GND) as zero
	 * REF2_5V 		- sets internal voltage reference to 2.5V - irrelevant as not using internal voltage reference
	 * ADC10SR		- "Sample Rate" bit reduces current consumption (sampling rate must be < 50ksps) Setting this saves us about 0.5mA, on paper. But it's not clear whether or not we can... so don't set it for now.
	 * ADC10SHT_3	- "Sample and Hold Time", sets sampling period to 64 clock cycles
	 *
	 * Note that, in accordance with section 22.2.5.1 of the user guide SLAU144I (pg. 552), the sample time
	 * must be at least 0.41us, assuming zero external input resistance, or - as in our case - an input reserve
	 * capacitance many times larger than the internal sampling capacitor of 27pF.
	 *
	 * Much harder to meet is the requirement for the 200,000 maximum number of samples per second.
	 * */

	ADC10CTL0 = ADC10ON + REFON + SREF_1 + REF2_5V + ADC10SHT_0; // 8MHz with 4 clock cycles sample time -> sample time of 0.5us
}

// Calibrate the voltage thresholds so that the ADC readings can be directly compared with
// no further calibration. See "Voltage threshold calibration.doc"
unsigned int secondStageCalibration(unsigned int uncalibratedThreshold, float _ADC_coeff) {
	// Let's create a variable to hold our ongoing work on the calibrated threshold,
	// and start by subtracting the offset calibration (supposed to be careful with the sign of
	// *CAL_ADC_OFFSET, but a bit of experimentation on codepad.org shows that the below simply
	// works as expected)
	unsigned int calibratedThreshold = uncalibratedThreshold - *CAL_ADC_OFFSET;
	// Now simply "float multiply" by the product coefficient calculated in the parent method.
	// Adding 0.5 to the result is just a trick to ensure that the conversion to int results
	// in rounding the float, rather than just truncating the numbers after the decimal place.
	calibratedThreshold = (calibratedThreshold * _ADC_coeff) + 0.5;
	// That's it. Now return the unsigned int.
	return calibratedThreshold;
}

// There are two steps in using the calibration data in information memory to calibrate
// the voltage thresholds. The first is to calculate the two factor (float) coefficients, to avoid
// doing this float division too many times. Then multiply them together to get a net factor and
// avoid doing this float multiplication too many times. The second step is to float multiply this
// coefficients with each threshold, after applying the offset calibration.
void calibrateThresholds(void) {
	// Firstly calculate the two "factor" calibration coefficients. The division must be "float
	// division", so the numerators are cast as such.
	float ADC_coeff1 = (float) 32768 / *CAL_ADC_25VREF_FACTOR;
	float ADC_coeff2 = (float) 32768 / *CAL_ADC_GAIN_FACTOR;
	// Now "float multiply" them together to get a single float coefficient
	float ADC_coeff_product = ADC_coeff1 * ADC_coeff2;
	// Now the second multiplication is applied to each threshold separately
	// Plenty of truncation going on here, but it's all checked and safe
	minBattV = secondStageCalibration(minBattV_uncalib, ADC_coeff_product);
	restartDischV = secondStageCalibration(restartDischV_uncalib, ADC_coeff_product);
}

void initialise(void) {
	patWatchdog();
	initialiseClock();
	initialiseIO();
	initialiseADC();
	calibrateThresholds();
}
