/*
 * refreshCharge.cpp
 *
 *  Created on: 7 Jan 2016
 *      Author: eddie
 */
#include <msp430.h>
#include "header.h"

void chargeEnable(void) {
	P2OUT &= ~BIT3;
}

void chargeDisable(void) {
	P2OUT |= BIT3;
}


void refreshCharge() {
	// No reason yet to disable/enable charge, as
	// this is all handled by the chip itself.
	// Unless, maybe if the insides get too hot?
	// Or if
}



