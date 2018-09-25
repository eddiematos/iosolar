/*
 * This initialises the device after a reset or
 * the first power up.
 *
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
	DCOCTL = DCOCTL_setting;  // CALDCO_8MHZ (unless header file has been updated since)

	/* Basic Clock System Control Register 1 settings:
	 * Modifies the DCO frequency along with DCOCTL (bits 0:3), and so another calibration
	 * value is provided. But as register also controls ACLK divider (which is
	 * used for watchdog timer here, we must also set those bits.
	 * DIVA_0		- Sets ACLK divider to 1 (32768 counts, ~12kHz clock speed (VLO), total Watchdog time (s) = DIVA * 32768 / 12000
	 * XT2OFF		- Keeps X2T off (though MSP430G2XX2 doesn't even have this functionality, but just in case)
	 * This setting is saved in the header file so that it may be safely copied
	 * to the considerSnooze() function. */
	BCSCTL1 = BCSCTL1_setting; // (XT2OFF + DIVA_0) | (0x0f & CALBC1_8MHZ) (unless header file has been updated since)

	/* Basic Clock System Control Register 2 settings:
	 * Both MCLK and SMCLK source and divider are here if needed.
	 * For now let's keep the defaults for these (=0), i.e. both are
	 * sourced from DCO, with divider = 1. */

	/* Basic Clock System Control Register 3 settings:
	 * LFXT1S_2 	- Use VLO (12kHz) as the source for LFXT (low frequency crystal), and ACLK, thereby minimising power consumption during sleep mode */
	BCSCTL3 |= LFXT1S_2;
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
	ADC10AE0 = (1<<CELL1) + (1<<CELL2) + (1<<CELL3) + (1<<CELL4) + (1<<PV) + (1<<DISCURRENT);
	/* ADC Control register 0 settings:
	 * ADC10ON		- enables ADC, disable before going to sleep to save power
	 * REFON 		- enables use of internal voltage reference
	 * SREF_1		- uses internal voltage reference as ADC reference
	 * REF2_5V 		- sets internal voltage reference to 2.5V
	 * ADC10SR		- "Sample Rate" bit reduces current consumption (sampling rate must be < 50ksps) Setting this saves us about 0.5mA, on paper. But it's not clear whether or not we can... so don't set it for now.
	 * ADC10SHT_0	- "Sample and Hold Time", sets sampling period to 4 clock cycles
	 *
	 * Note that, in accordance with section 22.2.5.1 of the user guide SLAU144I (pg. 552), the sample time
	 * must be at least 0.41us, assuming zero external input resistance, or - as in our case - an input reserve
	 * capacitance many times larger than the internal sampling capacitor of 27pF.
	 *
	 * Much harder to meet is the requirement for the 200,000 maximum number of samples per second.
	 * */

	ADC10CTL0 = ADC10ON + REFON + SREF_1 + REF2_5V + ADC10SHT_0; // 8MHz with 4 clock cycles sample time -> sample time of 0.5us
}

