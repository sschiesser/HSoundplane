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
#include "hsoundplane.h"
#include "masterSettings.h"

String command;							// serial command to parse

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* | setup																	| */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
void setup()
{
	// Set up sync pin(s)
	syncPinState = false;
	pinMode(SYNC_PIN_1, OUTPUT);
	digitalWrite(SYNC_PIN_1, syncPinState);
	
	// Toggle sync pin on and off to notify startup
	syncPinState = !syncPinState;
	digitalWrite(SYNC_PIN_1, syncPinState);
	syncPinState = !syncPinState;
	digitalWrite(SYNC_PIN_1, syncPinState);
	
	// Wait a bit to let the slaves starting up...
	delay(STARTUP_WAIT_MS);
	
	// Initialize HSoundplane variables...
	HSInit();

	// Set up communication...
	Serial.begin(SERIAL_SPEED);
	Wire.begin(); // Start i2c
	if(i2cFastMode) Wire.setClock(400000);
	
	// Welcome & information message...
	if(debug) {
		Serial.println("\nStarting up master controller...");
		Serial.println("*************************************");
		Serial.print("serial:\n\t- port @ "); Serial.println(SERIAL_SPEED, DEC);
		Serial.print("i2c:\n\t- port @ "); Serial.println((i2cFastMode) ? "400 kHz" : "100 kHz");
		Serial.print("slaves:\n\t- quantity: "); Serial.println(HS_SLAVE_NUMBER, DEC);
		Serial.print("piezos:\n\t- items/column: "); Serial.println((HSd.col9) ? "9" : "5");
		Serial.println("**************************************\n");
	}

	// Register slaves and list the avaiable ones
	slaveRegister();
	
	// Perform initialization for each available slave
	for(uint8_t i = 0; i < HS_SLAVE_NUMBER; i++) {
		delay(INIT_WAIT_MS);	// wait some ms between each slave to avoid collisions
		
		// Reset all drv2667 and switch them off
		driver = -1;
		reset = true;
		on = false;
		gain = 0;
		if(HSd.i2cSlaveAvailable[i]) {
			if(debug) {
				Serial.print("\nSetting up slave @ 0x"); Serial.print(HSd.i2cSlaveAddress[i], HEX);
				Serial.println("\n----------------------------------------");
			}
			// Set up the drv2667 and notify if succeeded
			bool setupOk = slaveDrvSetup(HSd.i2cSwitchAddress[i], driver, reset, on, gain);
			slaveInitNotify(HSd.i2cSlaveAddress[i], setupOk);

			// Toggle sync pin for time measurement
			syncPinState = !syncPinState;
			digitalWrite(SYNC_PIN_1, syncPinState);

			// Switch off all relays of selected slave
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
	char c;
	int8_t i2cAddr;
	uint8_t strLength;
	
	// Collect characters on the serial port started by '[' and terminated by ']'.
	// Then process the received command line
	if(Serial.available()) {
		c = Serial.read();
    
		// When start character received, empty the received string
		if(c == '[') {
			command = "";
		}
		// When stop character received, process the string
		else if(c == ']') {
			// Toggle sync pin for time measurement
			syncPinState = !syncPinState;
			digitalWrite(SYNC_PIN_1, syncPinState);

			command.trim();		// Trim leading and trailing spaces
			if(debug) {
				strLength = command.length();
				Serial.print("\n\n****************************************\nNew command: "); Serial.print(command);
				Serial.print(" (length: "); Serial.print(strLength); Serial.println(")");
				Serial.print("****************************************\n");
			}
			
			// Slice the received ASCII characters between spaces and
			// put the slices into slicedCmd[] array
			strLength = sliceCommand(command);

			// Toggle sync pin for time measurement
			syncPinState = !syncPinState;
			digitalWrite(SYNC_PIN_1, syncPinState);		// measuring slicing time

			// Input error check (odd coordinates)
			uint8_t pairs = strLength / 2;
			if(debug) {
				Serial.print("\nPairs: "); Serial.print(pairs, DEC);
				Serial.print(" x 2 = "); Serial.println((2 * pairs), DEC);
				Serial.print("strLength: "); Serial.println(strLength, DEC);
			}
			if((2 * pairs) == strLength) {
				// For each coordinate pair, convert the ASCII arrays into integers
				// and fill the HSd.HScoord[][] pairs
				for(uint8_t i = 0; i < pairs; i++) {
					HSd.HScoord[i][0] = convertStrToInt(slicedCmd[2*i]);
					HSd.HScoord[i][1] = convertStrToInt(slicedCmd[(2*i)+1]);
					if(debug) {
						Serial.print("HScoord: "); Serial.print(HSd.HScoord[i][0]);
						Serial.print(" "); Serial.println(HSd.HScoord[i][1]);
					}
				}
			
				// Toggle sync pin for time measurement
				// syncPinState = !syncPinState;
				// digitalWrite(SYNC_PIN_1, syncPinState);

				// Distribute HScoord to their corresponding slave and
				// calculate for each the piezo index or serial command
				distributeCoordinates(pairs, HSd.HScoord, HSd.HSpiezo);

				// Toggle sync pin for time measurement
				// syncPinState = !syncPinState;
				// digitalWrite(SYNC_PIN_1, syncPinState);

				// Command parser, resp. coordinate forwarder
				for(uint8_t i = 0; i < HS_SLAVE_NUMBER; i++) {
					if(HSd.i2cSlaveAvailable[i]) {	// for each available slave...
						// ...switch off all
						if(HSd.piezoOffAll[i]) {
							if(debug) {
								Serial.print("\nSending piezo off command to slave #"); Serial.println(i, DEC);
							}
							
							// Switch off all drivers
							driver = -1;
							reset = false;
							on = false;
							gain = 0;
							bool setupOk = slaveDrvSetup(HSd.i2cSwitchAddress[i], driver, reset, on, gain);

							if(debug) {
								if(setupOk) {
									Serial.println("drv2667 switched off, closing relays");
								} else {
									Serial.println("drv2667 setup failed!");
								}
							}
							// Close all relays, if switching off successful or not
							sendToSlave(HSd.i2cSlaveAddress[i], NULL, 0);
						}
						// ...send coordinate values
						else if(HSd.HSpiCnt[i] > 0) {
							// Toggle sync pin for time measurement
							syncPinState = !syncPinState;
							digitalWrite(SYNC_PIN_1, syncPinState);
							if(debug) {
								Serial.print("\nSending piezo settings to slave #"); Serial.println(i, DEC);
							}

							// Switch on the corresponding driver with maximum gain
							reset = false;
							on = true;
							gain = 3;
							bool setupOk = slaveDrvSetup(HSd.i2cSwitchAddress[i], driver, reset, on, gain);

							if(setupOk) {
								// Toggle sync pin for time measurement
								syncPinState = !syncPinState;
								digitalWrite(SYNC_PIN_1, syncPinState);

								if(debug) {
									Serial.println("drv2667 switched on, setting up relays");
								}

								sendToSlave(HSd.i2cSlaveAddress[i], HSd.HSpiezo[i], HSd.HSpiCnt[i]);
							} else {
								if(debug) {
									Serial.println("drv2667 setup failed, nothing done!");
								}
							}

							// Toggle sync pin for time measurement
							syncPinState = !syncPinState;
							digitalWrite(SYNC_PIN_1, syncPinState);
						}
					}
					// Reset all flags
					HSd.piezoOffAll[i] = false;
					HSd.HSpiCnt[i] = 0;
				}

				// Toggle sync pin for time measurement
				syncPinState = !syncPinState;
				digitalWrite(SYNC_PIN_1, syncPinState);		// measure sending time
			}
			// Odd number of coordinates entered, doing nothing
			else {
				if(debug) {
					Serial.println("Odd coordinate values, failed!");
				}
			}
		}
		// Concatenating currently entered input
		else {
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
		Serial.println("----------------------------------------");
		Serial.print("Received command length: "); Serial.println(s.length(), DEC);
	}
	
	String temp = "";
	uint8_t sliceInc = 0;
	
	for(uint8_t i = 0; i < s.length(); i++) {
		if(debug) {
			Serial.print("char: "); Serial.print(s[i], DEC);
		}
		if(s[i] == ' ') {
			slicedCmd[sliceInc++] = temp;
			temp = "";
			if(debug) {
				Serial.println(" - separator...");
			}
		} else {
			if(debug) {
				Serial.println("");
			}
			temp += s[i];
		}
	}
	slicedCmd[sliceInc++] = temp;
	
	if(debug) {
		Serial.print("\nSliced command line (length: "); Serial.print(sliceInc); Serial.println(")");
		for(uint8_t i = 0; i < (sliceInc); i++) {
			Serial.print(slicedCmd[i]); Serial.print(" ");
		}
		Serial.println("\n");
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
void slaveRegister()
{
	uint8_t receivedAddr;
  
	if(debug) {
		Serial.println("Registering slaves...");
		Serial.println("----------------------------------------");
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
bool slaveDrvSetup(int8_t addr, int8_t drv, bool reset, bool on, uint8_t gain)
{
	int8_t retVal;
	bool initOk = true;
	uint8_t slaveNum = addr - I2C_SWITCH_ADDR1;
	
	// initialization...
	if(reset) {
		if(debug) {
			Serial.println("\nResetting all drv2667...");
			Serial.print("- slave# "); Serial.print(slaveNum, DEC);
			Serial.print(" @ 0x"); Serial.println(addr, HEX);
			Serial.print("- driver# "); Serial.println((drv == -1) ? "ALL" : String(drv));
			Serial.print("- addressing i2c switch @ 0x"); Serial.println(addr, HEX);
		}
		// open the selected i2c switch channel (all channels if drv = -1) and
		// send reset command to the attached drv2667
		if(drv == -1) {			
			for(uint8_t i = 0; i < HS_DPS; i++) {
				// open switch
				Wire.beginTransmission(addr);		
				Wire.write((uint8_t)(1 << i));
				retVal = Wire.endTransmission();
				initOk = ((retVal == 0) && (initOk == true)) ? true : false;		
				if(debug) {
					Serial.print("\n- opening switch #"); Serial.print(i, DEC);
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
		} else {
			// open switch
			Wire.beginTransmission(addr);		
			Wire.write((uint8_t)(1 << drv));
			retVal = Wire.endTransmission();
			initOk = ((retVal == 0) && (initOk == true)) ? true : false;		
			if(debug) {
				Serial.print("\n- opening switch #"); Serial.print(drv, DEC);
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
			Serial.print("\n- closing i2c switch");
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

		// open the selected i2c switch channel (all channels if drv = -1) and
		// send settings to the attached drv2667
		if(drv == -1) {
			for(uint8_t i = 0; i < HS_DPS; i++) {
				// open switch
				Wire.beginTransmission(addr);
				Wire.write((uint8_t)(1 << i));
				retVal = Wire.endTransmission();
				initOk = ((retVal == 0) && (initOk == true)) ? true : false;		
				if(debug) {
					Serial.print("\n- opening switch #"); Serial.print(drv, DEC);
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
						Serial.println("\t\t\t\t\tsuccess!");
					} else {
						Serial.println("\t\t\t\t\tERROR!");
					}
				}

				// set mux & gain
				Wire.beginTransmission(DRV2667_I2C_ADDRESS);
				Wire.write(DRV2667_REG01);
				Wire.write(INPUT_MUX | gain);
				retVal = Wire.endTransmission();
				initOk = ((retVal == 0) && (initOk == true)) ? true : false;		
				if(debug) {
					Serial.print("- setting MUX&GAIN to 0x:"); Serial.print((INPUT_MUX | gain), HEX);
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
		} else {
			// open switch
			Wire.beginTransmission(addr);
			Wire.write((uint8_t)(1 << drv));
			retVal = Wire.endTransmission();
			initOk = ((retVal == 0) && (initOk == true)) ? true : false;		
			if(debug) {
				Serial.print("\n- opening switch #"); Serial.print(drv, DEC);
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
					Serial.println("\t\t\t\t\tsuccess!");
				} else {
					Serial.println("\t\t\t\t\tERROR!");
				}
			}

			// set mux & gain
			Wire.beginTransmission(DRV2667_I2C_ADDRESS);
			Wire.write(DRV2667_REG01);
			Wire.write(INPUT_MUX | gain);
			retVal = Wire.endTransmission();
			initOk = ((retVal == 0) && (initOk == true)) ? true : false;		
			if(debug) {
				Serial.print("- setting MUX&GAIN to 0x:"); Serial.print((INPUT_MUX | gain), HEX);
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
			Serial.print("\n- closing i2c switch");
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
		
		// open selected i2c switch channel (all channels if drv = -1) and
		// send standby command to the attached drv2667
		if(drv == -1) {
			for(uint8_t i = 0; i < HS_DPS; i++) {
				Wire.beginTransmission(addr);
				Wire.write((uint8_t)(1 << i));
				retVal = Wire.endTransmission();
				initOk = ((retVal == 0) && (initOk == true)) ? true : false;		
				if(debug) {
					Serial.print("\n- opening switch #"); Serial.print(i, DEC);
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
					Serial.print("- standing by");
					if(retVal == 0) {
						Serial.println("\t\t\t\t\tsuccess!");					
					} else {
						Serial.println("\t\t\t\t\tERROR!");					
					}
				}
			}
		} else {
			Wire.beginTransmission(addr);
			Wire.write((uint8_t)(1 << drv));
			retVal = Wire.endTransmission();
			initOk = ((retVal == 0) && (initOk == true)) ? true : false;		
			if(debug) {
				Serial.print("\n- opening switch #"); Serial.print(drv, DEC);
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
				Serial.print("- standing by");
				if(retVal == 0) {
					Serial.println("\t\t\t\t\tsuccess!");					
				} else {
					Serial.println("\t\t\t\t\tERROR!");					
				}
			}
		}
		
		// close i2c switch
		Wire.beginTransmission(addr);
		Wire.write(0);
		retVal = Wire.endTransmission();
		initOk = ((retVal == 0) && (initOk == true)) ? true : false;		
		if(debug) {
			Serial.print("\n- closing i2c switch");
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
		Serial.print("\nSending init notification to slave 0x"); Serial.println(addr, HEX);
		Serial.print("- value: "); Serial.println((notification) ? "true\n" : "false\n");
	}
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* | distributeCoordinates													| */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
void distributeCoordinates(uint8_t len, uint8_t orig[HS_COORD_MAX][2], uint8_t dest[HS_SLAVE_NUMBER][HS_COORD_MAX])
{
								
	uint8_t mod = 0;					// index modulo
	uint8_t sn = 0;						// slave number
	uint8_t pi = 0;						// piezo index
	uint8_t col = 0;

	if(debug) {
		Serial.print("\nDistributing coordinates... length: "); Serial.println(len);
		Serial.println("----------------------------------------");
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
				Serial.print("\nEntering command mode: ");
			}
			HSd.commandMode = true;
			switch(orig[i][1]) {
				case sCmd_piezoOffAll:
					if(debug) Serial.println("piezoOffAll");
					HSd.piezoOffAll[0] = true;
					HSd.piezoOffAll[1] = true;
					HSd.piezoOffAll[2] = true;
					HSd.piezoOffAll[3] = true;
					break;
				case sCmd_piezoOffS1:
					if(debug) Serial.println("piezoOffS1");
					HSd.piezoOffAll[0] = true;
					break;
				case sCmd_piezoOffS2:
					if(debug) Serial.println("piezoOffS2");
					HSd.piezoOffAll[1] = true;
					break;
				case sCmd_piezoOffS3:
					if(debug) Serial.println("piezoOffS3");
					HSd.piezoOffAll[2] = true;
					break;
				case sCmd_piezoOffS4:
					if(debug) Serial.println("piezoOffS4");
					HSd.piezoOffAll[3] = true;
					break;
				case sCmd_drvOnAll:
					if(debug) Serial.println("drvOnAll");
					HSd.drvOnAll[0] = true;
					HSd.drvOnAll[1] = true;
					HSd.drvOnAll[2] = true;
					HSd.drvOnAll[3] = true;
					break;
				case sCmd_drvOffAll:
					if(debug) Serial.println("drvOffAll");
					HSd.drvOffAll[0] = true;
					HSd.drvOffAll[1] = true;
					HSd.drvOffAll[2] = true;
					HSd.drvOffAll[3] = true;
					break;
				case sCmd_drvOnS1:
					if(debug) Serial.println("drvOnS1");
					HSd.drvOn[0] = true;
					break;
				case sCmd_drvOnS2:
					if(debug) Serial.println("drvOnS2");
					HSd.drvOn[1] = true;
					break;
				case sCmd_drvOnS3:
					if(debug) Serial.println("drvOnS3");
					HSd.drvOn[2] = true;
					break;
				case sCmd_drvOnS4:
					if(debug) Serial.println("drvOnS4");
					HSd.drvOn[3] = true;
					break;
				case sCmd_drvOffS1:
					if(debug) Serial.println("drvOffS1");
					HSd.drvOff[0] = true;
					break;
				case sCmd_drvOffS2:
					if(debug) Serial.println("drvOffS2");
					HSd.drvOff[1] = true;
					break;
				case sCmd_drvOffS3:
					if(debug) Serial.println("drvOffS3");
					HSd.drvOff[2] = true;
					break;
				case sCmd_drvOffS4:
					if(debug) Serial.println("drvOffS4");
					HSd.drvOff[3] = true;
					break;
				default:
					break;
			}
		} else {
			if(debug) {
				Serial.println("\nColumn value not valid! No slave...");
			}
			HSd.colError = true;
		}
		
		
		if(!HSd.commandMode && !HSd.colError) {
			if(debug) {
				Serial.print("\nWorking on slave #"); Serial.print(sn, DEC); Serial.println("...");
				Serial.print("- col value = "); Serial.println((orig[i][0] - mod), DEC);
				Serial.print("- raw value = "); Serial.println(orig[i][1], DEC);
				Serial.print("- available? "); Serial.println((HSd.i2cSlaveAvailable[sn]) ? "yes" : "no");
			}
			// work only for available devices AND available coordinates
			if(HSd.i2cSlaveAvailable[sn]) {
				if(HSd.col9 && (orig[i][1] > 8)) {
					HSd.rawError = true;
				}
				else if(!HSd.col9 && (orig[i][1] > 4)) {
					HSd.rawError = true;
				}
				if(debug && HSd.rawError) {
					Serial.print("Raw value not valid (> "); Serial.print((HSd.col9) ? "8" : "4");
					Serial.println(")!");
				}
				if(!HSd.rawError) {
					// calculate the linear position (piezo index) of the pair
					// and save it as next item of the selected slave of 'HSpiezo'.
					// !! always multiply by 9!! Not connected piezo will be skipped.
					pi = ((orig[i][0] - mod) * 9) + (orig[i][1] * 2);
					HSd.HSpiezo[sn][HSd.HSpiCnt[sn]] = pi;
					HSd.HSdrvOn[sn][HSd.HSpiCnt[sn]] = (orig[i][0] - mod);
					HSd.HSpiCnt[sn] += 1;	// increment the pi counter of the selected slave
					if(debug) {
						uint8_t pi5 = ((orig[i][0] - mod) * 5) + orig[i][1];
						// Serial.print("Slave#: "); Serial.print(sn);
						Serial.print("- piezo#: "); Serial.print(pi, DEC);
						Serial.print("("); Serial.print(pi5, DEC); Serial.println(")");
      
						Serial.print("- HSpiezo: "); Serial.print(HSd.HSpiezo[sn][HSd.HSpiCnt[sn]-1], DEC);
						Serial.print(" / HSpiCnt: "); Serial.println((HSd.HSpiCnt[sn]-1), DEC);
						Serial.print("- HSdrvOn: "); Serial.println(HSd.HSdrvOn[sn][HSd.HSpiCnt[sn]-1], DEC);
					}
				}
			}
		}
		HSd.commandMode = false;
		HSd.colError = false;
		HSd.rawError = false;
	}
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* | sendToSlave															| */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
void sendToSlave(uint8_t sAddr, uint8_t *mes, uint8_t len)
{
	if(debug) {
		Serial.print("\nWriting to 0x"); Serial.print(sAddr, HEX);
		Serial.print(" (length: "); Serial.print(len, DEC);
		Serial.println("):");
		Serial.println("----------------------------------------");
	}
  
	Wire.beginTransmission(sAddr);		// address slave @ sAddr
	Wire.write(i2cCmd_regSet);			// command byte with register set message
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

