#ifndef _HSOUNDPLANE_H
#define _HSOUNDPLANE_H

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "drv2667.h"

#define NUMBER_OF_SLAVES	1			// number of slaves to drive (1 - 4)
#define COLUMNS_PER_SLAVE	8			// number of piezo columns per slave (usually 8)
#define PIEZO_RAW_9			0			// piezo raws setting: 0 -> 5 items, 1 -> 9 items
#if(PIEZO_RAW_9 > 0)
#define PIEZOS_PER_SLAVE	(COLUMNS_PER_SLAVE * 9)
#else
#define PIEZOS_PER_SLAVE	(COLUMNS_PER_SLAVE * 5)
#endif
#define MAX_PIEZO_VAL		(NUMBER_OF_SLAVES * PIEZOS_PER_SLAVE)

#define MAX_COORD_PAIRS		16			// maximal amount of simultaneous coordinate pairs

#define I2C_SWITCH_ADDRESS	0x70		// i2c switch address (range: 0x70 - 0x77)
#define SLAVE_INIT_COMMAND	0xFF		// first i2c message byte to announce init sequence

#define I2C_SLAVE_ADDR1		0x51		// i2c slave1 address
#define I2C_SLAVE_ADDR2		0x52		// i2c slave2 address
#define I2C_SLAVE_ADDR3		0x53		// i2c slave3 address
#define I2C_SLAVE_ADDR4		0x54 		// i2c slave4 address

#define LED_ON				LOW			// macro to set if LEDs are switched on HIGH or LOW
#define LED_OFF				HIGH		// and never forget it after that

// Pinout of the arduino nano on the piezo driver board v0.95 - R003
#define LED1_PIN			3			// LED1 -> device started up
#define LED2_PIN			5			// LED2 -> drv2667 enabled
#define LED3_PIN			6			// LED3 -> SPI activity
#define SCK_PIN				13			// SPI clock (i.e. shift registers shifting clock)
#define SS_PIN				10			// slave select (for SPI slave mode)
#define MISO_PIN			12			// MISO (for SPI master receiver/slave transmitter mode)
#define MOSI_PIN			11			// MOSI (i.e. shift register serial in)
#define OE_PIN				7			// shift registers output enable (active low)
#define LOAD_PIN			8			// shift registers load clock (active rising)
#define CLR_PIN				9			// shift registers clear (active low)
#define SW_ADDR_0			2			// i2c switch hardware address bit0
#define SW_ADDR_1			A6			// i2c switch hardware address bit1
#define SW_ADDR_2			A7			// i2c switch hardware address bit2

extern bool debug;
extern SPISettings settingsA;
extern uint8_t coordinates[MAX_COORD_PAIRS][2];

extern char piezoIndex[NUMBER_OF_SLAVES][MAX_COORD_PAIRS];	// for each slave, indexes of the piezos to address
extern int pi[NUMBER_OF_SLAVES];
extern int8_t i2cSlaveAddresses[4];
extern bool i2cSlaveAvailable[NUMBER_OF_SLAVES];

// Array of eight piezo columns (8 x 9) single addressed. To combine: bitwise AND (&)
// NOTE: works also with 5 piezos connected per raw (master jumps over empty contacts)
const uint32_t piezoArray1[32] = {
	0xFFFFFFFE, 0xFFFFFFFD, 0xFFFFFFFB, 0xFFFFFFF7, 0xFFFFFFEF, 0xFFFFFFDF, 0xFFFFFFBF, 0xFFFFFF7F, 0xFFFFFEFF,	// row 1
	0XFFFFFDFF, 0XFFFFFBFF, 0XFFFFF7FF, 0XFFFFEFFF, 0XFFFFDFFF, 0XFFFFBFFF, 0XFFFF7FFF, 0XFFFEFFFF, 0XFFFDFFFF,	// row 2
	0XFFFBFFFF, 0XFFF7FFFF, 0XFFEFFFFF, 0XFFDFFFFF, 0XFFBFFFFF, 0XFF7FFFFF, 0XFEFFFFFF, 0XFDFFFFFF, 0XFBFFFFFF,	// row 3
	0XF7FFFFFF, 0xEFFFFFFF, 0xDFFFFFFF, 0xBFFFFFFF, 0x7FFFFFFF};												// row 4 (5/9)
const uint32_t piezoArray2[32] = {
	0xFFFFFFFE, 0xFFFFFFFD, 0xFFFFFFFB, 0xFFFFFFF7,																// raw 4 (4/9)
	0xFFFFFFEF, 0xFFFFFFDF, 0xFFFFFFBF, 0xFFFFFF7F, 0xFFFFFEFF, 0XFFFFFDFF, 0XFFFFFBFF, 0XFFFFF7FF, 0XFFFFEFFF,	// raw 5
	0XFFFFDFFF, 0XFFFFBFFF, 0XFFFF7FFF, 0XFFFEFFFF, 0XFFFDFFFF, 0XFFFBFFFF, 0XFFF7FFFF, 0XFFEFFFFF, 0XFFDFFFFF, // raw 6
	0XFFBFFFFF, 0XFF7FFFFF, 0XFEFFFFFF, 0XFDFFFFFF, 0XFBFFFFFF, 0XF7FFFFFF, 0xEFFFFFFF, 0xDFFFFFFF, 0xBFFFFFFF, // raw 7
	0x7FFFFFFF};																								// raw 8 (1/9)
const uint32_t piezoArray3[8] = {
	0xFFFFFFFE, 0xFFFFFFFD, 0xFFFFFFFB, 0xFFFFFFF7, 0xFFFFFFEF, 0xFFFFFFDF, 0xFFFFFFBF, 0xFFFFFF7F};			// row 8 (8/9)


void driverSetup(bool startup, bool on, uint8_t gain);
uint8_t piezoSend(uint32_t val1, uint32_t val2, uint32_t val3);
void slaveInit();
void distributeCoordinates(uint8_t len);
void sendToSlave(int slaveNumber, char *message, int len);

#endif
