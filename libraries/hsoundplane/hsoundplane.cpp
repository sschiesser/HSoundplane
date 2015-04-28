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


// ********************
// MASTER FUNCTIONS...
// ********************

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* | slaveRegister															| */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
void slaveRegister() {
	uint8_t receivedAddr;
	// bool reset = true;
	// bool on = true;
	// uint8_t gain = 3;
  
	if(debug) {
		Serial.println("Registering slaves...");
	}

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
			delay(DEBUG_MONITOR_DELAY_MS);
			Serial.print("Slave @ address 0x"); Serial.print(i2cSlaveAddresses[i], HEX);
			Serial.println((i2cSlaveAvailable[i]) ? " available" : " NOT available");
		}
	}
	if(debug) {
		Serial.println("");
	}
}



/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* | slaveDrvSetup															| */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
bool slaveDrvSetup(int8_t addr, bool reset, bool on, uint8_t gain) {
	int8_t retVal;
	bool initOk = true;
	
	// initialization...
	if(reset) {
		if(debug) {
			Serial.println("Resetting drv2667...");
			Serial.print("- addressing i2c switch @ 0x"); Serial.println(addr, HEX);
		}

		// open each i2c switch channel and send reset command to the attached drv2667
		for(uint8_t i = 0; i < DRV_PER_SLAVE; i++) {
			// open switch
			Wire.beginTransmission(addr);		
			Wire.write((uint8_t)(1 << i));
			retVal = Wire.endTransmission();
			initOk = ((retVal == 0) && (initOk == true)) ? true : false;		
			if(debug) {
				Serial.print("- opening switch #"); Serial.print(i, DEC);
				if(retVal == 0) {
					Serial.println("\t\t\tsuccess!");
				} else {
					Serial.println("\t\t\tERROR!");
				}
				delay(DEBUG_MONITOR_DELAY_MS);
			}
			
			// reset driver
			Wire.beginTransmission(DRV2667_I2C_ADDRESS);
			Wire.write(DRV2667_REG02);
			Wire.write(DEV_RST);
			retVal = Wire.endTransmission();
			initOk = ((retVal == 0) && (initOk == true)) ? true : false;		
			if(debug) {
				Serial.print("- resetting device");
				if(retVal == 0) {
					Serial.println("\t\t\tsuccess!");
				} else {
					Serial.println("\t\t\tERROR!");
				}
				delay(DEBUG_MONITOR_DELAY_MS);
			}
		}
		
		// close switch
		Wire.beginTransmission(addr);
		Wire.write(0);
		retVal = Wire.endTransmission();
		initOk = ((retVal == 0) && (initOk == true)) ? true : false;		
		if(debug) {
			Serial.print("- closing i2c swtich");
			if(retVal == 0) {
				Serial.println("\t\t\tsuccess!\n");
			} else {
				Serial.println("\t\t\tERROR!\n");
			}
			delay(DEBUG_MONITOR_DELAY_MS);
		}
	}
  
	// switch on...
	if(on) {
		if(debug) {
			Serial.println("Starting up drv2667...");
			Serial.print("- addressing i2c switch @ 0x"); Serial.println(addr, HEX);
		}

		// open each i2c switch channel and send settings to the attached drv2667
		for(uint8_t i = 0; i < DRV_PER_SLAVE; i++) {
			// open switch
			Wire.beginTransmission(addr);
			Wire.write((uint8_t)(1 << i));
			retVal = Wire.endTransmission();
			initOk = ((retVal == 0) && (initOk == true)) ? true : false;		
			if(debug) {
				Serial.print("- opening switch #"); Serial.print(i, DEC);
				if(retVal == 0) {
					Serial.println("\t\t\tsuccess!");					
				} else {
					Serial.println("\t\t\tERROR!");					
				}
				delay(DEBUG_MONITOR_DELAY_MS);
			}
			
			// wake up
			Wire.beginTransmission(DRV2667_I2C_ADDRESS);
			Wire.write(DRV2667_REG02);
			Wire.write(GO);
			retVal = Wire.endTransmission();
			initOk = ((retVal == 0) && (initOk == true)) ? true : false;		
			if(debug) {
				Serial.print("- waking up");
				if(retVal == 0) {
					Serial.println("\t\t\t\tsuccess!");
				} else {
					Serial.println("\t\t\t\tERROR!");
				}
				delay(DEBUG_MONITOR_DELAY_MS);
			}

			// set mux & gain
			Wire.beginTransmission(DRV2667_I2C_ADDRESS);
			Wire.write(DRV2667_REG01);
			Wire.write(INPUT_MUX | gain);
			retVal = Wire.endTransmission();
			initOk = ((retVal == 0) && (initOk == true)) ? true : false;		
			if(debug) {
				Serial.print("- setting MUX and GAIN to 0x:"); Serial.print((INPUT_MUX | gain), HEX);
				if(retVal == 0) {
					Serial.println("\tsuccess!");
				} else {
					Serial.println("\tERROR!");
				}
				delay(DEBUG_MONITOR_DELAY_MS);
			}
			
			// enable amplifier
			Wire.beginTransmission(DRV2667_I2C_ADDRESS);
			Wire.write(DRV2667_REG02);
			Wire.write(EN_OVERRIDE);
			retVal = Wire.endTransmission();
			initOk = ((retVal == 0) && (initOk == true)) ? true : false;		
			if(debug) {
				Serial.print("- enabling amplifier");
				if(retVal == 0) {
					Serial.println("\t\t\tsuccess!");
				} else {
					Serial.println("\t\t\tERROR!");
				}
				delay(DEBUG_MONITOR_DELAY_MS);
			}				
		}
		
		// close i2c switch
		Wire.beginTransmission(addr);
		Wire.write(0);
		retVal = Wire.endTransmission();
		initOk = ((retVal == 0) && (initOk == true)) ? true : false;		
		if(debug) {
			Serial.print("- closing i2c swtich");
			if(retVal == 0) {
				Serial.println("\t\t\tsuccess!");
			} else {
				Serial.println("\t\t\tERROR!");
			}
			delay(DEBUG_MONITOR_DELAY_MS);
		}
		
		
	} else { // switch off...
		if(debug) {
			Serial.println("Switching off drv2667...");
			Serial.print("- addressing i2c switch @ 0x"); Serial.println(addr, HEX);
		}
		
		// open each i2c switch channel and send standby command to the attached drv2667
		for(uint8_t i = 0; i < DRV_PER_SLAVE; i++) {
			Wire.beginTransmission(addr);
			Wire.write((uint8_t)(1 << i));
			retVal = Wire.endTransmission();
			initOk = ((retVal == 0) && (initOk == true)) ? true : false;		
			if(debug) {
				Serial.print("- opening switch #"); Serial.print(i, DEC);
				if(retVal == 0) {
					Serial.println("\t\t\tsuccess!");					
				} else {
					Serial.println("\t\t\tERROR!");					
				}
				delay(DEBUG_MONITOR_DELAY_MS);
			}
      
			Wire.beginTransmission(DRV2667_I2C_ADDRESS);
			Wire.write(DRV2667_REG02);
			Wire.write(STANDBY);
			retVal = Wire.endTransmission();
			initOk = ((retVal == 0) && (initOk == true)) ? true : false;		
			if(debug) {
				Serial.println("- standing by");
				if(retVal == 0) {
					Serial.println("\t\t\tsuccess!");					
				} else {
					Serial.println("\t\t\tERROR!");					
				}
				delay(DEBUG_MONITOR_DELAY_MS);
			}
		}
		
		// close i2c switch
		Wire.beginTransmission(addr);
		Wire.write(0);
		retVal = Wire.endTransmission();
		initOk = ((retVal == 0) && (initOk == true)) ? true : false;		
		if(debug) {
			Serial.print("- closing i2c swtich");
			if(retVal == 0) {
				Serial.println("\t\t\tsuccess!");
			} else {
				Serial.println("\t\t\tERROR!");
			}
			delay(DEBUG_MONITOR_DELAY_MS);
		}
	}
	
	return initOk;
}


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* | slaveInitNotify														| */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
void slaveInitNotify(int8_t addr, bool notification)
{
	Wire.beginTransmission(addr);
	Wire.write(I2C_INIT_NOTIFY);
	Wire.write((notification) ? 1 : 0);
	Wire.endTransmission();
	if(debug) {
		Serial.print("Sending init notification to slave 0x"); Serial.println(addr, HEX);
		Serial.print("- value: "); Serial.println((notification) ? "true" : "false");
	}
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* | distributeCoordinates													| */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
void distributeCoordinates(	uint8_t len, uint8_t orig[MAX_COORD_PAIRS][2], uint8_t dest[NUMBER_OF_SLAVES][MAX_COORD_PAIRS]) {
								
	uint8_t mod, sn, pi;		// index modulo, slave number, piezo index

	if(debug) {
		Serial.println("\nDistributing coordinates...");
	}
	
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
				Serial.print(" / piCnt: "); Serial.println(piCnt[sn]-1);
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
    Serial.print("\nWriting to 0x");
    Serial.print(sn, HEX);
    Serial.print(": ");
  }
  
  Wire.beginTransmission(sn);		// address slave @ address sn
  Wire.write(I2C_REGISTER_SET);		// command byte with register set message
  for(uint8_t i = 0; i < len; i++) {
    if(debug) {
      Serial.print(mes[i], DEC); Serial.print("(0x");
	  Serial.print(mes[i], HEX); Serial.print(")");
      if(i < (len-1)) Serial.print(" - ");
    }
    Wire.write(mes[i]);				// send all indexes associated to this slave
  }
  if(debug) {
	  Serial.println("");
  }
  Wire.endTransmission();
}



// ********************
// SLAVE FUNCTIONS...
// ********************


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
		Serial.print("       "); Serial.println((uint8_t)(val1), HEX);
	}
	
	digitalWrite(LOAD_PIN, HIGH);		// generate rising edge on LOAD pin
	digitalWrite(LED3_PIN, HIGH);		// stop SPI activity notification
}
