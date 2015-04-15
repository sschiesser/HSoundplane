#ifndef _MASTERSETTINGS_H
#define _MASTERSETTINGS_H

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* | MACROS																	| */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
#define I2C_FAST_MODE 1				// 0 -> standad mode (100 kHz) i2c,
									// 1 -> fast mode (400 kHz)
#define SERIAL_SPEED 115200			// serial communication speed for
									// 1. host command reception (CDC or rawHID)
									// 2. debugging
#define START_MARKER_MASK 0xE0		// bit mask to recognize serial start byte (ST)
#define STOP_MARKER_MASK 0xF0		// bit mask to recognize serial stop byte (SP)

#define STARTUP_WAIT_MS 2000		// startup waiting time to let the slaves be ready


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* | GLOBALS																| */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
bool debug = false;					// DEBUG FLAG!!

#if(I2C_FAST_MODE > 0)				// i2c speed flag
  bool i2cFastMode = true;
#else
  bool i2cFastMode = false;
#endif

String command;								// serial command to parse

uint8_t coordinates[MAX_COORD_PAIRS][2];	// coordinate pairs fetched from the
											// serial command parsing
int8_t i2cSlaveAddresses[] = {				// slave addresses array
	I2C_SLAVE_ADDR1,
	I2C_SLAVE_ADDR2,
	I2C_SLAVE_ADDR3,
	I2C_SLAVE_ADDR4};
	
bool i2cSlaveAvailable[NUMBER_OF_SLAVES];	// available slaves array
char piezoIndex[NUMBER_OF_SLAVES][MAX_COORD_PAIRS];	// for each slave, indexes of the piezos to address
int pi[NUMBER_OF_SLAVES];

/* ------------------------- *
 * Soundplane variables      *
 * ------------------------- */
// char coordinates[MAX_COORD_PAIRS][2]; // coordinate pairs table used in serial receiving
// char piezoIndex[NUMBER_OF_SLAVES][MAX_COORD_PAIRS]; // for each slave, indexes of the piezos to address
// int pi[NUMBER_OF_SLAVES];
// int piezoPerSlave = COLUMNS_PER_SLAVE * piezoMod; // # piezos for each slave module
// int maxSlaveVal = NUMBER_OF_SLAVES; // # slave modules to talk to
// int maxPiezoVal = maxSlaveVal * piezoPerSlave; // current maximum number of available piezos

// extern int piezoPerSlave;
// extern int maxSlaveVal;
// extern int maxPiezoVal;
// extern char piezoIndex[NUMBER_OF_SLAVES][MAX_COORD_PAIRS];
// extern int pi[NUMBER_OF_SLAVES];

#endif