#include "hsoundplane.h"

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
				digitalWrite(LED2_PIN, LOW);
				if(debug) Serial.println("SUCCESS!");
			} else {
				digitalWrite(LED2_PIN, HIGH);
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
				digitalWrite(LED2_PIN, HIGH);
				if(debug) Serial.println("SUCCESS!");
			} else {
				digitalWrite(LED2_PIN, LOW);
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