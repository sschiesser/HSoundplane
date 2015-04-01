#include <Wire.h>

#define TWI_FREQ 100000L
#define I2C_SLAVE_ADDRESS 0x51

void setup() {
  Serial.begin(57600);
  Wire.begin(I2C_SLAVE_ADDRESS);
  Wire.onRequest(requestEvent);
}

void loop() {
  delay(100);
}

void requestEvent() {
  Serial.print("I2C request received... sending "); Serial.println(I2C_SLAVE_ADDRESS);
  Wire.write(I2C_SLAVE_ADDRESS);
}
