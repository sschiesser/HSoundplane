#include <Wire.h>
#include <SPI.h>
#include "drv2667.h"
#include "hsoundplane.h"
#include "slaveSettings.h"

/* -------------------------------------------------------------- */
/* -------------------------------------------------------------- */
/* | SETUP														| */
/* -------------------------------------------------------------- */
/* -------------------------------------------------------------- */
void setup() {
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
  
	// Setting up output values
	digitalWrite(OE_PIN, HIGH); 		// disable latch outputs
	digitalWrite(CLR_PIN, HIGH);		// master restet active LOW
	digitalWrite(LOAD_PIN, LOW);		// storage clock active on rising edge
	digitalWrite(OE_PIN, LOW);			// enable latch outputs
  
	digitalWrite(SW_ADDR_0, LOW);		// i2c switch addresss
	digitalWrite(SW_ADDR_1, LOW);		// ...
	digitalWrite(SW_ADDR_2, LOW);		// ...
  
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
			digitalWrite(LED1_PIN, LOW);
			digitalWrite(LED2_PIN, HIGH);
			digitalWrite(LED3_PIN, HIGH);
			delay(50);
			digitalWrite(LED1_PIN, HIGH);
			digitalWrite(LED2_PIN, LOW);
			delay(50);
			digitalWrite(LED2_PIN, HIGH);
			digitalWrite(LED3_PIN, LOW);
			delay(50);
		}
	}
  
	digitalWrite(LED1_PIN, LOW); // announcing startup done...
	digitalWrite(LED2_PIN, HIGH);
	digitalWrite(LED3_PIN, HIGH);
}


/* -------------------------------------------------------------- */
/* -------------------------------------------------------------- */
/* | LOOP														| */
/* -------------------------------------------------------------- */
/* -------------------------------------------------------------- */
void loop() {
	delay(1);
}


/* -------------------------------------------------------------- */
/* -------------------------------------------------------------- */
/* | I2C REQUEST EVENT											| */
/* -------------------------------------------------------------- */
/* -------------------------------------------------------------- */
void requestEvent() {
	if(debug) {
		Serial.print("i2c registration request received... sending own address 0x"); Serial.println(I2C_SLAVE_ADDRESS, HEX);
	}
	Wire.write(I2C_SLAVE_ADDRESS);
}


/* -------------------------------------------------------------- */
/* -------------------------------------------------------------- */
/* | I2C RECEIVE EVENT											| */
/* -------------------------------------------------------------- */
/* -------------------------------------------------------------- */
void receiveEvent(int howmany) {
	uint32_t piezoVal1 = 0xFFFFFFFF;
	uint32_t piezoVal2 = 0xFFFFFFFF;
	uint32_t piezoVal3 = 0xFFFFFFFF;
  
	if(debug) {
		Serial.print("i2c message received... length: "); Serial.println(howmany, DEC);
	}
	
	uint8_t received = Wire.read();
	if(received == initCommand) {
		drv2667Reset = (Wire.read() == 1) ? true : false;
		drv2667SwitchOn = (Wire.read() == 1) ? true : false;
		drv2667Gain = Wire.read();
		if(debug) {
			Serial.print("INIT: ");
			Serial.print("reset - "); Serial.print((drv2667Reset) ? "true" : "false");
			Serial.print(" / ");
			Serial.print("switch on - "); Serial.print((drv2667SwitchOn) ? "true" : "false");
			Serial.print(" / ");
			Serial.print("gain - "); Serial.println(drv2667Gain, DEC);
			Serial.println("");
		}
		   driverSetup(drv2667Reset, drv2667SwitchOn, drv2667Gain);
	} else {
		uint8_t decount = howmany;
		while(decount > 0) {
			Serial.print("Received "); Serial.print(received, DEC);
			if(received < 32) {
				piezoVal1 &= piezoArray1[received];
				if(debug) Serial.print("\t-> piezoVal1: 0x"); Serial.println(piezoVal1, HEX);
			} else if(received < 64) {
				piezoVal2 &= piezoArray2[received-32];
				if(debug) Serial.print("\t-> piezoVal2: 0x"); Serial.println(piezoVal2, HEX);
			} else if(received < 72) {
				piezoVal3 &= piezoArray3[received-64];
				if(debug) Serial.print("\t-> piezoVal3: 0x"); Serial.println(piezoVal3, HEX);
			} else {
				if(debug) Serial.println("\t!!piezo value out of range!!");
			}
			received = Wire.read();
			decount--;
		}
		if(debug) Serial.println("");
		piezoSend(piezoVal1, piezoVal2, piezoVal3);
	}
}


