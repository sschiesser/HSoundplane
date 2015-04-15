#ifndef _SLAVESETTINGS_H
#define _SLAVESETTINGS_H

// TO CHANGE FOR EACH SLAVE
#define I2C_SLAVE_ADDRESS 0x51	// own i2c slave address to respond to

// GENERAL SLAVE SETTINGS
#define I2C_FAST_MODE 1			// 0 -> standad mode (100 kHz) i2c, 1 -> fast mode (400 kHz)
#define SERIAL_SPEED 115200		// serial communication speed for debugging
#define SPI_SPEED 2000000 		// SPI communication speed


bool debug = true;

bool drv2667Reset, drv2667SwitchOn;
uint8_t drv2667Gain;
uint8_t initCommand = SLAVE_INIT_COMMAND;

SPISettings settingsA(SPI_SPEED, MSBFIRST, SPI_MODE1);

// Set i2c speed flag
#if(I2C_FAST_MODE > 0)
	bool i2cFastMode = true;
#else
	bool i2cFastMode = false;
#endif



#endif