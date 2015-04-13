#include <Wire.h>

boolean debug = true;

#define I2C_FAST_MODE 1
#define I2C_SLAVE_ADDRESS 0x51
#define SERIAL_SPEED 57600
#define SLAVE_INIT_COMMAND 0xFF
#define NUMBER_OF_COLUMNS 8
#define PIEZO_RAWS_9 0

#if(I2C_FAST_MODE > 0)
  boolean i2cFastMode = true;
#else
  boolean i2cFastMode = false;
#endif

#if(PIEZO_RAWS_9 > 0)
  const int piezoPerSlave = NUMBER_OF_COLUMNS * 9;
#else
  const int piezoPerSlave = NUMBER_OF_COLUMNS * 5;
#endif

boolean drv2667Reset, drv2667SwitchOn;
char drv2667Gain;
char initCommand = SLAVE_INIT_COMMAND;
char receiveTable[piezoPerSlave];

void setup() {
  Serial.begin(SERIAL_SPEED);
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

void receiveEvent(int howmany) {
  char received = Wire.read();
  
  if(debug) {
    Serial.print("I2C message received... length: "); Serial.println(howmany, DEC);
  }
  if(received == initCommand) {
    drv2667Reset = (Wire.read() == 1) ? true : false;
    drv2667SwitchOn = (Wire.read() == 1) ? true : false;
    drv2667Gain = Wire.read();
    if(debug) {
      Serial.print("INIT - ");
      Serial.print((drv2667Reset) ? "true" : "false"); Serial.print(" - ");
      Serial.print((drv2667SwitchOn) ? "true" : "false"); Serial.print(" - ");
      Serial.println(drv2667Gain, DEC);
    }
  } else {
    receiveTable[0] = received;
    if(debug) Serial.print(receiveTable[0], DEC); Serial.print(" - ");
    for(int i = 1; i < howmany; i++) {
      receiveTable[i] = Wire.read();
      if(debug) {
        Serial.print(receiveTable[i], DEC); 
        if(i < (howmany - 1)) Serial.print(" - ");
      }
    }
    if(debug) Serial.println("");
  }
}