/* -------------------------------------------------------------- */
/* -------------------------------------------------------------- */
/* | DRIVERS SETUP												| */
/* -------------------------------------------------------------- */
/* -------------------------------------------------------------- */
// void drivers_setup(bool startup, bool on, uint8_t gain) {
// 	// initialization...
// 	if(startup) {
// 		if(debug) Serial.println("Starting up DRV2667...\n- addressing i2c switch");
// 		Wire.beginTransmission(I2C_SWITCH_ADDRESS);
// 		for(uint8_t i = 0; i < 8; i++) {
// 			if(debug) Serial.print("- opening switch #"); Serial.println(i, DEC);
// 			Wire.write((uint8_t)(1 << i));
// 			Wire.endTransmission();
//
// 			if(debug) Serial.println("- resetting device");
// 			Wire.beginTransmission(DRV2667_I2C_ADDRESS);
// 			Wire.write(DRV2667_REG02);
// 			Wire.write(DEV_RST);
// 			Wire.endTransmission();
// 		}
// 	}
//
// 	// switch on...
// 	if(on) {
// 		if(debug) Serial.println("Switching on...\n- addressing i2c switch");
// 		Wire.beginTransmission(I2C_SWITCH_ADDRESS);
// 		for(uint8_t i = 0; i < 8; i++) {
// 			if(debug) Serial.print("- opening switch #"); Serial.println(i, DEC);
// 			Wire.write((uint8_t)(1 << i));
// 			Wire.endTransmission();
//
// 			if(debug) Serial.println("- waking up");
// 			Wire.beginTransmission(DRV2667_I2C_ADDRESS);
// 			Wire.write(DRV2667_REG02);
// 			Wire.write(GO);
//
// 			if(debug) Serial.print("- setting MUX and GAIN to 0x:"); Serial.println((INPUT_MUX | drv2667Gain), HEX);
// 			Wire.beginTransmission(DRV2667_I2C_ADDRESS);
// 			Wire.write(DRV2667_REG01);
// 			Wire.write(INPUT_MUX | drv2667Gain);
// 			Wire.endTransmission();
//
// 			if(debug) Serial.println("- enabling amplifier");
// 			Wire.beginTransmission(DRV2667_I2C_ADDRESS);
// 			Wire.write(DRV2667_REG02);
// 			Wire.write(EN_OVERRIDE);
// 			if(Wire.endTransmission() == 0) {
// 				digitalWrite(LED2_PIN, LOW);
// 				if(debug) Serial.println("SUCCESS!");
// 			} else {
// 				digitalWrite(LED2_PIN, HIGH);
// 				if(debug) Serial.println("ERROR!");
// 			}
// 		}
// 	} else { // switching off...
// 		if(debug) Serial.println("Switching off...\n- addressing i2c switch");
// 		Wire.beginTransmission(I2C_SWITCH_ADDRESS);
// 		for(uint8_t i = 0; i < 8; i++) {
// 			if(debug) Serial.print("- opening switch #"); Serial.println(i, DEC);
// 			Wire.write((uint8_t)(1 << i));
// 			Wire.endTransmission();
//
// 			if(debug) Serial.println("- standing by");
// 			Wire.beginTransmission(DRV2667_I2C_ADDRESS);
// 			Wire.write(DRV2667_REG02);
// 			Wire.write(STANDBY);
// 			if(Wire.endTransmission() == 0) {
// 				digitalWrite(LED2_PIN, HIGH);
// 				if(debug) Serial.println("SUCCESS!");
// 			} else {
// 				digitalWrite(LED2_PIN, LOW);
// 				if(debug) Serial.println("ERROR!");
// 			}
// 		}
// 	}
// }
