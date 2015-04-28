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
#include <String.h>
#include "HSoundplane.h"
#include "masterSettings.h"

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* | setup																	| */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
void setup()
{
	// setting up sync pin(s)
	syncPinState = false;
	pinMode(SYNC_PIN_1, OUTPUT);
	digitalWrite(SYNC_PIN_1, syncPinState);
	
	syncPinState = !syncPinState;
	digitalWrite(SYNC_PIN_1, syncPinState);
	syncPinState = !syncPinState;
	digitalWrite(SYNC_PIN_1, syncPinState);
	
	delay(STARTUP_WAIT_MS);
	
	// Setting up communication...
	Serial.begin(SERIAL_SPEED);
	Wire.begin(); // Start i2c
	if(i2cFastMode) Wire.setClock(400000);
	
	if(debug) {
		Serial.println("\nStarting up master controller...");
		Serial.println("*************************************");
		Serial.print("serial:\n\t- port @ "); Serial.println(SERIAL_SPEED, DEC);
		Serial.print("i2c:\n\t- port @ "); Serial.println((i2cFastMode) ? "400 kHz" : "100 kHz");
		Serial.print("slaves:\n\t- quantity: "); Serial.println(NUMBER_OF_SLAVES, DEC);
		Serial.print("\t- piezos/slave: "); Serial.println(PIEZOS_PER_SLAVE, DEC);
		Serial.print("\t -> available piezos: "); Serial.println(MAX_PIEZO_VAL, DEC);
		// Serial.print("drivers:\n\t- gain: "); Serial.println(drv2667Gain, DEC);
		Serial.println("**************************************\n");
	}

	// Initializing slaves
	slaveRegister();
	for(uint8_t i = 0; i < NUMBER_OF_SLAVES; i++) {
		syncPinState = !syncPinState;
		digitalWrite(SYNC_PIN_1, syncPinState);
		
		bool reset = true;
		bool on = true;
		uint8_t gain = 3;
		if(i2cSlaveAvailable[i]) {
			if(debug) {
				Serial.print("\nSetting up slave @ 0x"); Serial.print(i2cSlaveAddresses[i], HEX);
				Serial.println(":\n------------------------");
			}
			bool setupOk = slaveDrvSetup(i2cSwitchAddresses[i], reset, on, gain);
			syncPinState = !syncPinState;
			digitalWrite(SYNC_PIN_1, syncPinState);
			slaveInitNotify(i2cSlaveAddresses[i], setupOk);
		}
	}
}


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* | loop																	| */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
void loop()
{
	uint8_t strLength;
  
	// Waiting for characters on the serial port until '\n' (LF).
	// Then process the received command line
	if(Serial.available()) {
		char c = Serial.read();
    
		if(c == '\n') {
			syncPinState = !syncPinState;
			digitalWrite(SYNC_PIN_1, syncPinState);		// signalize command line reception
			
			strLength = parseCommand(command);
			
			syncPinState = !syncPinState;
			digitalWrite(SYNC_PIN_1, syncPinState);		// measure parsing time
			command = "";
			if(strLength != -1) {
				distributeCoordinates(strLength, HScoord, piezoMatrix);
				for(uint8_t i = 0; i < NUMBER_OF_SLAVES; i++) {
					if(i2cSlaveAvailable[i]) {
						if(piCnt[i] > 0) {
							sendToSlave(i2cSlaveAddresses[i], piezoMatrix[i], piCnt[i]);
						}
					}
					piCnt[i] = 0;
					// if(debug) {
					// 	Serial.print("piCnt["); Serial.print(i, DEC);
					// 	Serial.print("]: "); Serial.println(piCnt[i], DEC);
					// }
				}
			}
		}
		else {
			command += c;
		}
	}
}



/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* | parseCommand															| */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
uint8_t parseCommand(String com)
{
	if(debug) {
		Serial.println("\nEntering parser...");
		Serial.println("------------------");
	}

	String subPart;		// substring used for trimming
	uint8_t i, piezoPairs, coord, index;
	bool cont;			// Flag to continue slicing the command string
	bool startMarkerSet, byteOne, firstPair, lastPair;
  
	if(debug) {
		Serial.print("Received: "); Serial.print(com);
		// Serial.print(" ...length: "); Serial.println(com.length());
	}
	if(com.length() != 0) {
		cont = true;
		byteOne = true;
		com.concat(" "); // Work around to be able to terminate the parsing of the string
		i = 0;
		while(cont) {
			subPart = com.substring(0, com.indexOf(" "));
			subPart.trim();
            
			/* Byte marker logic:
			* - if B1 (col) & start marker      -> first byte
			* - if B1 (col) & NOT start marker  -> any B1 (col)
			* - if B2 (raw) & stop marker       -> last byte
			* - if B2 (raw) & NOT stop marker   -> any B2 (raw)
			*/
			coord = subPart.toInt();
			if(byteOne) {
				if(coord & START_MARKER_MASK) {
					firstPair = true;
					lastPair = false;
				} else {
					firstPair = false;
				}
			} else {
				if(coord & STOP_MARKER_MASK) {
					lastPair = true;
				} else {
					lastPair = false;
				}
			}
      
			if(byteOne & firstPair) {
				if(debug) {
					Serial.print("B1 (col) & ST\t--> "); Serial.println(coord, DEC);
				}
				index = 0;
				HScoord[index][0] = (coord & ~START_MARKER_MASK);
				byteOne = false;
			} else if(byteOne & !firstPair) {
				if(debug) {
					Serial.print("B1 (col)\t--> "); Serial.println(coord, DEC);
				}
				HScoord[index][0] = coord;
				byteOne = false;
			} else if(!byteOne & !lastPair) {
				if(debug) {
					Serial.print("B2 (raw)\t--> "); Serial.println(coord, DEC);
				}
				HScoord[index][1] = coord;
				index += 1;
				byteOne = true;
			} else if(!byteOne & lastPair) {
				if(debug) {
					Serial.print("B2 (raw) & SP\t--> "); Serial.println(coord, DEC);
				}
				HScoord[index][1] = (coord & ~STOP_MARKER_MASK);
				byteOne = true;
				piezoPairs = index + 1;
			} else {
				if(debug) {
					Serial.println("ERROR in pair matching!");
				}
				return -1;
			}
      
			com.remove(0, com.indexOf(" ") + 1);
			if(com.length() <= 0) cont = false;
		}
    
		if(debug) {
			Serial.print("\nCoordinates table ("); Serial.print(piezoPairs); Serial.println(" pairs):");
			Serial.println("x\ty\n---------");
			for(i = 0; i < piezoPairs; i++) {
				Serial.print(HScoord[i][0], DEC);
				Serial.print("\t"); Serial.println(HScoord[i][1], DEC);
			}
		}
	}

	return piezoPairs; // returns the length of the parsed command line
}
