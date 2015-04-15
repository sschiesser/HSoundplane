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
#include <SPI.h>
#include <Wire.h>
#include "HSoundplane.h"


// MASTER FUNCTIONS...

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* | slaveInit																| */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
void slaveInit() {
	uint8_t receivedAddr;
	bool reset = true;
	bool on = true;
	uint8_t gain = 3;
  
	if(debug) Serial.println("Initializing slaves...");

	// Registering...
	for(uint8_t i = 0; i < NUMBER_OF_SLAVES; i++) {
		Wire.requestFrom(i2cSlaveAddresses[i], 1);
    
		while(Wire.available()) {
			receivedAddr = Wire.read();
			if(receivedAddr == i2cSlaveAddresses[i]) {
				i2cSlaveAvailable[i] = true;
			} else {
				i2cSlaveAvailable[i] = false;
			}
		}
		if(debug) {
			Serial.print("Slave @ address 0x"); Serial.print(i2cSlaveAddresses[i], HEX);
			Serial.println((i2cSlaveAvailable[i]) ? " available" : " NOT available");
		}
	}
   
	/* Setting up values...
	* Bytes (3) order:
	* - reset (0/1)
	* - switch ON (0/1)
	* - drv gain (0 - 3)
	*/
	for(uint8_t i = 0; i < NUMBER_OF_SLAVES; i++) {
		if(i2cSlaveAvailable[i]) {
			if(debug) {
				Serial.print("Sending to 0x"); Serial.print(i2cSlaveAddresses[i], HEX);
				Serial.print(": 0x"); Serial.print(SLAVE_INIT_COMMAND, HEX);
				Serial.print(" - "); Serial.print(reset);
				Serial.print(" - "); Serial.print(on);
				Serial.print(" - "); Serial.println(gain, DEC);
			}
			Wire.beginTransmission(i2cSlaveAddresses[i]);
			Wire.write(SLAVE_INIT_COMMAND);
			Wire.write((uint8_t)reset);
			Wire.write((uint8_t)on);
			Wire.write(gain);
			Wire.endTransmission();
		}
	}
}


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* | distributeCoordinates													| */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
void distributeCoordinates(	uint8_t len,
							uint8_t orig[MAX_COORD_PAIRS][2],
							uint8_t dest[NUMBER_OF_SLAVES][MAX_COORD_PAIRS]) {
								
	uint8_t mod, sn, pi;		// index modulo, slave number, piezo index

	if(debug) Serial.println("Distributing coordinates...");
	
	for(uint8_t i = 0; i < len; i++) {
		// check column value of a pair and assign it to the corresponding slave
		if(orig[i][0] < COLUMNS_PER_SLAVE) {
			static uint8_t slaveAddress = i2cSlaveAddresses[0];
			mod = 0;
			sn = 0;
		} else if(orig[i][0] < (2 * COLUMNS_PER_SLAVE)) {
			static uint8_t slaveAddress = i2cSlaveAddresses[1];
			mod = COLUMNS_PER_SLAVE;
			sn = 1;
		} else if(orig[i][0] < (3 * COLUMNS_PER_SLAVE)) {
			static uint8_t slaveAddress = i2cSlaveAddresses[2];
			mod = 2 * COLUMNS_PER_SLAVE;
			sn = 2;
		} else if(orig[i][0] < (4 * COLUMNS_PER_SLAVE)) {
			static uint8_t slaveAddress = i2cSlaveAddresses[3];
			mod = 3 * COLUMNS_PER_SLAVE;
			sn = 3;
		}
		
		// work only for available devices
		if(i2cSlaveAvailable[sn]) {
			// calculate the linear position (piezo index) of the pair
			// and save it as next item of the selected slave of 'piezoMatrix'.
			// !! always multiply by 9!! Not connected piezo will be skipped.
			pi = ((orig[i][0] - mod) * 9) + (orig[i][1] * 2);
			piezoMatrix[sn][piCnt[sn]] = pi;
			piCnt[sn] += 1;	// increment the pi counter of the selected slave
		
			if(debug) {
				uint8_t pi5 = ((orig[i][0] - mod) * 5) + orig[i][1];
				Serial.print("Slave#: "); Serial.print(sn);
				Serial.print(" Piezo#: "); Serial.print(pi);
				Serial.print("("); Serial.print(pi5); Serial.println(")");
      
				Serial.print("piezoMatrix: "); Serial.print(piezoMatrix[sn][piCnt[sn]-1], DEC);
				Serial.print("... piCnt: "); Serial.println(piCnt[sn]-1);
			}
		}
	}
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* | sendToSlave															| */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
void sendToSlave(uint8_t sn, uint8_t *mes, uint8_t len) {
  if(debug) {
    Serial.print("Writing to 0x");
    Serial.print(sn, HEX);
    Serial.print(": ");
  }
  
  Wire.beginTransmission(sn);		// address slave @ address sn
  for(uint8_t i = 0; i < len; i++) {
    if(debug) {
      Serial.print(mes[i], DEC);
      if(i < (len-1)) Serial.print(" - ");
    }
    Wire.write(mes[i]);				// send all indexes associated to this slave
  }
  if(debug) Serial.println("");
  Wire.endTransmission();
}



// SLAVE FUNCTIONS...

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* | driverSetup															| */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
void driverSetup(bool startup, bool on, uint8_t gain) {
	// initialization...
	if(startup) {
		if(debug) Serial.println("Starting up DRV2667...\n- addressing i2c switch");

		// open each i2c switch channel and send reset command to the attached drv2667
		Wire.beginTransmission(I2C_SWITCH_ADDRESS);		
		for(uint8_t i = 0; i < 8; i++) {
			if(debug) Serial.print("- opening switch #"); Serial.println(i, DEC);
			Wire.write((uint8_t)(1 << i));
			Wire.endTransmission();
      
			if(debug) Serial.println("- resetting device");
			Wire.beginTransmission(DRV2667_I2C_ADDRESS);
			Wire.write(DRV2667_REG02);
			Wire.write(DEV_RST);
			Wire.endTransmission();
		}
	}
  
	// switch on...
	if(on) {
		if(debug) Serial.println("Switching on...\n- addressing i2c switch");

		// open each i2c switch channel and send settings to the attached drv2667
		Wire.beginTransmission(I2C_SWITCH_ADDRESS);
		for(uint8_t i = 0; i < 8; i++) {
			if(debug) Serial.print("- opening switch #"); Serial.println(i, DEC);
			Wire.write((uint8_t)(1 << i));
			Wire.endTransmission();
      
			if(debug) Serial.println("- waking up");
			Wire.beginTransmission(DRV2667_I2C_ADDRESS);
			Wire.write(DRV2667_REG02);
			Wire.write(GO);
      
			if(debug) Serial.print("- setting MUX and GAIN to 0x:"); Serial.println((INPUT_MUX | gain), HEX);
			Wire.beginTransmission(DRV2667_I2C_ADDRESS);
			Wire.write(DRV2667_REG01);
			Wire.write(INPUT_MUX | gain);
			Wire.endTransmission();
      
			if(debug) Serial.println("- enabling amplifier");
			Wire.beginTransmission(DRV2667_I2C_ADDRESS);
			Wire.write(DRV2667_REG02);
			Wire.write(EN_OVERRIDE);
			if(Wire.endTransmission() == 0) {
				digitalWrite(LED2_PIN, LED_ON);			// notify driver on status
				if(debug) Serial.println("SUCCESS!");
			} else {
				digitalWrite(LED2_PIN, LED_OFF);		// driver NOT on -> error
				if(debug) Serial.println("ERROR!");
			}
		}
	} else { // switching off...
		if(debug) Serial.println("Switching off...\n- addressing i2c switch");
		
		// open each i2c switch channel and send standby command to the attached drv2667
		Wire.beginTransmission(I2C_SWITCH_ADDRESS);
		for(uint8_t i = 0; i < 8; i++) {
			if(debug) Serial.print("- opening switch #"); Serial.println(i, DEC);
			Wire.write((uint8_t)(1 << i));
			Wire.endTransmission();
      
			if(debug) Serial.println("- standing by");
			Wire.beginTransmission(DRV2667_I2C_ADDRESS);
			Wire.write(DRV2667_REG02);
			Wire.write(STANDBY);
			if(Wire.endTransmission() == 0) {
				digitalWrite(LED2_PIN, LED_OFF);		// notify driver off status
				if(debug) Serial.println("SUCCESS!");
			} else {
				digitalWrite(LED2_PIN, LED_ON);			// driver NOT off -> error
				if(debug) Serial.println("ERROR!");
			}
		}
	}
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* | piezoSend																| */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
void piezoSend(uint32_t val1, uint32_t val2, uint32_t val3) {
	digitalWrite(LED3_PIN, LOW);		// notify SPI activity
	digitalWrite(LOAD_PIN, LOW);		// prepare LOAD pin
	digitalWrite(CLR_PIN, LOW);			// reset all shift registers
	digitalWrite(CLR_PIN, HIGH);		// ...
	
	SPI.beginTransaction(settingsA);	// start the SPI transaction with saved settings
	SPI.transfer((uint8_t)(val3));
	SPI.transfer((uint8_t)(val2 >> 24));
	SPI.transfer((uint8_t)(val2 >> 16));
	SPI.transfer((uint8_t)(val2 >> 8));
	SPI.transfer((uint8_t)(val2));
	SPI.transfer((uint8_t)(val1 >> 24));
	SPI.transfer((uint8_t)(val1 >> 16));
	SPI.transfer((uint8_t)(val1 >> 8));
	SPI.transfer((uint8_t)(val1));
	SPI.endTransaction();
	
	if(debug) {
		Serial.print("Sending to shift registers:\nb'");
		Serial.print((uint8_t)(val3), BIN); Serial.print(" ");
		Serial.print((uint8_t)(val2 >> 24), BIN); Serial.print(" ");
		Serial.print((uint8_t)(val2 >> 16), BIN); Serial.print(" ");
		Serial.print((uint8_t)(val2 >> 8), BIN); Serial.print(" ");
		Serial.print((uint8_t)(val2), BIN); Serial.print(" ");
		Serial.print((uint8_t)(val1 >> 24), BIN); Serial.print(" ");
		Serial.print((uint8_t)(val1 >> 16), BIN); Serial.print(" ");
		Serial.print((uint8_t)(val1 >> 8), BIN); Serial.print(" ");
		Serial.print((uint8_t)(val1), BIN); Serial.println("'");
		
		Serial.print("0x   "); Serial.print((uint8_t)val3, HEX);
		Serial.print("       "); Serial.print((uint8_t)(val2 >> 24), HEX);
		Serial.print("       "); Serial.print((uint8_t)(val2 >> 16), HEX);
		Serial.print("       "); Serial.print((uint8_t)(val2 >> 8), HEX);
		Serial.print("       "); Serial.print((uint8_t)(val2), HEX);
		Serial.print("       "); Serial.print((uint8_t)(val1 >> 24), HEX);
		Serial.print("       "); Serial.print((uint8_t)(val1 >> 16), HEX);
		Serial.print("       "); Serial.print((uint8_t)(val1 >> 8), HEX);
		Serial.print("       "); Serial.print((uint8_t)(val1), HEX);
	}
	
	digitalWrite(LOAD_PIN, HIGH);		// generate rising edge on LOAD pin
	digitalWrite(LED3_PIN, HIGH);		// stop SPI activity notification
}
