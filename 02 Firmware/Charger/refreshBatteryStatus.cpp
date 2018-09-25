/*
 * refreshBatteryStatus.cpp
 *
 *  Created on: 7 Jan 2016
 *      Author: eddie
 */

/* This method looks at the available data and decides whether or not actions need to be taken
 * by other methods during this loop. If actions are required, we enter the relevant "dynamic mode"
 * to signal to the other methods what is required of them. If actions are not required, or if we
 * have just completed a loop on "dynamic mode", then we either remain, or enter, a "static mode"
 * (respectively).
 *
 * The refreshJouleCounter() method is a backup version of this, which only handles entering into
 * dynamic modes, and not returning/remaining in static modes. It acts only if no decision has already
 * been taken by this method, whilst we test its effectiveness vs. voltage measurement.
 *
 * DYNAMIC MODE = ACTIONS REQUIRED THIS LOOP
 * STATIC MODE = ACTIONS NOT REQUIRED THIS LOOP

 * BatteryStatus = 0  -> (Static mode) Empty battery, gate has already been closed, LED has been flashed, and joule counter has already been recalibrated
 * BatteryStatus = 1  -> (Dynamic mode) Empty battery, gate needs closing, LED needs flashing, and joule counter needs recalibrating!
 * BatteryStatus = 2  -> (Static mode) Charging battery, should display a colour relevant to amount of charge left
 * BatteryStatus = 3  -> (Static mode) Not-charging battery (i.e. at rest), should display a colour relevant to amount of charge left
 * BatteryStatus = 4  -> (Dynamic mode) Battery has fully recovered from deep discharge, gate needs opening!
 * BatteryStatus = 5  -> (Dynamic mode) Battery has reached full capacity, recalibrate joule-counter
 * BatteryStatus = 6  -> (Static mode) Battery is full, joule-counter has already recalibrated
 *
 * A tripped short-circuit fuse is handled directly in the refreshDischarge method.
 *
 * */

#include <msp430.h>
#include "header.h"

void refreshBatteryStatus(void) {

	switch (BatteryStatus) {
	case 0:		// Check if battery has recovered, and if so, upgrade level
		if (BatteryVoltage >= restartDischV)
			BatteryStatus = 4;
		break;
	case 1:		// One loop of the system has finished the tasks that needed doing, so now recede to ongoing status.
		BatteryStatus--;
		break;
	case 2:		// Charging! Check if battery is either deeply discharged, full, or stopped charging and respond appropriately
		if (BatteryVoltage < minBattV)
			BatteryStatus = 1;
		else if (Stat2) // Important to check for a full battery, before checking if stopped charging, as Stat1 will turn off once a full battery is reached!
			BatteryStatus = 5;
		else if (!Stat1)
			BatteryStatus = 3;
		break;
	case 3:		// At rest. Check if battery is either deeply discharged, or started charging.
		if (BatteryVoltage < minBattV)
			BatteryStatus = 1;
		else if (Stat1)
			BatteryStatus = 2;
		break;
	case 4:		// One loop of the system has finished the tasks that needed doing, so now recede to ongoing status.
		BatteryStatus--;
		break;
	case 5:		// One loop of the system has finished the tasks that needed doing, so now recede to ongoing status.
		BatteryStatus++;
		break;
	case 6:		// Full battery. Check if battery returns to "at rest" mode or charging mode
		if (Stat1)
			BatteryStatus = 2;
		else if (!Stat2)  // ...and Stat1 must be false as we've checked this already!
			BatteryStatus = 3;
		break;
	}
}

