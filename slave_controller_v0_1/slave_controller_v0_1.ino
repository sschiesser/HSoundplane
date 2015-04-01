#include <Wire.h>

#define TWI_FREQ 100000L
#define I2C_SLAVE_ADDRESS 0x51

boolean drv2667Reset, drv2667SwitchOn;
char drv2667Gain;

void setup() {
  Serial.begin(57600);
  Wire.begin(I2C_SLAVE_ADDRESS);
  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);
}

void loop() {
  delay(10);
}

void requestEvent() {
  Serial.print("I2C request received... sending own address 0x"); Serial.println(I2C_SLAVE_ADDRESS, HEX);
  Wire.write(I2C_SLAVE_ADDRESS);
}

void receiveEvent(int n) {
  Serial.print("I2C message received... ");
  drv2667Reset = (Wire.read() == 1) ? true : false;
  drv2667SwitchOn = (Wire.read() == 1) ? true : false;
  drv2667Gain = Wire.read();
  Serial.print(drv2667Reset); Serial.print(" - ");
  Serial.print(drv2667SwitchOn); Serial.print(" - ");
  Serial.println(drv2667Gain, DEC);
}
