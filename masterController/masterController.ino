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
	
	for(uint8_t i = 0; i < HS_SLAVE_NUMBER; i++) {
		HSd.piezoOff[i] = false;
	}
	HSd.piezoOffAll = false;
	HSd.drvOffAll = false;

	// Setting up communication...
	Serial.begin(SERIAL_SPEED);
	Wire.begin(); // Start i2c
	if(i2cFastMode) Wire.setClock(400000);
	
	if(debug) {
		Serial.println("\nStarting up master controller...");
		Serial.println("*************************************");
		Serial.print("serial:\n\t- port @ "); Serial.println(SERIAL_SPEED, DEC);
		Serial.print("i2c:\n\t- port @ "); Serial.println((i2cFastMode) ? "400 kHz" : "100 kHz");
		Serial.print("slaves:\n\t- quantity: "); Serial.println(HS_SLAVE_NUMBER, DEC);
		Serial.println("**************************************\n");
	}

	// Initializing slaves
	slaveRegister();
	for(uint8_t i = 0; i < HS_SLAVE_NUMBER; i++) {
		delay(INIT_WAIT_MS);
		
		bool reset = true;
		bool on = true;
		uint8_t gain = 3;
		if(HSd.i2cSlaveAvailable[i]) {
			if(debug) {
				Serial.print("\nSetting up slave @ 0x"); Serial.print(HSd.i2cSlaveAddress[i], HEX);
				Serial.println(":\n------------------------");
			}
			bool setupOk = slaveDrvSetup(HSd.i2cSwitchAddress[i], reset, on, gain);
			syncPinState = !syncPinState;
			digitalWrite(SYNC_PIN_1, syncPinState);
			slaveInitNotify(HSd.i2cSlaveAddress[i], setupOk);
			sendToSlave(HSd.i2cSlaveAddress[i], NULL, 0);
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
  
	// Collecting characters on the serial port started by '[' and terminated by ']'.
	// Then process the received command line
	if(Serial.available()) {
		char c = (char)Serial.read();
    
		if(c == '[') {
			command = "";
		} else if(c == ']') {
			syncPinState = !syncPinState;
			digitalWrite(SYNC_PIN_1, syncPinState);		// signalize command line reception

			// command.trim();
			strLength = command.length();
			if(debug) {
				Serial.print("Command: "); Serial.println(command);
				Serial.print("Length: "); Serial.println(strLength);
			}
			strLength = sliceCommand(command);

			syncPinState = !syncPinState;
			digitalWrite(SYNC_PIN_1, syncPinState);		// measuring slicing time

			uint8_t HSpairs = strLength / 2;
			for(uint8_t i = 0; i < HSpairs; i++) {
				HSd.HScoord[i][0] = convertStrToInt(slicedCmd[2*i]);
				HSd.HScoord[i][1] = convertStrToInt(slicedCmd[(2*i)+1]);
				if(debug) {
					Serial.print("HScoord: "); Serial.print(HSd.HScoord[i][0]);
					Serial.print(" "); Serial.println(HSd.HScoord[i][1]);
				}
			}
			
			syncPinState = !syncPinState;
			digitalWrite(SYNC_PIN_1, syncPinState);		// measure converting time

			distributeCoordinates(HSpairs, HSd.HScoord, HSd.HSpiezo);

			syncPinState = !syncPinState;
			digitalWrite(SYNC_PIN_1, syncPinState);		// measure distributing time

			for(uint8_t i = 0; i < HS_SLAVE_NUMBER; i++) {
				if(HSd.i2cSlaveAvailable[i]) {
					if(HSd.piezoOffAll) {
						if(debug) {
							Serial.println("Sending all piezo off command to slave");
						}
						sendToSlave(HSd.i2cSlaveAddress[i], NULL, 0);
					} else if(HSd.piezoOff[i]) {
						if(debug) {
							Serial.println("Sending piezo off command to slave #"); Serial.print(i, DEC);
						}
						sendToSlave(HSd.i2cSlaveAddress[i], NULL, 0);
					} else if(HSd.piCnt[i] > 0) {
						sendToSlave(HSd.i2cSlaveAddress[i], HSd.HSpiezo[i], HSd.piCnt[i]);
					}
				}
				HSd.piezoOff[i] = false;
				HSd.piCnt[i] = 0;
			}
			HSd.piezoOffAll = false;

			syncPinState = !syncPinState;
			digitalWrite(SYNC_PIN_1, syncPinState);		// measure sending time

		} else {
			command += c;
		}
	}
}


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* | sliceCommand															| */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
uint8_t sliceCommand(String s)
{
	if(debug) {
		Serial.println("\nSlicing command line into ints...");
		Serial.println("---------------------------------");
	}
	
	String temp = "";
	uint8_t sliceInc = 0;
	
	for(uint8_t i = 0; i < s.length(); i++) {
		if(debug) {
			Serial.print("char: "); Serial.println(s[i], DEC);
		}
		if(s[i] == ' ') {
			slicedCmd[sliceInc++] = temp;
			temp = "";
			if(debug) {
				Serial.println("Separator...");
			}
		} else {
			temp += s[i];
		}
	}
	slicedCmd[sliceInc++] = temp;
	
	if(debug) {
		Serial.print("Sliced command line (length: "); Serial.print(sliceInc); Serial.println(")");
		for(uint8_t i = 0; i < (sliceInc); i++) {
			Serial.print(slicedCmd[i]); Serial.print(" ");
		}
		Serial.println("");
	}
	
	return sliceInc;
}


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* | convertStrToInt														| */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
uint8_t convertStrToInt(String s)
{
	char test[s.length()+1];
	s.toCharArray(test, sizeof(test));
	return atoi(test);
}

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
	for(uint8_t i = 0; i < HS_SLAVE_NUMBER; i++) {
		Wire.requestFrom(HSd.i2cSlaveAddress[i], 1);
    
		while(Wire.available()) {
			receivedAddr = Wire.read();
			if(receivedAddr == HSd.i2cSlaveAddress[i]) {
				HSd.i2cSlaveAvailable[i] = true;
			} else {
				HSd.i2cSlaveAvailable[i] = false;
			}
		}
		
		if(HSd.i2cSlaveAvailable[i] == true) {
			slaveInitNotify(HSd.i2cSlaveAddress[i], false);
		}
		if(debug) {
			Serial.print("Slave @ address 0x"); Serial.print(HSd.i2cSlaveAddress[i], HEX);
			Serial.println((HSd.i2cSlaveAvailable[i]) ? " available" : " NOT available");
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
		for(uint8_t i = 0; i < HS_DPS; i++) {
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
		}
	}
  
	// switch on...
	if(on) {
		if(debug) {
			Serial.println("Starting up drv2667...");
			Serial.print("- addressing i2c switch @ 0x"); Serial.println(addr, HEX);
		}

		// open each i2c switch channel and send settings to the attached drv2667
		for(uint8_t i = 0; i < HS_DPS; i++) {
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
		}
		
		
	} else { // switch off...
		if(debug) {
			Serial.println("Switching off drv2667...");
			Serial.print("- addressing i2c switch @ 0x"); Serial.println(addr, HEX);
		}
		
		// open each i2c switch channel and send standby command to the attached drv2667
		for(uint8_t i = 0; i < HS_DPS; i++) {
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
	Wire.write(i2cCmd_notify);
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
void distributeCoordinates(	uint8_t len, uint8_t orig[HS_COORD_MAX][2], uint8_t dest[HS_SLAVE_NUMBER][HS_COORD_MAX]) {
								
	uint8_t mod, sn, pi;		// index modulo, slave number, piezo index

	if(debug) {
		Serial.print("\nDistributing coordinates... length: "); Serial.println(len);
	}
	
	for(uint8_t i = 0; i < len; i++) {
		// check column value of a pair and assign it to the corresponding slave
		if(orig[i][0] < HS_CPS) {
			mod = 0;
			sn = 0;
		} else if(orig[i][0] < (2 * HS_CPS)) {
			mod = HS_CPS;
			sn = 1;
		} else if(orig[i][0] < (3 * HS_CPS)) {
			mod = 2 * HS_CPS;
			sn = 2;
		} else if(orig[i][0] < (4 * HS_CPS)) {
			mod = 3 * HS_CPS;
			sn = 3;
		} else if(orig[i][0] == SERIAL_CMD_MODE) {
			if(debug) {
				Serial.print("Entering command mode: 0x"); Serial.println(orig[i][1], HEX);
			}
			switch(orig[i][1]) {
				case sCmd_piezoOffAll:
					HSd.piezoOffAll = true;
					break;
				case sCmd_piezoOffS1:
					HSd.piezoOff[0] = true;
					break;
				case sCmd_piezoOffS2:
					HSd.piezoOff[1] = true;
					break;
				case sCmd_piezoOffS3:
					HSd.piezoOff[2] = true;
					break;
				case sCmd_piezoOffS4:
					HSd.piezoOff[3] = true;
					break;
				case sCmd_drvOffAll:
					HSd.drvOffAll = true;
					break;
				case sCmd_drvOn:
					HSd.drvOn = true;
					break;
				case sCmd_drvOff:
					HSd.drvOff = true;
					break;
				default:
					break;
			}
		}
		if(debug) {
			Serial.print("sn: "); Serial.print(sn, DEC);
			Serial.print(" ...available? "); Serial.println((HSd.i2cSlaveAvailable[sn]) ? "yes" : "no");
			
		}
		// work only for available devices
		if(HSd.i2cSlaveAvailable[sn]) {
			// calculate the linear position (piezo index) of the pair
			// and save it as next item of the selected slave of 'HSpiezo'.
			// !! always multiply by 9!! Not connected piezo will be skipped.
			pi = ((orig[i][0] - mod) * 9) + (orig[i][1] * 2);
			HSd.HSpiezo[sn][HSd.piCnt[sn]] = pi;
			HSd.piCnt[sn] += 1;	// increment the pi counter of the selected slave
		
			if(debug) {
				uint8_t pi5 = ((orig[i][0] - mod) * 5) + orig[i][1];
				Serial.print("Slave#: "); Serial.print(sn);
				Serial.print(" Piezo#: "); Serial.print(pi);
				Serial.print("("); Serial.print(pi5); Serial.println(")");
      
				Serial.print("HSpiezo: "); Serial.print(HSd.HSpiezo[sn][HSd.piCnt[sn]-1], DEC);
				Serial.print(" / piCnt: "); Serial.println(HSd.piCnt[sn]-1);
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
	Wire.write(i2cCmd_regSet);		// command byte with register set message
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

