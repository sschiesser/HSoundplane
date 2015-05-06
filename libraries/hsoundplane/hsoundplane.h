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

#ifndef _HSOUNDPLANE_H
#define _HSOUNDPLANE_H

#include "drv2667.h"

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* | MACROS																	| */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
// HSoundplane characteristics
#define NUMBER_OF_SLAVES	4			// number of slaves to drive (1 - 4)
#define COLUMNS_PER_SLAVE	8			// number of columns per slave (usually 8)
#define PIEZO_RAW_9			0			// piezo raws setting: 0 -> 5 items, 1 -> 9 items
#if(PIEZO_RAW_9 > 0)					// number of piezo per slave
#define PIEZOS_PER_SLAVE	(COLUMNS_PER_SLAVE * 9)
#else
#define PIEZOS_PER_SLAVE	(COLUMNS_PER_SLAVE * 5)
#endif
#define MAX_PIEZO_VAL		(NUMBER_OF_SLAVES * PIEZOS_PER_SLAVE)	// number of available piezos
#define MAX_COORD_PAIRS		16			// maximal amount of simultaneous coordinate pairs
#define DRV_PER_SLAVE		8			// number of drv2667 per slave

// I2C communication settings
#define I2C_MASTER_ADDRESS	0x40		// i2c master address
#define I2C_SLAVE_ADDR1		0x51		// i2c slave1 address
#define I2C_SLAVE_ADDR2		0x52		// i2c slave2 address
#define I2C_SLAVE_ADDR3		0x53		// i2c slave3 address
#define I2C_SLAVE_ADDR4		0x54 		// i2c slave4 address
#define I2C_SWITCH_ADDR1	0x71		// i2c switch1 address (range: 0x70 - 0x77)
#define I2C_SWITCH_ADDR2	0x72		// i2c switch2 address
#define I2C_SWITCH_ADDR3	0x73		// i2c switch3 address
#define I2C_SWITCH_ADDR4	0x74		// i2c switch4 address
#define I2C_REGISTER_SET	0xFD
#define I2C_INIT_NOTIFY		0xFE	

#define SERIAL_CMD_MODE		0xFF
#define CMD_PIEZO_ALL_OFF	0x00
#define CMD_PIEZO_S1_OFF	0x01
#define CMD_PIEZO_S2_OFF	0x02
#define CMD_PIEZO_S3_OFF	0x03
#define CMD_PIEZO_S4_OFF	0x04
#define CMD_DRV_ALL_OFF		0x10
#define CMD_DRV_D1_OFF		0x11
#define CMD_DRV_D2_OFF		0x12
#define CMD_DRV_D3_OFF		0x13
#define CMD_DRV_D4_OFF		0x14
#define CMD_DRV_ALL_ON		0x20
#define CMD_DRV_D1_ON		0x21
#define CMD_DRV_D2_ON		0x22
#define CMD_DRV_D3_ON		0x23
#define CMD_DRV_D4_ON		0x24


// Pinout of the arduino nano on the driver board
#define LED1_PIN			3			// LED1 -> device started up
#define LED2_PIN			5			// LED2 -> drv2667 enabled
#define LED3_PIN			6			// LED3 -> SPI activity
#define XCK_PIN				4			// UART clock
// #define TXD_PIN				TXD			// UART TX
// #define RXD_PIN				RXD			// UARD RX
#define SCL_PIN				A5			// i2c SCL (not needed since automatically assigned by setup)
#define SDA_PIN				A4			// i2c SDA (not needed since automatically assigned by setup)
#define SCK_PIN				13			// SPI clock (i.e. shift registers shifting clock)
#define SS_PIN				10			// slave select (for SPI slave mode)
#define MISO_PIN			12			// MISO (for SPI master receiver/slave transmitter mode)
#define MOSI_PIN			11			// MOSI (i.e. shift register serial in)
#define OE_PIN				7			// shift registers output enable (active low)
#define LOAD_PIN			8			// shift registers load clock (active rising)
#define CLR_PIN				9			// shift registers clear (active low)
#define SW_ADDR_0			2			// i2c switch hardware address bit0
#define SW_ADDR_1			A2			// i2c switch hardware address bit1
#define SW_ADDR_2			A3			// i2c switch hardware address bit2

#define LED_ON				LOW			// macro to set if LEDs are switched on HIGH or LOW
#define LED_OFF				HIGH		// and never forget it after that


#define DEBUG_MONITOR_DELAY_MS	20		// waiting time for debug Serial.print

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* | VARIABLES																| */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
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

// extern...
extern bool debug;
extern SPISettings settingsA;
extern uint8_t HScoord[MAX_COORD_PAIRS][2];
extern uint8_t piezoMatrix[NUMBER_OF_SLAVES][MAX_COORD_PAIRS];
extern uint8_t piCnt[NUMBER_OF_SLAVES];
extern int8_t i2cSlaveAddresses[NUMBER_OF_SLAVES];
extern int8_t i2cSwitchAddresses[NUMBER_OF_SLAVES];
extern bool i2cSlaveAvailable[NUMBER_OF_SLAVES];
extern bool piezoOff[5];
extern bool driverOff[5];


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* | FUNCTIONS																| */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
// master functions
// void slaveInit(bool, bool, uint8_t);
void slaveRegister();
bool slaveDrvSetup(int8_t, bool, bool, uint8_t);
void slaveInitNotify(int8_t, bool);
void distributeCoordinates(uint8_t len, uint8_t orig[MAX_COORD_PAIRS][2], uint8_t dest[NUMBER_OF_SLAVES][MAX_COORD_PAIRS]);
void sendToSlave(uint8_t slaveNumber, uint8_t *message, uint8_t len);

// slave functions
void driverSetup(bool, bool on, uint8_t gain);
void piezoSend(uint32_t val1, uint32_t val2, uint32_t val3);


#endif