void initialiseIO(void) {
	/* IO Pin descriptions
	 * P1.0 / A0		- Cell3 voltage, analog input, configure as input
	 * P1.1 / A1		- Cell4 voltage, analog input, configure as input
	 * P1.2 / A2		- Cell2 voltage, analog input, configure as input
	 * P1.3 / A3		- Cell1 voltage, analog input, configure as input
	 * P1.4 / A4		- PV voltage, analog input, configure as input
	 * P1.5				- Unused, configure as input, pull-down resistor enabled
	 * P1.6				- Red LED, digital output, initial value 0
	 * P1.7				- Fuse (current discharge sense) voltage, analog input, configure as input
	 * P2.0				- Unused, configure as input, pull-down resistor enabled
	 * P2.1				- Cell1 bleed gate, digital output, initial OFF state = 1
	 * P2.2				- Cell4 bleed gate, high impedance digital output (digital input with internal resistors enabled), initial OFF state = 0
	 * P2.3				- Discharge MOSFET gate, digital output, initial OFF state = 0
	 * P2.4				- Cell3 bleed gate, high impedance digital output (digital input with internal resistors enabled), initial OFF state = 0
	 * P2.5				- Cell2 bleed gate, high impedance digital output (digital input with internal resistors enabled), initial OFF state = 0
	 * P2.6 / TA0.1		- Nudge voltage, PWM output, initial value 1 (i.e. nudge charging voltage low)
	 * P2.7				- Green LED, digital output, initial value 0
	 * */

	// Need to set the PWM pin to use the "primary peripheral module" function
	// so that the PWM comes out of here
	P2SEL = BIT6;
	// Set those pins that require initially HIGH output states - the PWM pin (P2.6) doesn't need to be set here, but it's useful for when PWM is disabled for it to enter a high voltage state
	P1OUT = 0;
	P2OUT = BIT1 + BIT6;
	// Set resistor pull-downs on unused pins, and on high-impedance digital output pins (cell bleed gates for cells 2-4)
	P1REN = BIT5;
	P2REN = BIT0 + BIT2 + BIT4 + BIT5;
	// Set port directions (1 means output, 0 means input), high-impedance digital outputs stay defined as inputs
	P1DIR = BIT6;
	P2DIR = BIT1 + BIT3 + BIT6 + BIT7;
}
void initialiseTimer(void) {
	// Mode configuration for PWM taken from code example in
	// MSP430Ware, "MSP430G2xx2 Demo - Timer_A, PWM TA1, Up Mode, 32kHz ACLK"

	/* Timer Capture/Compare Control Register settings:
	 * OUTMOD_7			- "Output mode", set to "reset/set" */
	TACCTL1 = OUTMOD_7;
	/* Timer Capture/Compare Register 0 (counter top-out):
	 * Increasing this will increase the PWM period, but will
	 * also increase the resolution. Max 0xffffh */
	TACCR0 = PWMperiod;   // Start at max voltage PWM to nudge charging voltage downwards
	/* Timer Capture/Compare Register 1 (toggle level):
	 * This defines the duty cycle, so we need to set an inital value. Set
	 * equal to TACCR0 to ensure it starts with the max. voltage on
	 * Vnudge, thereby pulling the charging voltage to its minimum. */
	TACCR1 = maxDuty;
	/* Timer_A Control Register settings (do this last as this is what turns it on):
	 * TASSEL_2			- "Source select", selects clock source of timer as SMCLK
	 * MC_1				- "Mode control", set to continuous up mode - counting up to TACCR0. Set to MC_0 to stop timer and save power when going to sleep. */
	TACTL = TASSEL_2 + MC_1;
}

void initialiseGlobals(void) {
	// Initialise LED status char
	LEDStatus = 0; // off!
	// Initialise Battery status char
	batteryStatus = 2; // Assume empty to avoid constant deep-discharge reseting when clouds go over! (i.e. won't come on until min battery cell voltage exceeds restartDischV), although this initialisation is reduntant as long as we're using av_cell_voltages to decide on battery status, as the first few cycles will show cell voltages close to zero anyway!
	for (int i = 0; i < 4 ; i++)
		av_cell_values[i] = minCellV; // Because of the averaging system, it can take quite a while for a unit to wake up, if the lowest cell voltage is close to restartChargeV. This initialisation can give it a little boost, instead of starting from zero.
}


// Calibrate the voltage thresholds so that the ADC readings can be directly compared with
// no further calibration. See "Voltage threshold calibration.doc"
unsigned int secondStageCalibration(unsigned int uncalibratedThreshold, float _ADC_coeff) {
	// Let's create a variable to hold our ongoing work on the calibrated threshold,
	// and start by subtracting the offset calibration (supposed to be careful with the sign of
	// *CALADC_OFFSET, but a bit of experimentation on codepad.org shows that the below simply
	// works as expected)
	unsigned int calibratedThreshold = uncalibratedThreshold - *CALADC_OFFSET;
	// Now simply "float multiply" by the product coefficient calculated in the parent method.
	// Adding 0.5 to the result is just a trick to ensure that the conversion to int results
	// in rounding the float, rather than just truncating the numbers after the decimal place.
	calibratedThreshold = (unsigned int) (calibratedThreshold * _ADC_coeff) + 0.5;
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
	float ADC_coeff1 = (float) 32768 / *CALADC_25VREF_FACTOR;
	float ADC_coeff2 = (float) 32768 / *CALADC_GAIN_FACTOR;
	// Now "float multiply" them together to get a single float coefficient
	float ADC_coeff_product = ADC_coeff1 * ADC_coeff2;
	// Now the second multiplication is applied to each threshold separately
	// Plenty of truncation going on here, but it's all checked and safe
	PVmpp = secondStageCalibration(PVmpp_uncalib, ADC_coeff_product);
	lowPV = secondStageCalibration(lowPV_uncalib, ADC_coeff_product);
	maxCellV = secondStageCalibration(maxCellV_uncalib, ADC_coeff_product);
	minCellV = secondStageCalibration(minCellV_uncalib, ADC_coeff_product);
	minFuse = secondStageCalibration(minFuse_uncalib, ADC_coeff_product);
	minBleedV = secondStageCalibration(minBleedV_uncalib, ADC_coeff_product);
	stopChargeV = secondStageCalibration(stopChargeV_uncalib, ADC_coeff_product);
	restartChargeV = secondStageCalibration(restartChargeV_uncalib, ADC_coeff_product);
	restartDischV = secondStageCalibration(restartDischV_uncalib, ADC_coeff_product);
	LEDthresh1 = secondStageCalibration(LEDthresh1_uncalib, ADC_coeff_product);
	LEDthresh2 = secondStageCalibration(LEDthresh2_uncalib, ADC_coeff_product);
	LEDthreshHyst = secondStageCalibration(LEDthreshHyst_uncalib, ADC_coeff_product);
}

