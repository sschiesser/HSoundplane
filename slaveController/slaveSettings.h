#ifndef _SLAVESETTINGS_H
#define _SLAVESETTINGS_H


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* | MACROS																	| */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
// TO CHANGE FOR EACH SLAVE
#define I2C_SLAVE_ADDRESS 0x51	// own i2c slave address to respond to

// GENERAL SLAVE SETTINGS
#define I2C_FAST_MODE 1			// 0 -> standad mode (100 kHz) i2c,
								// 1 -> fast mode (400 kHz)
#define SERIAL_SPEED 115200		// serial communication speed for debugging
#define SPI_SPEED 2000000 		// SPI communication speed for shift registers


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* | GLOBALS																| */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
bool debug = true;				// DEBUG FLAG!!

#if(I2C_FAST_MODE > 0)			// i2c speed flag
	bool i2cFastMode = true;
#else
	bool i2cFastMode = false;
#endif

SPISettings settingsA(SPI_SPEED, MSBFIRST, SPI_MODE0);	// SPI settings

#endif