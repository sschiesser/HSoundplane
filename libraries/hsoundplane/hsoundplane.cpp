#include "HSoundplane.h"

void driverSetup(bool startup, bool on, uint8_t gain) {
	// initialization...
	if(startup) {
		if(debug) Serial.println("Starting up DRV2667...\n- addressing i2c switch");
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
				digitalWrite(LED2_PIN, LED_ON);
				if(debug) Serial.println("SUCCESS!");
			} else {
				digitalWrite(LED2_PIN, LED_OFF);
				if(debug) Serial.println("ERROR!");
			}
		}
	} else { // switching off...
		if(debug) Serial.println("Switching off...\n- addressing i2c switch");
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
				digitalWrite(LED2_PIN, LED_OFF);
				if(debug) Serial.println("SUCCESS!");
			} else {
				digitalWrite(LED2_PIN, LED_ON);
				if(debug) Serial.println("ERROR!");
			}
		}
	}
}

uint8_t piezoSend(uint32_t val1, uint32_t val2, uint32_t val3) {
	digitalWrite(LED3_PIN, LOW);	// notify SPI activity
	digitalWrite(LOAD_PIN, LOW);	// prepare LOAD pin
	digitalWrite(CLR_PIN, LOW);		// reset all shift registers
	digitalWrite(CLR_PIN, HIGH);	// ...
	// digitalWrite(LOAD_PIN, HIGH);	// generate LOAD clock rising edge
	// digitalWrite(LOAD_PIN, LOW);	// ...
	
	SPI.beginTransaction(settingsA);
	SPI.transfer(val3);
	SPI.transfer((uint8_t)(val2 >> 24));
	SPI.transfer((uint8_t)(val2 >> 16));
	SPI.transfer((uint8_t)(val2 >> 8));
	SPI.transfer((uint8_t)(val2));
	SPI.transfer((uint8_t)(val1 >> 24));
	SPI.transfer((uint8_t)(val1 >> 16));
	SPI.transfer((uint8_t)(val1 >> 8));
	SPI.transfer((uint8_t)(val1));
	SPI.endTransaction();
	// shiftOut(MOSI_PIN, SCK_PIN, MSBFIRST, val3);
	// shiftOut(MOSI_PIN, SCK_PIN, MSBFIRST, (uint8_t)(val2 >> 24));
	// shiftOut(MOSI_PIN, SCK_PIN, MSBFIRST, (uint8_t)(val2 >> 16));
	// shiftOut(MOSI_PIN, SCK_PIN, MSBFIRST, (uint8_t)(val2 >> 8));
	// shiftOut(MOSI_PIN, SCK_PIN, MSBFIRST, (uint8_t)(val2));
	// shiftOut(MOSI_PIN, SCK_PIN, MSBFIRST, (uint8_t)(val1 >> 24));
	// shiftOut(MOSI_PIN, SCK_PIN, MSBFIRST, (uint8_t)(val1 >> 16));
	// shiftOut(MOSI_PIN, SCK_PIN, MSBFIRST, (uint8_t)(val1 >> 8));
	// shiftOut(MOSI_PIN, SCK_PIN, MSBFIRST, (uint8_t)(val1));
	
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
		
		Serial.print("0x"); Serial.print((uint8_t)val3, HEX);
		Serial.print("\t0x"); Serial.print(val2, HEX);
		Serial.print("\t0x"); Serial.println(val1, HEX);
	}
	
	digitalWrite(LOAD_PIN, HIGH);	// generate rising edge on LOAD pin
	digitalWrite(LED3_PIN, HIGH);
	
	return 0;
}


void slaveInit() {
	static uint8_t i, receivedAddr;
	static bool reset = true;
	static bool on = true;
	static uint8_t gain = 3;
  
	if(debug) Serial.println("Initializing slaves...");

	// Registering...
	for(i = 0; i < NUMBER_OF_SLAVES; i++) {
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
	for(i = 0; i < NUMBER_OF_SLAVES; i++) {
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


void distributeCoordinates(uint8_t len) {
  uint8_t modulo, slaveNum, piezoPointer;

  if(debug) Serial.println("Distributing coordinates...");
  for(int i = 0; i < len; i++) {
    if(coordinates[i][0] < COLUMNS_PER_SLAVE) {
      int slaveAddress = i2cSlaveAddresses[0];
      modulo = 0;
      slaveNum = 0;
    } else if(coordinates[i][0] < (2 * COLUMNS_PER_SLAVE)) {
      int slaveAddress = i2cSlaveAddresses[1];
      modulo = COLUMNS_PER_SLAVE;
      slaveNum = 1;
    } else if(coordinates[i][0] < (3 * COLUMNS_PER_SLAVE)) {
      int slaveAddress = i2cSlaveAddresses[2];
      modulo = 2 * COLUMNS_PER_SLAVE;
      slaveNum = 2;
    } else if(coordinates[i][0] < (4 * COLUMNS_PER_SLAVE)) {
      int slaveAddress = i2cSlaveAddresses[3];
      modulo = 3 * COLUMNS_PER_SLAVE;
      slaveNum = 3;
    }
     // always multiply by 9!! Not connected piezo will be skipped.
    piezoPointer = ((coordinates[i][0] - modulo) * 9) + (coordinates[i][1] * 2);
    piezoIndex[slaveNum][pi[slaveNum]] = piezoPointer;
    pi[slaveNum] += 1;
    if(debug) {
      int piezoPointer5 = ((coordinates[i][0] - modulo) * 5) + coordinates[i][1];
      Serial.print("Slave#: "); Serial.print(slaveNum);
      Serial.print(" Piezo#: "); Serial.print(piezoPointer);
      Serial.print("("); Serial.print(piezoPointer5); Serial.println(")");
      
      Serial.print("piezoIndex = "); Serial.print(piezoIndex[slaveNum][pi[slaveNum]-1], DEC);
      Serial.print("... pi = "); Serial.println(pi[slaveNum]-1);
    }
  }
}

void sendToSlave(int slaveNumber, char *message, int len) {
  if(debug) {
    Serial.print("Writing to 0x");
    Serial.print(slaveNumber, HEX);
    Serial.print(": ");
  }
  Wire.beginTransmission(slaveNumber);
  for(int i = 0; i < len; i++) {
    if(debug) {
      Serial.print(message[i], DEC);
      if(i < (len-1)) Serial.print(" - ");
    }
    Wire.write(message[i]);
  }
  if(debug) Serial.println("");
  Wire.endTransmission();

}


