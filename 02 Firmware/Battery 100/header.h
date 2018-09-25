/*
 * header.h
 *
 *  Created on: 12 Jan 2015
 *      Author: Edward
 */

#ifndef HEADER_FILE_H
#define HEADER_FILE_H

// Uncalibrated voltage thresholds (ADC units - conversion depends on potential divider - see "Voltage to ADC value conversions.xlsx")
#define PVmpp_uncalib			442		// ADC equivalent to 18.00V - 0.5V = 17.50V - PV maximum power point accounting for diode drop (Keep at 18V which is 0.5V higher than starter set so as to give starter set priority!)
#define lowPV_uncalib			256		// ADC equivalent to 10V PV minimum turn-on/turn-off voltage, and minimum required for bleeding (lower than Vmpp to allow for some overshoot during charging)
#define maxCellV_uncalib		136		// ADC equivalent to 3.65V - maximum cell voltage before throttling
#define minCellV_uncalib		104		// ADC equivalent to 2.70V - minimum cell voltage before low voltage discharge kicks in
#define minFuse_uncalib			186		// ADC equivalent to 5.00V - minimum voltage after fuse which causes discharge MOSFET shut
#define minBleedV_uncalib		130		// ADC equivalent to 3.50V - lowest voltage that bleeding can take a cell down to
#define stopChargeV_uncalib		130		// ADC equivalent to 3.50V - if all cells are above this voltage then charging stops
#define restartChargeV_uncalib	123		// ADC equivalent to 3.35V - after charge stops, if cell with lowest voltage drops down to here, charging will restart again. Should be able to raise this a bit if we can get ADC channels to be more stable
#define restartDischV_uncalib	113		// ADC equivalent to 3.05V - after discharge stops, if cell with lowest voltages rises up to here, discharging will restart
#define LEDthresh1_uncalib		123		// ADC equivalent to 3.35V - Threshold between Green and Yellow (LEDthresh1 + LEDthreshHyst should be <= to stopChargeV, else you'll never see green!)
#define LEDthresh2_uncalib		115		// ADC equivalent to 3.15V - Threshold between Yellow and Red
#define LEDthreshHyst_uncalib	7		// ADC equivalent to 0.15V - Hysteresis added to thresholds "on the way up"; needed to avoid flickering

// Clock initialisation (settings for clock used in both initialising, and after waking up from snooze in considerSnooze()
// Settings are described in initialise.cpp
#define DCOCTL_setting 	CALDCO_8MHZ
#define	BCSCTL1_setting	(XT2OFF + DIVA_0) | (0x0f & CALBC1_8MHZ)

// PWM
#define PWMperiod	0xFFFF	// 3000 is good (though wobbly at low-batt charging). Value for TACCR0 that defines top counter value of PWM timer, and therefore both PWM period and resolution increase with this
#define maxDuty		0x6E14  // 1300 is good. Start-up value for TACCR1 (duty cycle), and also max value it rise to (defines lowest voltage it can sink to which helps speed up initial settling)

// Timing
#define standardBlinkNumber			30		// Number of flash toggles for short-circuit timeout
#define lowBattBlinkDuration		2 		// Number of 1/8th of a second per flash toggle
#define shortCircuitBlinkDuration	1 		// Number of 1/8th of a second per flash toggle
#define maxSnoozeTime				2928814 // Number of cycles without charge before the unit should go to sleep (2928814 should be 48 hours)

// Values -> centivolts (cV) inverse coefficients for conversion
#define C_CELL	2.688172 	// (1+0.1/0.1)*250/1023	<- ( Total pot. resistance / sensed resistance) * V_ref(cV) / ADC divisions
#define C_PV	3.910068	// (33+2.2/2.2)*250/1023

// Analog pin numbers (A.x)
#define CELL1		3		// Cell 1 terminal
#define CELL2		2		// Cell 2 terminal
#define CELL3		0		// Cell 3 terminal
#define CELL4		1		// V_batt/Cell 4 terminal
#define PV			4		// PV voltage
#define DISCURRENT	7		// Voltage downstream of PTC fuse

// Declare variables that cross source-files
extern unsigned int av_ADC_values[6]; // Holds running averages of the ADC channel readings (although currently the PV and the DISCURRENT channels are not averaged, so these indices only hold the latest value)
extern unsigned int av_cell_values[4]; // Running averages of cell voltages for the purpose of more stable threshold crossings
extern bool cell_bleedingOn[4]; // Sets to true if bleeding is happening, false if it's not.
extern char dropped_bits[4]; // Value for saving the previous cycle's bits that were dropped in the bit-shifting divider for updateAverages()
extern char batteryStatus;		// 0: charging, 1: full, 2: empty, 3: short-circuited
extern char minCell;
extern char maxCell;
extern unsigned int* CALADC_25VREF_FACTOR;
extern unsigned int *CALADC_GAIN_FACTOR;
extern int *CALADC_OFFSET;
extern char LEDStatus; // 0: off, 1: red, 2: yellow, 3: green
extern char ADC_CH_numbers[6]; // Refers to the ADC channel port number, by easier numbers to remember and list: 0-CELL1, 1-CELL2, 2-CELL3, 3-CELL4, 4-PV, 5-DISCURRENT
extern unsigned long timeSinceLastCharge; // Counts the number of cycles that the unit has been awake for since last charging

