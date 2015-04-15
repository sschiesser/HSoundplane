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

#include <Wire.h>
#include <SPI.h>
#include "drv2667.h"
#include "HSoundplane.h"
#include "slaveSettings.h"

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* | setup																	| */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
void setup()
{
	// Starting communication
	Serial.begin(SERIAL_SPEED); // Serial...
  
	Wire.begin(I2C_SLAVE_ADDRESS); // i2c...
	if(i2cFastMode) Wire.setClock(400000);
	Wire.onRequest(requestEvent); // attach request event (slave transmitter) handler
	Wire.onReceive(receiveEvent); // attach receive event (slave receiver) handler

	SPI.begin(); // SPI...

	// Setting up pin directions
	pinMode(LED1_PIN, OUTPUT);			// LEDs
	pinMode(LED2_PIN, OUTPUT);
	pinMode(LED3_PIN, OUTPUT);
	
	pinMode(MOSI_PIN, OUTPUT);			// SPI
	pinMode(MISO_PIN, INPUT_PULLUP);
	pinMode(SCK_PIN, OUTPUT);
	pinMode(SS_PIN, INPUT_PULLUP);
  
	pinMode(OE_PIN, OUTPUT);			// shift registers controls
	pinMode(LOAD_PIN, OUTPUT);
	pinMode(CLR_PIN, OUTPUT);
  
	pinMode(SW_ADDR_0, OUTPUT);			// i2c switch address
	pinMode(SW_ADDR_1, OUTPUT);
	pinMode(SW_ADDR_2, OUTPUT);  
	
	pinMode(SYNC_PIN_1, syncPinState);
  
	// Setting up output values
	digitalWrite(OE_PIN, HIGH); 		// disable latch outputs
	digitalWrite(CLR_PIN, HIGH);		// master restet active LOW
	digitalWrite(LOAD_PIN, LOW);		// storage clock active on rising edge
	digitalWrite(OE_PIN, LOW);			// enable latch outputs
  
	digitalWrite(SW_ADDR_0, LOW);		// i2c switch addresss
	digitalWrite(SW_ADDR_1, LOW);		// ...
	digitalWrite(SW_ADDR_2, LOW);		// ...
	
	digitalWrite(SYNC_PIN_1, syncPinState);
  
  	if(debug) {
		Serial.println("\nStarting up slave controller...");
		Serial.println("*************************************");
		Serial.print("serial:\n\t- port @ "); Serial.println(SERIAL_SPEED, DEC);
		Serial.print("i2c:\n\t- port @ "); Serial.println((i2cFastMode) ? "400 kHz" : "100 kHz");
		Serial.print("\t- own address: 0x"); Serial.println(I2C_SLAVE_ADDRESS, HEX);
		Serial.print("\t- switch address: 0x"); Serial.println(I2C_SWITCH_ADDRESS, HEX);
		Serial.println("piezos:\n\t- raws: 9\n\t- columns: 8\n\t -> available piezos: 72");
		Serial.println("*************************************\n");

		for(uint8_t i = 0; i < 3; i++) {
			digitalWrite(LED1_PIN, LED_ON);
			digitalWrite(LED2_PIN, LED_OFF);
			digitalWrite(LED3_PIN, LED_OFF);
			delay(50);
			digitalWrite(LED1_PIN, LED_OFF);
			digitalWrite(LED2_PIN, LED_ON);
			delay(50);
			digitalWrite(LED2_PIN, LED_OFF);
			digitalWrite(LED3_PIN, LED_ON);
			delay(50);
		}
	}
	
	// announcing startup done...
	digitalWrite(LED1_PIN, LED_ON);
	digitalWrite(LED2_PIN, LED_OFF);
	digitalWrite(LED3_PIN, LED_OFF);
}


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* | loop																	| */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
void loop()
{
	delay(1);
}


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* | requestEvent															| */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
void requestEvent()
{
	if(debug) {
		Serial.print("i2c registration request received... sending own address 0x");
		Serial.println(I2C_SLAVE_ADDRESS, HEX);
	}
	Wire.write(I2C_SLAVE_ADDRESS);
}


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* | receiveEvent															| */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
void receiveEvent(int howmany)
{
	syncPinState = !syncPinState;
	digitalWrite(SYNC_PIN_1, syncPinState);
	
	uint32_t piezoVal1 = 0xFFFFFFFF;
	uint32_t piezoVal2 = 0xFFFFFFFF;
	uint32_t piezoVal3 = 0xFFFFFFFF;
  
	if(debug) {
		Serial.print("i2c message received... length: ");
		Serial.println(howmany, DEC);
	}
	
	// receive the first byte and check if it's an initialization request
	uint8_t received = Wire.read();

	if(received == SLAVE_INIT_COMMAND) {
		bool reset = (Wire.read() == 1) ? true : false;
		bool on = (Wire.read() == 1) ? true : false;
		uint8_t gain = Wire.read();
		if(debug) {
			Serial.print("INIT: ");
			Serial.print("reset - "); Serial.print((reset) ? "true" : "false");
			Serial.print(" / ");
			Serial.print("switch on - "); Serial.print((on) ? "true" : "false");
			Serial.print(" / ");
			Serial.print("gain - "); Serial.println(gain, DEC);
			Serial.println("");
		}
		// driverSetup(reset, on, gain);
	} else {
		// receive all sent bytes and set (bitwise AND) the piezo bismasks
		uint8_t decount = howmany;
		while(decount > 0) {
			if(debug) {
				Serial.print("Received "); Serial.print(received, DEC);
			}
			if(received < 32) {
				piezoVal1 &= piezoArray1[received];
				if(debug) {
					Serial.print("\t-> piezoVal1: 0x"); Serial.println(piezoVal1, HEX);
				}
			} else if(received < 64) {
				piezoVal2 &= piezoArray2[received-32];
				if(debug) {
					Serial.print("\t-> piezoVal2: 0x"); Serial.println(piezoVal2, HEX);
				}
			} else if(received < 72) {
				piezoVal3 &= piezoArray3[received-64];
				if(debug) {
					Serial.print("\t-> piezoVal3: 0x"); Serial.println(piezoVal3, HEX);
				}
			} else {
				if(debug) {
					Serial.println("\t!!piezo value out of range!!");
				}
			}
			
			received = Wire.read();
			decount--;
		}
		if(debug) {
			Serial.println("");
		}
		
		// send the piezo bitmasks to the shift registers
		piezoSend(piezoVal1, piezoVal2, piezoVal3);
	}
	syncPinState = !syncPinState;
	digitalWrite(SYNC_PIN_1, syncPinState);
}