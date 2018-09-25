/*
 * Source code for io project
 * Product: Battery 100 Block
 * Rev: V0.1
 * Author: Edward Matos
 * Date: 23/01/2015
 *
 * Notes:
 *V1.0  - Testing build, and first attempt at implementing basic functionality.
 * 		- Voltage nudging only works at 8MHz, not at 1MHz. Should see if there's anything we can do to improve that.
 * 		- Need to learn how to wake up from LPM3 through interrupts, instead of relying on Watchdog timer. This will open up opportunities to drastically reduce power consumption whilst in "snooze mode".
 * 		- Might be nice to try and do PWM on the LED lights so as to have variable brightness (high during charging - i.e. during day, low during discharging - i.e. during night). Not looking likely given the code size limitations.
 * 		- Should continue to fiddle with voltage thresholds to maximise battery space without damaging battery - but should probably get ADC reading nailed down first.
 * 		- Implement some basic "first run" testing? Unlikely given code size limitations.
 * 		- Learn how to optimise long multiplications and float multiplications to try and reduce code size. Find out what else is keeping code size high.
 * 		- Move all voltage comparisons into ADC domain, intead of voltage domain. I.e. convert all voltage thresholds to 0->1023 numbers intead of vice versa to minimise multiplication conversions.
 *V1.01 - Some minor changes of voltage thresholds
 *		- Noticed that for-delay loops were being "optimised" out by the compiler and so replaced them with intrinsic commands to delay for a known number of cycles
 *V1.02 - Increased the minimum input voltage threshold so as to improve interaction with Starter block. (I.e. Starter block should be able to take priority)
 *		- Descreased lowPV, the point at which we go into snooze mode, so that it only enters snooze mode when the panel is well and truly in the shade. This avoids instability at low power regions.
 *		- Increased LED thresholds by 0.1V, because it was never turning orange/red otherwise.
 *V1.021- Decreased LED hysterisis amount, cause otherwise the higher LED threshold means we never see green!
 *		- Decreased restart voltage. To extend the period by which a retailer can wake up a unit after a long sleep on the shelf. Also added initial value to av_cell_voltages, to speed up initial settling on wake-up.
 *V1.03 - Translated all thresholds to ADC units, instead of voltages (replaced ADC_voltages[]) to eliminate extra step of float conversion to voltages.
 *		- Found out that "remainder" system for rolling averages was no good (see "Rolling average problem.xlsx"), so implemented a different rolling average based on saving past cycle's dropped bits.
 *V1.04	- Pre-calibrated of all the voltage thresholds at start-up to account for ADC coefficients in information memory, therefore now we don't have calibrate each value at each cycle.
 *		- Increased PWM period to increase resolution of nudging voltage changes. Smaller steps have made it more stable at low-batt charging. This was only possible since speeding up the cycles.
 *		- After stop charging, battery voltage was dropping below Green threshold and stopChargeV - presumably because voltage measurements are now better. So decreased these thresholds slightly, and increased the hysterises value accordingly.
 *V1.05 - Implemented optional logging of the internal temperature sensor to get a "max-temp" figure and save it to a location in Flash, and shut-down unit if temperature gets too high.
 *V1.06 - Implemented optional first run self-diagnosis test, for assembly partners to verify that everything works quickly and easily, and then put itself to sleep again automatically
 *		- Make LED flashing function more generic, so it can be used in other functions
 *		- Made ADC reading, averaging and cell value calculating more generic so it can be used in other functions. Also, in the new way, the time between each channel read is maximised to decrease the chances of us going above the 200ksps limit.
 *		- Implemented a new array to save ADC channel numbers separately. Almost pays for itself because it eliminated those two empty unsigned int slots in the data array, and makes things much easier.
 *		- Did some speed testing, using a cycle counter, opt_level = 2 is needed so that the self-diagnosis fits on the 4kB chip. This actually speeds up the cycle count!
 *V1.07	- Fixed a bug in the flashLED method which was not calibrating the clock timing for the flashes correctly
 *		- Debugged and tested first run test routines
 *V1.08	- Doubled the number of cycles for maxSnoozeTime to increase time before sleeping to 48 hours
 *		- Increased PVmax tolerance for solar panel voltage testing in firstRunTest() to account for panel temperature range causing wide open-circuit voltage variations
 *V2.00 - Final release for firmware in batch of Autumn 2015
 *		- Replaced pointers to calibration data in sector A with newly named definitions in msp430g2332.h
 *		- TODO: check out mitigation of integer truncating after rounding in calibration of thresholds - shouldn't the 0.5 be added before the threshold gets truncated by the "int" cast?
 *		- TODO: try decreasing capacitance of Vnudge capacitor? If it results in more stability, then maybe we can turn down the MCU speed a notch? If we can, remember that changing the MCU speeds wil wreak havoc with all the timing, so will need to be careful here.
 *		- TODO: change Vref down to 1.5V for some of the ADC channels, to improve resolution
 *		- TODO: can we use a hardware multiplier?
 *		- TODO: check out strange behaviour on initiasation of nudge voltage pin: when it is set as an output with HIGH voltage, it doesn't go high, allowing PV current to rush in. Only a problem during debugging really... but worth understanding what's going on. Could be that P2SEL should be set only after configuring the timer?
 *		- TODO: digital ports are only initialised if they should be high, if they should be unitialised they are not touched. But the user guide says that they will not be automatically initialised to LOW. Rather, they retain their previous value. So need to double check if failing to reset a HIGH to LOW at the start could cause problems.
 */