// Calibrated threshold variables
extern unsigned int PVmpp;
extern unsigned int lowPV;
extern char maxCellV;
extern char minCellV;
extern char minFuse;
extern char minBleedV;
extern char stopChargeV;
extern char restartChargeV;
extern char restartDischV;
extern char LEDthresh1;
extern char LEDthresh2;
extern char LEDthreshHyst;

// Declare functions that cross source-files
void patWatchdog(void);			// initialise.cpp
void initialisePre(void); 		// initialise.cpp
void initialiseFull(void);		// initialise.cpp
void checkPV(void);				// ADCs.cpp
void refreshADCs(void);			// ADCs.cpp
void refreshBatteryStatus(void);// refreshBatteryStatus.cpp
void refreshDischarge(void);	// refreshDischarge.cpp
void refreshCharge(void);		// refreshCharge.cpp
void considerSleep(void); 		// considerSleep.cpp
void refreshLEDs(void);			// refreshLEDs.cpp
void setLEDs(char);				// refreshLEDs.cpp
void flashLED(char, char, char, char, unsigned int);			// refreshLEDs.cpp
void considerSnooze(void);		// considerSleep.cpp
// These are used in both the optional logTemp function and the optional firstRunTest function,
// so rather than have nested ifdef's, let's just declare them no matter what
unsigned int readADCChannel(char); // ADCs.cpp
void openGate(void);			// refreshDischarge.cpp
void closeGate(void);			// refreshDischarge.cpp



// Declaration of some things to help with logging and using the interal temperature sensor, placed in header.h
// Also have to remember to include or not include logTemp.cpp!
// Costs 486 bytes in code space to include (--opt_level = off, --opt_for_speed = 1)
#define enableMaxTempLog   // Comment this out to remove all the relevant code and variables throughout the project
#define shutdownTemp_uncalib		75			// Max temp before shutdown in degrees celcius
#define tempShutdownBlinkNumber		150			// Number of blinks to carry out on thermal shutdown
#define tempShutdownBlinkDuration	8			// 1/8ths of a second
extern char temp_dropped_bits;
extern unsigned int av_tempADC;
extern unsigned int maxTemp_RAM;
extern unsigned int *maxTemp_FLASH;
extern unsigned int *CALADC_15T85;
extern unsigned int *CALADC_15T30;
extern unsigned int shutdownTemp;
extern unsigned int flashReady;
void logTemp(void);				// logTemp.cpp
void goToSnooze(void);			// considerSleep.cpp
void wakeUpFromSnooze(void);	// considerSleep.cpp
void closeGate(void);			// refreshDischarge.cpp


// Declaration of some things to help with running a "first run" test, placed in header.h
// Also have to remember to include or not include firstRunTest.cpp!
//#define enableFirstRunTest				// Comment this out to remove all the relevant code and variables throughout the project
#define acquireBaselinesLoops			20000u		// Number of samples to take to get a nice baseline of the internal cell voltages
#define chargeTestLoops					40000ul	    // Normal loop does 4300Hz, so assume this for now to calculate length of prototype loop
#define testFailBlinkNumber				15			// Number of blinks to carry out after test failure - has to be short to not keep tester waiting...
#define testSuccessBlinkNumber			300			// Number of blinks to carry out after test success - has to be long to permit testing of output sockets!
#define testBlinkDuration				8			// Nice slow blinking (approx. 1s on, 1s off)
#define PVmax							524			// Nominal voltage of open-circuit PV after diode (20.5 - 0.5 = 20.0V)
#define PVtolerance						38 			// How far off can PVmax be from the ideal? (quite large to allow for higher panel temperatures)
#define testChargeMargin				1			// Battery voltage must increase by this amount during the test charge to pass the test
#define maxFuseDrop						20			// Maximum voltage drop across the fuse permissable during discharge test
extern char *testResult;
void initialiseFull(void);					// initialise.cpp
void goToSleep(void);						// considerSleep.cpp
void updateAverage(unsigned int, char);		// ADCs.cpp
void firstRunTest(void);					// firstRunTest.cpp



#endif /* HEADER_FILE_H */
