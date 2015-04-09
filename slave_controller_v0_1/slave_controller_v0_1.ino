#include <Wire.h>

boolean debug = false;

#define I2C_FAST_MODE 1
#define I2C_SLAVE_ADDRESS 0x51

#if(I2C_FAST_MODE > 0)
  boolean i2cFastMode = true;
#else
  boolean i2cFastMode = false;
#endif

boolean drv2667Reset, drv2667SwitchOn;
char drv2667Gain;

void setup() {
  Serial.begin(57600);
  Wire.begin(I2C_SLAVE_ADDRESS);
  if(i2cFastMode) Wire.setClock(400000); //TWBR = 12;
  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);
}

void loop() {
  delay(1);
}

void requestEvent() {
  if(debug) {
    Serial.print("I2C request received... sending own address 0x"); Serial.println(I2C_SLAVE_ADDRESS, HEX);
  }
  Wire.write(I2C_SLAVE_ADDRESS);
}

void receiveEvent(int n) {
  drv2667Reset = (Wire.read() == 1) ? true : false;
  drv2667SwitchOn = (Wire.read() == 1) ? true : false;
  drv2667Gain = Wire.read();
  if(debug) {
    Serial.print("I2C message received... ");
    Serial.print(drv2667Reset); Serial.print(" - ");
    Serial.print(drv2667SwitchOn); Serial.print(" - ");
    Serial.println(drv2667Gain, DEC);
  }
}
