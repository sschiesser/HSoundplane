////////////////////////////////////////////////////////////////////////////
//
//  This file is part of HSoundplane library
//
//	Works with the following hardware (150415):
//		- Soundplane piezo-driver v0.95 - R003
//		- Soundplane piezo-layer v.095 - R006
//
//  Copyright (c) 2015, www.icst.net
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy of 
//  this software and associated documentation files (the "Software"), to deal in 
//  the Software without restriction, including without limitation the rights to use, 
//  copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the 
//  Software, and to permit persons to whom the Software is furnished to do so, 
//  subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in all 
//  copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
//  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
//  PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
//  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION 
//  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
//  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef _MASTERSETTINGS_H
#define _MASTERSETTINGS_H

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* | MACROS																	| */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
#define I2C_FAST_MODE		1			// 0 -> standad mode (100 kHz) i2c,
										// 1 -> fast mode (400 kHz)
#define SERIAL_SPEED		230400		// serial communication speed for
										// 1. host command reception (CDC or rawHID)
										// 2. debugging
#define START_MARKER_MASK	0xE0		// bit mask to recognize serial start byte (ST)
#define STOP_MARKER_MASK	0xF0		// bit mask to recognize serial stop byte (SP)

#define STARTUP_WAIT_MS		50			// startup waiting time to let the slaves be ready
#define INIT_WAIT_MS		500			// initialization waiting time to SEE slave getting ready

#define SYNC_PIN_1			2			// pin used to measure time between events


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* | VARIABLES																| */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
bool debug = true;						// DEBUG FLAG!!

#if(I2C_FAST_MODE > 0)					// i2c speed flag
  bool i2cFastMode = true;
#else
  bool i2cFastMode = false;
#endif
  
bool syncPinState;

bool allPiezosOff;
bool allDriversOff;

String command;							// serial command to parse
String slicedCmd[2 * MAX_COORD_PAIRS];	// command line sliced into integers

uint8_t HScoord[MAX_COORD_PAIRS][2];	// HSoundplane coordinate pairs fetched
										// from the serial command parsing

uint8_t piezoMatrix[NUMBER_OF_SLAVES][MAX_COORD_PAIRS];	// for each slave,
														// indexes of the piezos to address

uint8_t piCnt[NUMBER_OF_SLAVES];		// for each slave, piezo index counter

int8_t i2cSlaveAddresses[] = {			// slave addresses array
	I2C_SLAVE_ADDR1,
	I2C_SLAVE_ADDR2,
	I2C_SLAVE_ADDR3,
	I2C_SLAVE_ADDR4
};

int8_t i2cSwitchAddresses[] = {			// i2c switch addresses array
	I2C_SWITCH_ADDR1,
	I2C_SWITCH_ADDR2,
	I2C_SWITCH_ADDR3,
	I2C_SWITCH_ADDR4
};

bool i2cSlaveAvailable[NUMBER_OF_SLAVES];	// available slaves array


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* | FUNCTIONS																| */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
uint8_t parseCommand(String);
#endif