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
//  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE ////////////////////////////////////////////////////////////////////////////
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
#define I2C_FAST_MODE		0			// 0 -> standad mode (100 kHz) i2c,
										// 1 -> fast mode (400 kHz)
#define SERIAL_SPEED		230400		// serial communication speed for
										// 1. host command reception (CDC or rawHID)
										// 2. debugging

#define SCMD_START			253			// serial command start byte
#define SCMD_STOP			255			// serial command stop byte
#define SCMD_SETTINGS		100			// threshold value above wich
										// setting commands are sent
#define SCMD_POFF_ALL		110
#define SCMD_DOFF_S0		120
#define SCMD_DOFF_S1		121
#define SCMD_DOFF_S2		122
#define SCMD_DOFF_S3		123
#define SCMD_DOFF_ALL		129
#define SCMD_DON_S0			130
#define SCMD_DON_S1			131
#define SCMD_DON_S2			132
#define SCMD_DON_S3			133
#define SCMD_DON_ALL		139

#define SERR_NOERROR		0
#define SERR_MISMATCH		1
#define SERR_LENGTH			2
#define SERR_OVERFLOW		3
#define SERR_CRLF			255

#define STARTUP_WAIT_MS		1000		// startup waiting time to let the slaves be ready
#define INIT_WAIT_MS		50			// initialization waiting time to SEE slave getting ready

#define SLAVE_REG_RETRIES	5

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

// String slicedCmd[2 * HS_COORD_MAX];		// command line sliced into integers

// extern...
extern struct HSdata HSd;


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* | FUNCTIONS																| */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
void parseCommand(void);
#endif