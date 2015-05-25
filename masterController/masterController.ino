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

// String command;							// serial command to parse

// uint8_t cmd[250];
bool cmdByteOne = false;
bool cmdZeroLen = false;
// bool cmdError = false;
int8_t cmdLength;
int8_t cmdIndexCnt;


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

	// Initialize HSoundplane variables...
	HSInit();
	
	// Wait a bit to let the slaves starting up...
	delay(STARTUP_WAIT_MS);
	
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
		
		bool reset = true;
		bool on = true;
		uint8_t gain = 3;
		if(HSd.i2cSlaveAvailable[i]) {
			if(debug) {
				Serial.print("\nSetting up slave @ 0x"); Serial.print(HSd.i2cSlaveAddress[i], HEX);
				Serial.println("\n----------------------------------------");
			}
			// Set up the drv2667 and notify if succeeded
			HSd.i2cSlaveSetup[i] = slaveDrvSetup(HSd.i2cSwitchAddress[i], reset, on, gain);
			slaveInitNotify(HSd.i2cSlaveAddress[i], HSd.i2cSlaveSetup[i]);

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
	byte b;
	int8_t i2cAddr;
	
	// Collect characters on the serial port started by '[' and terminated by ']'.
	// Then process the received command line
	if(Serial.available()) {
		b = Serial.read();
    
		// When start character received...
		// --------------------------------
		if(b == SCMD_START) {
		    // reset the command index counter and prepare reading length
			if(debug) {
				Serial.println("Start byte received...");
			}
			cmdIndexCnt = 0;
			cmdByteOne = true;
		}
		
		// When stop character received...
		// -------------------------------
		else if(b == SCMD_STOP) {
			if(debug) {
				Serial.println("Stop byte received...");
			}

			// Check if stop byte is simultaneously byte one (MSG: "START STOP")
			if(cmdByteOne) {
				if(debug) {
					Serial.println("Start/stop message received");
				} else {
					Serial.write(SERR_NOERROR);
					Serial.write(SERR_CRLF);
				}

				// Close all relays of all available slaves
				for(uint8_t i = 0; i < HS_SLAVE_NUMBER; i++) {
					if(HSd.i2cSlaveAvailable[i]) {	// for each available slave...
						if(debug) {
							Serial.print("\nSending piezo off command to slave #"); Serial.println(i, DEC);
						}
						sendToSlave(HSd.i2cSlaveAddress[i], NULL, 0);
					}
				}
				cmdByteOne = false;			// Reset byte one flag
			}
			
			// Verify if entered command length corresponds to counted items
			else if(cmdIndexCnt == cmdLength) {
				if(debug) {
					Serial.print("\r\n\r\n****************************************\r\nNew command: "); 
					for(uint8_t i = 0; i < (cmdIndexCnt/2); i++) {
						uint8_t col = ( ((i % 2) == 0) ? (i/2) : ((i-1)/2) );
						Serial.print(HSd.HScoord[i][0], DEC); Serial.print(" ");
						Serial.print(HSd.HScoord[i][1], DEC); Serial.print(" ");
						// Serial.print(cmd[i], DEC); Serial.print(" ");
					}
					Serial.print(" (l = "); Serial.print(cmdIndexCnt, DEC); Serial.println(")");
					Serial.print(" Entered length: "); Serial.println(cmdLength, DEC);
					Serial.print("****************************************\r\n");
				} else {
					Serial.write(SERR_NOERROR);
					Serial.write(SERR_CRLF);
				}
				
				// Toggle sync pin for time measurement
				syncPinState = !syncPinState;
				digitalWrite(SYNC_PIN_1, syncPinState);

				distributeCoordinates((cmdLength/2), HSd.HScoord, HSd.HSpiezo);

				// Toggle sync pin for time measurement
				syncPinState = !syncPinState;
				digitalWrite(SYNC_PIN_1, syncPinState);

				parseCommand();
				
				// Toggle sync pin for time measurement
				syncPinState = !syncPinState;
				digitalWrite(SYNC_PIN_1, syncPinState);

			}
			
			// When checks did not match, send a mismatch error
			else {
				if(debug) {
					Serial.print("ERROR#"); Serial.print(SERR_MISMATCH, DEC);
					Serial.println("! Mismatch");
				} else {
					Serial.write(SERR_MISMATCH);
					Serial.write(SERR_CRLF);
				}
			}
		}
		
		// When cmdByteOne flag is active...
		// ---------------------------------
		else if(cmdByteOne) {
			cmdLength = b;
			cmdByteOne = false;
		}
		
		// Concatenating currently entered input...
		// ----------------------------------------
		else {
			if(debug) {
				Serial.print("cmdIndexCnt = "); Serial.print(cmdIndexCnt, DEC);
				Serial.print(", cmdLength = "); Serial.println(cmdLength, DEC);
			}
			
			// cmdIndexCnt is indexed for each byte, while the HScoord table saves coordinate
			// pairs of half length
			uint8_t p = ( ((cmdIndexCnt % 2) == 0) ? (cmdIndexCnt/2) : ((cmdIndexCnt-1)/2) );
			if( (cmdIndexCnt % 2) == 0) {
				HSd.HScoord[p][0] = b;
			} else {
				HSd.HScoord[p][1] = b;
			}

			cmdIndexCnt += 1;
		}
	}
}


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* | parseCommand															| */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
void parseCommand(void)
{
	// Command parser, resp. coordinate forwarder
	for(uint8_t i = 0; i < HS_SLAVE_NUMBER; i++) {
		// if(HSd.i2cSlaveAvailable[i]) {	// for each available slave...
			// ...switch off all
			if(HSd.piezoOffAll[i]) {
				if(debug) {
					Serial.print("\nSending piezo off command to slave #"); Serial.println(i, DEC);
				}
				// Close all relays, if switching off successful or not
				sendToSlave(HSd.i2cSlaveAddress[i], NULL, 0);
			}
			// ...send coordinate values
			else if(HSd.piCnt[i] > 0) {
				// // Toggle sync pin for time measurement
				// syncPinState = !syncPinState;
				// digitalWrite(SYNC_PIN_1, syncPinState);
				
				if(debug) {
					Serial.print("\nSending piezo settings to slave #"); Serial.println(i, DEC);
				}
				
				sendToSlave(HSd.i2cSlaveAddress[i], HSd.HSpiezo[i], HSd.piCnt[i]);

				// // Toggle sync pin for time measurement
				// syncPinState = !syncPinState;
				// digitalWrite(SYNC_PIN_1, syncPinState);
			}
			else if(HSd.piCnt[i] == 0) {
				sendToSlave(HSd.i2cSlaveAddress[i], NULL, 0);
			}
		// }
		// Reset all flags
		HSd.piezoOffAll[i] = false;
		HSd.piCnt[i] = 0;
	}
	// // Toggle sync pin for time measurement
	// syncPinState = !syncPinState;
	// digitalWrite(SYNC_PIN_1, syncPinState);
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

	// Registering each slave...
	for(uint8_t i = 0; i < HS_SLAVE_NUMBER; i++) {
		// If registration doesn't succeed, retry several times to be sure
		for(uint8_t j = 0; j < SLAVE_REG_RETRIES; j++) {
			Wire.requestFrom(HSd.i2cSlaveAddress[i], 1);
			while(Wire.available()) {
				receivedAddr = Wire.read();
				if(receivedAddr == HSd.i2cSlaveAddress[i]) {
					HSd.i2cSlaveAvailable[i] = true;
					j = SLAVE_REG_RETRIES;
				} else {
					HSd.i2cSlaveAvailable[i] = false;
				}
			}
		}
		
		if(debug) {
			Serial.print("Slave @ address 0x"); Serial.print(HSd.i2cSlaveAddress[i], HEX);
			Serial.println((HSd.i2cSlaveAvailable[i]) ? " available" : " NOT available");
		}
		if(HSd.i2cSlaveAvailable[i] == true) {
			slaveInitNotify(HSd.i2cSlaveAddress[i], false);
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
bool slaveDrvSetup(int8_t addr, bool reset, bool on, uint8_t gain)
{
	int8_t retVal;
	bool initOk = true;
	
	// initialization...
	if(reset) {
		if(debug) {
			Serial.println("\nResetting drv2667...");
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

		// open each i2c switch channel and send settings to the attached drv2667
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
		
		// open each i2c switch channel and send standby command to the attached drv2667
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
		Serial.print("\nSending notification to slave 0x"); Serial.print(addr, HEX);
		Serial.println((notification) ? ": true\n" : ": false\n");
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
	bool cErr = false;					// column input error flag
	bool rErr = false;					// raw input error flag
	bool cmdMode = false;				// command mode flag

	if(debug) {
		Serial.print("\nDistributing coordinates... pairs: "); Serial.println(len);
		Serial.println("----------------------------------------");
	}
	
	// For each coordinate pair, the first item is tested.
	// If it is above a threshold value, a setting mode is entered FOR THE GIVEN PAIR!
	// Else the pair is processed like a standard coordinate.
	for(uint8_t i = 0; i < len; i++) {
		// Check (first) if command mode was entered
		if(orig[i][0] >= SCMD_SETTINGS) {
			cmdMode = true;
			switch(orig[i][0]) {
				// Switch off all relays of slave# given in orig[i][1]
				case SCMD_POFF_ALL:
				HSd.piezoOffAll[orig[i][1]] = true;
				break;
				// Switch off all drivers of slave# given in orig[i][1]
				case SCMD_DOFF_ALL:
				break;
				// Switch off drivers of slave1 according to the bitmask in orig[i][1]
				case SCMD_DOFF_S1:
				break;
				// Switch off drivers of slave2 according to the bitmask in orig[i][1]
				case SCMD_DOFF_S2:
				break;
				// Switch off drivers of slave3 according to the bitmask in orig[i][1]
				case SCMD_DOFF_S3:
				break;
				// Switch off drivers of slave4 according to the bitmask in orig[i][1]
				case SCMD_DOFF_S4:
				break;
				// Switch on all drivers of slave# given in orig[i][1]
				case SCMD_DON_ALL:
				break;
				// Switch on drivers of slave1 according to the bitmask in orig[i][1]
				case SCMD_DON_S1:
				break;
				// Switch on drivers of slave2 according to the bitmask in orig[i][1]
				case SCMD_DON_S2:
				break;
				// Switch on drivers of slave3 according to the bitmask in orig[i][1]
				case SCMD_DON_S3:
				break;
				// Switch on drivers of slave4 according to the bitmask in orig[i][1]
				case SCMD_DON_S4:
				break;
				// No command match
				default:
				break;
			}
		}

		// Check column value of a pair and assign it to the corresponding slave.
		// Note that coordinate system of the HSoundplane is a bit error generating:
		// - Soundplane input example:		(6, 3)
		// - HSoundplane coordinate:		(7, 3)
		// - Corresponding audio channel:	8
		// This is due to the fact that the 4 haptic slaves are made of 8 channels each,
		// with first (0) and last (31) one are used for audio purpose. That means that
		// the Soundplane columns 0 to 29 correspond to the haptic columns 1 to 30, on
		// audio channels 2 to 31!
		// Thus, first of all, add 1 to the input column number!
		else {
			orig[i][0] += 1;
			sn = orig[i][0] / HS_CPS;
			mod = sn * HS_CPS;
			
			// Nothing matched... something went entered wrong.
			if(orig[i][0] > 30) {
				if(debug) {
					Serial.println("\nColumn value not valid! No slave...");
				}
				cErr = true;
			}
		}
	
		if(!cmdMode && !cErr) {
			if(debug) {
				Serial.print("\nWorking on slave #"); Serial.print(sn, DEC); Serial.println("...");
				Serial.print("- col value = "); Serial.println((orig[i][0] - mod), DEC);
				Serial.print("- raw value = "); Serial.println(orig[i][1], DEC);
				Serial.print("- available? "); Serial.println((HSd.i2cSlaveAvailable[sn]) ? "yes" : "no");
				Serial.print("- setup ok? "); Serial.println((HSd.i2cSlaveSetup[sn]) ? "yes" : "no");
			}
			
			// work only for available devices
			if((HSd.i2cSlaveAvailable[sn])) {
				// Check if raw value entered correctly
				if(HSd.col9) {
					if(orig[i][1] > 8) {
						// HSd.rawError = true;
						rErr = true;
						if(debug) {
							Serial.println("Raw value not valid (> 8)!");
						}						
					}
				} else {
					if(orig[i][1] > 4) {
						// HSd.rawError = true;
						rErr = true;
						if(debug) {
							Serial.println("Raw value not valid (> 4)!");
						}						
					}
				}

				if(!rErr) {
					// calculate the linear position (piezo index) of the pair
					// and save it as next item of the selected slave of 'HSpiezo'.
					// !! always multiply by 9!! Not connected piezo will be skipped.
					pi = ((orig[i][0] - mod) * 9) + (orig[i][1] * 2);
					HSd.HSpiezo[sn][HSd.piCnt[sn]] = pi;
					HSd.piCnt[sn] += 1;	// increment the pi counter of the selected slave
		
					if(debug) {
						uint8_t pi5 = ((orig[i][0] - mod) * 5) + orig[i][1];
						// Serial.print("Slave#: "); Serial.print(sn);
						Serial.print("- piezo#: "); Serial.print(pi, DEC);
						Serial.print("("); Serial.print(pi5, DEC); Serial.println(")");
      
						Serial.print("- HSpiezo: "); Serial.print(HSd.HSpiezo[sn][HSd.piCnt[sn]-1], DEC);
						Serial.print(" / piCnt: "); Serial.println((HSd.piCnt[sn]-1), DEC);
					}
				}
			}
		}
		// Reset all flags within the loop to be able to process other coordinates
		cErr = false;
		rErr = false;
	}
	// Reset command mode flag
	cmdMode = false;
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