#include <msp430.h>
#include "header.h"

// Define global values and flags, declared in header for cross-source file usage
// define here, and (if needed) initialised in defineGlobals(), run in initialiseFull();
// Global variables are initialised to zero by compiler
unsigned int av_ADC_values[6];
unsigned int av_cell_values[4];
bool cell_bleedingOn[4];
char dropped_bits[4];
char batteryStatus;
char minCell;
char maxCell;
char LEDStatus;
unsigned long timeSinceLastCharge;
// Define calibrated threshold variables
unsigned int PVmpp;
unsigned int lowPV;
char maxCellV;
char minCellV;
char minFuse;
char minBleedV;
char stopChargeV;
char restartChargeV;
char restartDischV;
char LEDthresh1;
char LEDthresh2;
char LEDthreshHyst;
char ADC_CH_numbers[6] = {CELL1, CELL2, CELL3, CELL4, PV, DISCURRENT};
char led_code = 1;

// Pointer definition for loading ADC calibration data from flash memory
// (for some reason these are not pre-defined in the header file)
unsigned int *CALADC_25VREF_FACTOR = (unsigned int *) 0x10E6;
unsigned int *CALADC_GAIN_FACTOR = (unsigned int *) 0x10DC;
int *CALADC_OFFSET = (int *) 0x10DE;

// Some global variables to help with getting internal temperature, placed in global space of main.cpp
#ifdef enableMaxTempLog
	char temp_dropped_bits;
	unsigned int av_tempADC;
	unsigned int maxTemp_RAM;
	unsigned int flashReady;
	unsigned int shutdownTemp;
	unsigned int *maxTemp_FLASH = (unsigned int *) 0x1040;
	unsigned int *CALADC_15T85 = (unsigned int *) 0x10E4;
	unsigned int *CALADC_15T30 = (unsigned int *) 0x10E2;
#endif //enableMaxTempLog

// Some global variables to help with the first run test, placed in global space of main.cpp
#ifdef enableFirstRunTest
	char *testResult = (char *) 0x1080;
#endif //enableFirstRunTest

int main(void) {
	// Just woken up, chances are by the watchdog timer after
	// having been sent to sleep for a few seconds.
	// First things first - reset the watchdog timer and slow
	// it down.
	patWatchdog();
    // Initialise the bare minimum we need to be able check PV voltage,
	// i.e. ADCs and clock.
    initialisePre();
    // Check PV voltage real quick, if low, go back to sleep
	// (which will eventually reboot MCU). Otherwise lets wake
    // up and go to main loop
    checkPV();   // If this is disabled for debugging, note that average cell voltages will not get enough time to come up, and unit will be put to sleep by considerSleep() immediately.
    // We've reached here, so must be ready to wake up. Let's initialise
    // the remaining things we need, and also check for firstBoot.
    initialiseFull();

    // Begin main loop
    for (;;) {
    	// "Pat" the watchdog: let it know we're not asleep so
    	// it won't reset the MCU.
    	patWatchdog();
    	// Refresh all voltage inputs: cell voltages, PV voltage,
    	// and fuse (discharge current) voltage
        refreshADCs();
        // Analyse the battery voltages to determine what "state"
        // the battery is in
        refreshBatteryStatus();
        // Enable/disable discharge based on cell voltages and
        // fuse status. If cell voltage is too low here then
        // discharge will be switched off. Also checks fuse voltage
        // for a short circuit. If there's a short circuit, program
        // will stop charging and freeze here (flashing lights)
        // until it disappears.
        refreshDischarge();
        // Refresh charging parameters based on PV and battery
        // voltages (assuming PV voltage present). Also handles
        // cell balancing. Stops charging if cell voltages are
        // too high, and restarts charging if they fall low again.
        refreshCharge();
        // Refresh indicator LED colours to give user a feel for battery charge
        // remaining.
        refreshLEDs();
        // If discharge is disabled (due to low cell voltage, not due to
        // short circuit) AND there's
        // no PV voltage, then unit will go to sleep, to be eventually
        // woken up by the Watchdog timer and reset.
        considerSleep();
        // If there is no PV voltage, but there's still battery left, let's
        // slow things down to save battery. If PV voltage returns, then we
        // should make sure to speed things up again to ensure stable charging.
        considerSnooze();
    }
    return 0;
}


