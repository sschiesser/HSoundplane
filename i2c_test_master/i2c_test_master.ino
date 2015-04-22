// #include <Wire.h>
// #include <SPI.h>
// #include "drv2667.h"
// #include "HSoundplane.h"
//
// #define I2C_SWITCH_ADDRESS 0x70
//
// bool debug = false;
//
// void setup()
// {
// 	Wire.begin();
// }
//
// void loop()
// {
// 	bool s = false;
// 	bool on = true;
// 	uint8_t g = 0;
//
// 	for(;;) {
// 		Wire.beginTransmission(0x51);
// 		Wire.write(0xAA);
// 		Wire.endTransmission();
//
// 		delay(1000);
// 	}
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

#define THIS_ADDRESS 0x8
#define OTHER_ADDRESS 0x9

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
    last_state = !last_state;
    Wire.beginTransmission(OTHER_ADDRESS);
    Wire.write(last_state);
    Wire.endTransmission();
	
	delay(120);
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