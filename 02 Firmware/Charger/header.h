/*
 * header.h
 *
 *  Created on: 7 Jan 2016
 *      Author: eddie
 */

#ifndef HEADER_H_
#define HEADER_H_

// ADC pin definitions
#define CURRENTV_PIN		5
#define REF1V_PIN			4
#define BATTV_PIN			3

// (To be removed) Values for converting ADC current value
// to actual amps value (interesting for debug)
#define SENSE_RESISTANCE	0.001
#define ADC_VREF			2.5
#define OPA_GAIN			200

// Voltage thresholds (converted to ADC)
#define minBattV_uncalib			409
#define restartDischV_uncalib		454

// Calibrated voltage thresholds
extern unsigned int minBattV;
extern unsigned int restartDischV;

// Timing
#define FLIPFLOP_DELAY		200		// TIME Number of __delay_cycles() needed for set/reset pin to pull relevant flip-flop signal down to earth. Good to keep this as short as possible in case you're opening up into a short circuit!
#define SHORT_FLASH_TIME	8		// Number of 1/8 seconds per flash during a short
#define SHORT_FLASH_NUMBER	10		// Number of flashes to give


// Clock initialisation (settings for clock used in both initialising, and after waking up from snooze in considerSnooze()
// Settings are described in initialise.cpp
#define DCOCTL_setting 	CALDCO_1MHZ
#define	BCSCTL1_setting	(XT2OFF + DIVA_0) | (0x0f & CALBC1_1MHZ)

// Prototypes for functions that cross source files
void tripFuse(void); 									// refreshDischarge.cpp
void resetFuse(void);									// refreshDischarge.cpp
void initialise(void);									// initialise.cpp
void patWatchdog(void);									// initialise.cpp
void getData(void);										// getData.cpp
void refreshCharge(void);								// refreshCharge.cpp
void refreshDischarge(void);							// refreshDischarge.cpp
void refreshLEDs(void);									// refreshLEDs.cpp
void flashLED(char, char, char, char, unsigned int);	// refreshLEDs.cpp
void refreshBatteryStatus(void);						// refreshBatteryStatus.cpp
void refreshJouleCounter(void);							// refreshJouleCounter.cpp

void initialFuseTrip(void);

// Prototypes for global variables that cross source files
extern unsigned int ChargingCurrent;
extern unsigned int BatteryVoltage;
extern bool Stat1;
extern bool Stat2;
extern char BatteryStatus;
extern unsigned int loop_counter;
extern unsigned int *CAL_ADC_25VREF_FACTOR;
extern unsigned int *CAL_ADC_GAIN_FACTOR;
extern int *CAL_ADC_OFFSET;

#endif /* HEADER_H_ */