// This routine initialises the bare minimum needed
// to take an ADC reading of the PV voltage.
void initialisePre(void) {
	initialiseClock();
	initialiseADC();
}

// Now we've decided to stay awake, this will initialise
// the rest.
void initialiseFull(void) {
	calibrateThresholds();
	initialiseGlobals();
	initialiseIO();
	initialiseTimer();

// Initialise variables needed for temperature logging and use.
// This is located in "initialiseFull()" in initialise.cpp
#ifdef enableMaxTempLog
	flashReady = 0xFFFF;
	maxTemp_RAM = 0;
	av_tempADC = 0;
	// The flash timer clock speed must be between 257 kHz -> 476 kHz.
	// Let's set the clock source to MCLK (8MHz) and set a divider to 27.
	// We don't bother with this if we're snoozing, so no need to worry
	// about other clock speeds.
	FCTL2 = FWKEY + FSSEL_1 + (27 - 1);
	// Calculate the maximum shutdown temperature as an ADC reading (adding 0.5 at the end to ensure rounding instead of truncation)
	shutdownTemp = ( (float) shutdownTemp_uncalib - 30 ) * ( *CALADC_15T85 - *CALADC_15T30 ) / ( 85 - 30 ) + *CALADC_15T30 + 0.5;
#endif  // enableMaxTempLog

// This tests the unit to see whether it's had it's had a good test.
// Located in initialise.cpp.
#ifdef enableFirstRunTest
	// If the unit has already had a good test, the value of *testResult will be 0. Otherwise, we should run (or rerun) the
	// testing protocol, and send the LED result as an output
	if (*testResult != 0) {
		firstRunTest(); // This method will run the "first run" test, and assign a value to test result
		// Now we're guaranteed a result, so let's check it.
		// In each case we display LEDs, but with different "codes" to show failure mode.
		// The code numbers are chosen to match those used in the quality control flow charts
		switch (*testResult) {
		case 0:
			// Successful test result, display green led flash, and then go to sleep.
			// Next time we wake up the test will not be run.
			flashLED(3,0,testBlinkDuration,testBlinkDuration,testSuccessBlinkNumber);
			goToSleep();
			break;
		case 3:
			// Bad solar panel possibility, display red and green to suggest change of solar panel, then go to sleep
			// Next time we wake up the test will be rerun
			flashLED(1,3,testBlinkDuration,testBlinkDuration,testFailBlinkNumber);
			goToSleep();
			break;
		case 4:
			// Bad battery possibility, flash red to suggest change of battery, then go to sleep
			// Next time we wake up the test will be rerun
			flashLED(1,0,testBlinkDuration,testBlinkDuration,testFailBlinkNumber);
			goToSleep();
			break;
		case 5:
			// Bad PCB or solar panel, display long red flash, with short pulses off to suggest change of PCB or solar panel, then go to sleep
			// Next time we wake up the test will be rerun
			flashLED(1,0, testBlinkDuration * 2, testBlinkDuration / 2, testFailBlinkNumber);
			goToSleep();
			break;
		case 7:
			// Probably bad PCB, display long red flash, with short pulses of green to suggest change of PCB, then go to sleep
			// Next time we wake up the test will be rerun
			flashLED(1,3, testBlinkDuration * 2, testBlinkDuration / 2, testFailBlinkNumber);
			goToSleep();
			break;
		}
	}

#endif // enableFirstRunTest

}
