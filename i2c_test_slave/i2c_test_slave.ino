// #include <Wire.h>
// #include <SPI.h>
// #include "drv2667.h"
// #include "HSoundplane.h"
//
// #define I2C_SWITCH_ADDRESS	0x70
// #define SYNC_PIN_1			A0
//
// bool debug = false;
// bool syncPinState = false;
// bool transmit = false;
//
// void setup()
// {
// 	pinMode(SYNC_PIN_1, OUTPUT);
// 	Wire.begin(0x51);
// 	Wire.onReceive(receiveEvent);
// }
//
// void loop()
// {
// 	bool s = false;
// 	bool on = true;
// 	uint8_t g = 0;
//
// 	if(transmit) {
// 		Wire.beginTransmission(I2C_SWITCH_ADDRESS);
// 		Wire.write(0x55);
// 		Wire.endTransmission();
// 		transmit = false;
// 	}
// 	// for(;;) {
// 	// 	s = !s;
// 	// 	on = !on;
// 	// 	driverSetup(s, on, g);
// 	// 	g += 1;
// 	// 	delay(1000);
// 	// }
// }
//
// void receiveEvent(int howMany)
// {
// 	while(Wire.available() > 0) {
// 		syncPinState = !syncPinState;
// 		digitalWrite(SYNC_PIN_1, syncPinState);
// 		Wire.read();
// 		syncPinState = !syncPinState;
// 		digitalWrite(SYNC_PIN_1, syncPinState);
// 	}
// 	transmit = true;
// 	// driverSetup(true, false, 1);
// }

/**
 *
 * Sample Multi Master I2C implementation.  Sends a button state over I2C to another
 * Arduino, which flashes an LED correspinding to button state.
 * 
 * Connections: Arduino analog pins 4 and 5 are connected between the two Arduinos, 
 * with a 1k pullup resistor connected to each line.  Connect a push button between 
 * digital pin 10 and ground, and an LED (with a resistor) to digital pin 9.
 * 
 */

#include <Wire.h>

#define LED 13
#define BUTTON 10

#define THIS_ADDRESS 0x9
#define OTHER_ADDRESS 0x8

boolean last_state = true;

void setup() {
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  
  pinMode(BUTTON, INPUT);
  digitalWrite(BUTTON, HIGH);
  
  Wire.begin(THIS_ADDRESS);
  Wire.onReceive(receiveEvent);
}

void loop() {
  for(;;) {
    Wire.beginTransmission(OTHER_ADDRESS);
    Wire.write(last_state);
    Wire.endTransmission();
	
	last_state = !last_state;
	delay(134);
  }
}

void receiveEvent(int howMany){
  while (Wire.available() > 0){
    boolean b = Wire.read();
    Serial.print(b, DEC);
    digitalWrite(LED, !b);
  }
  Serial.println(); 
} 