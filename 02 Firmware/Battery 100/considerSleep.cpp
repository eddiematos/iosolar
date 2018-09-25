/* This function sets up the MCU to go into low
 * power consumption mode (including closing discharge
 * MOSFET, sleeps, wakes up every
 * so often to check the panel voltage, and if panel
 * voltage is high it sets the MCU back up to exit
 * low power consumption mode, getting it ready
 * for active mode. I.e. the MCU enters this function
 * in active mode, and leaves this function in active
 *  mode.
*/

#include <msp430.h>
#include "header.h"

void goToSleep(void) {
	// Do all the things necessary before going to sleep to minimise power
	// consumption during this period.
	// P1OUT and P2OUT will not be initialised at reset, so should set them
	// to their default states now, which will also switch off any LEDs
	// (Below copied from initialise.cpp)
	// Set those pins that require initially HIGH output states - the PWM pin (P2.6) probably doesn't need to be set here, but it's set anyway just in case
	P1OUT = 0;
	P2OUT = BIT1 + BIT6;
	// Disable PWM output
	P2SEL = 0;
	// Shutdown Timer_A
	TACTL = MC_0;
	// Shutdown ADC (in case it's not done automatically)
	ADC10CTL0 = 0;
	// Discharge MOSFET is already shut (it's a condition of being here!)
	// so don't need to worry about that.
	// Pat watchdog
	patWatchdog();
	// Enter Low Power Mode 3 (turns off CPU and stops code
	// here until Watchdog timer kicks in and reboots the MCU
	LPM3;
}

// This method slows down the MCU to conserve energy when not charging.
// Also used in some flashing loops.
void goToSnooze(void) {
	// Disable PWM output pin (should then become a digital output, set high
	// which will disable charging)
	P2SEL = 0;
	// Shutdown Timer_A
	TACTL = MC_0;
	// Set PWM pin to high-impedance input, to prevent current drain.
	P2DIR &= ~BIT6;
	// Slow DCO, and ACLK down to min speed
	DCOCTL = 0;
	BCSCTL1 = (BCSCTL1 & ~0x0f);  // ~0x0f is a mask to clear the last 4 bits, those corresponding to RSEL
	// Put ADC into low power mode (see initialise file for further info)
	ADC10CTL0 |= ADC10SHT_3 + ADC10SR;
	ADC10CTL1 |= ADC10DIV_7;
	// Initialise counter for keeping track of how long the unit has been snoozing
	timeSinceLastCharge = maxSnoozeTime;
}

// This method speeds up the MCU when charging, to ensure max loop stability.
// Also used in some flashing loops.
void wakeUpFromSnooze(void) {
	// Set PWM port back to a digital output (set high) to immediately
	// disable charging.
	P2DIR |= BIT6;
	// Speed up DCO, and ACLK up to initialised speed
	DCOCTL = DCOCTL_setting;
	BCSCTL1 = BCSCTL1_setting;
	// Turn on Timer_A (same settings as in initialisation)
	TACTL = TASSEL_2 + MC_1;
	// Enable PWM output
	P2SEL = BIT6;
	// Put ADC into fast mode
	ADC10CTL0 &= (~ADC10SHT_3 & ~ADC10SR);
	ADC10CTL1 &= ~ADC10DIV_7;
}

void considerSnooze(void) {
	// If not snoozing, and there's no PV voltage, then:
	// - Switch MCLK source to very low frequency oscillator (VLO)
	// - Turn off digital oscillator (DCO)
	// - disable PWM
	// - put ADC into low power mode
	// Check PWM output pin (Port 2.6) function byte first to
	// see if we're already snoozing and then this
	// can be skipped
	if ( (P2SEL == BIT6) && (av_ADC_values[4] < lowPV) ) {
		goToSnooze();
	}
	// If snoozing, count a cycle since last charge, and check to see
	// if we've been snoozing for too long.
	// If, whilst snoozing, PV voltage appears, then speed up clock and
	// re-enable PWM. Check PWM output pin function byte to see
	// if we're already awake and then this can be skipped.
	if (P2SEL == 0) {
		timeSinceLastCharge--;
		if (timeSinceLastCharge == 0)
			goToSleep();
		if (av_ADC_values[4] >= lowPV)
			wakeUpFromSnooze();
	}
}

void considerSleep(void) {
	// Go to sleep if:
	// - No solar power AND Discharge is currently off due to the battery being empty
	if ( (av_ADC_values[4] < lowPV) && (batteryStatus == 2) )
		goToSleep();
}
