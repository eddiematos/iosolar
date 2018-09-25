#include <msp430.h>
#include "header.h"

// Use average cell voltages to avoid accidental triggering!
void refreshBatteryStatus(void) {
	// Check if battery has run out, i.e. lowest cell is
	// lower than minCellV
	if ( av_cell_values[minCell] <= minCellV )
		batteryStatus = 2;  // empty battery!
	// If - after battery has run out - charging or rest brings
	// up the minimum cell voltage again then allow discharging to return
	if ( (batteryStatus == 2) && (av_cell_values[minCell] >= restartDischV) )
		batteryStatus = 0;  // back to normal!
	// Check if battery is full, i.e. if all cells have voltages higher than stopChargeV
	if ( (av_cell_values[0] >= stopChargeV) && (av_cell_values[1] >= stopChargeV) && (av_cell_values[2] >= stopChargeV) && (av_cell_values[3] >= stopChargeV) )
		batteryStatus = 1;  // battery is full!
	// If, after a period of rest and/or discharge the min cell voltage drops beneath
	// the restart charging threshold then restart charging.
	if ( (batteryStatus == 1) && (av_cell_values[minCell] < restartChargeV) )
		batteryStatus = 0; // back to normal!
	// Check for short-circuit/over-current condition.
	if ( av_ADC_values[5] < minFuse)
		batteryStatus = 3;  // short-circuited!
}



