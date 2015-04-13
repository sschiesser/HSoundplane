
/*
  Serial parser for the HSoundplane
  
  Hardware:
    - Arduino Due with USB CDC or RawHID communication
    - 1 to 4 slaves connected on UART0 to UART4
  
  Parses serial input and

*/

#include <Wire.h>
#include <SPI.h>
#include <String.h>

boolean debug = true;

/* ***************************
 *   USER-DEFINED SETTINGS   *
 * ***************************/
#define NUMBER_OF_SLAVES 4 // number of slaves to drive (max 4)
#define COLUMNS_PER_SLAVE 8 // number of columns per slave (usually 8)
#define PIEZO_RAWS_9 0 // piezo raws setting: 0 -> 5 items, 1 -> 9 items

#define I2C_SLAVE_ADDR1 0x51 // i2c slave addresses
#define I2C_SLAVE_ADDR2 0x52 // ..
#define I2C_SLAVE_ADDR3 0x53 // ..
#define I2C_SLAVE_ADDR4 0x54 // ..

#define I2C_FAST_MODE 1 // i2c fast mode flag: 0 -> off (100 kHz), 1 -> on (400 kHz)
#define SLAVE_INIT_COMMAND 0xFF
#define SERIAL_SPEED 57600

#define STARTUP_WAIT_MS 2000

#define MAX_COORD_PAIRS 16 // maximum number of touch coordinates
#define START_MARKER_MASK 0xE0 // bit mask to recognize serial start byte (ST)
#define STOP_MARKER_MASK 0xF0 // bit mask to recognize serial stop byte (SP)

/* ------------------------- *
 * Parsing command variables *
 * --------------------------*/
#if(PIEZO_RAWS_9 > 0)
  boolean piezoRaw9 = true;
  const int piezoMod = 9;
#else
  boolean piezoRaw9 = false;
  const int piezoMod = 5;
#endif

String command; // Serial command to parse

/* ------------------------- *
 * Soundplane variables      *
 * ------------------------- */
char coordinates[MAX_COORD_PAIRS][2]; // coordinate pairs table used in serial receiving
char piezoIndex[NUMBER_OF_SLAVES][MAX_COORD_PAIRS]; // for each slave, indexes of the piezos to address 
int pi[NUMBER_OF_SLAVES];
int piezoPerSlave = COLUMNS_PER_SLAVE * piezoMod; // # piezos for each slave module
int maxSlaveVal = NUMBER_OF_SLAVES; // # slave modules to talk to
int maxPiezoVal = maxSlaveVal * piezoPerSlave; // current maximum number of available piezos

/* arrays of 8 x 9 piezo matrix, single addressed.
 * to combine          -> bitwise AND (&)
 * to work with 5 raws -> jump over missing piezos
 */
//unsigned long piezoArray1[32] = {
//  0xFFFFFFFE, 0xFFFFFFFD, 0xFFFFFFFB, 0xFFFFFFF7, 0xFFFFFFEF, 0xFFFFFFDF, 0xFFFFFFBF, 0xFFFFFF7F, 0xFFFFFEFF, // Row 1
//  0XFFFFFDFF, 0XFFFFFBFF, 0XFFFFF7FF, 0XFFFFEFFF, 0XFFFFDFFF, 0XFFFFBFFF, 0XFFFF7FFF, 0XFFFEFFFF, 0XFFFDFFFF, // Row 2
//  0XFFFBFFFF, 0XFFF7FFFF, 0XFFEFFFFF, 0XFFDFFFFF, 0XFFBFFFFF, 0XFF7FFFFF, 0XFEFFFFFF, 0XFDFFFFFF, 0XFBFFFFFF, // Row 3
//  0XF7FFFFFF, 0xEFFFFFFF, 0xDFFFFFFF, 0xBFFFFFFF, 0x7FFFFFFF}; // Row 4 (5/9)
//  
//unsigned long piezoArray2[32] = {
//  0xFFFFFFFE, 0xFFFFFFFD, 0xFFFFFFFB, 0xFFFFFFF7, 0xFFFFFFEF, 0xFFFFFFDF, 0xFFFFFFBF, 0xFFFFFF7F, 0xFFFFFEFF, // Row 4 - 5
//  0XFFFFFDFF, 0XFFFFFBFF, 0XFFFFF7FF, 0XFFFFEFFF, 0XFFFFDFFF, 0XFFFFBFFF, 0XFFFF7FFF, 0XFFFEFFFF, 0XFFFDFFFF, // Row 5 - 6
//  0XFFFBFFFF, 0XFFF7FFFF, 0XFFEFFFFF, 0XFFDFFFFF, 0XFFBFFFFF, 0XFF7FFFFF, 0XFEFFFFFF, 0XFDFFFFFF, 0XFBFFFFFF, // Row 6 - 7
//  0XF7FFFFFF, 0xEFFFFFFF, 0xDFFFFFFF, 0xBFFFFFFF, 0x7FFFFFFF};   // Row 7 (5/9)
//  
//byte piezoArray3[8] = {0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF, 0x7F}; // Row 8


/* ------------------------- *
 * Arduino nano (slave) vars *
 * ------------------------- */
char i2cSlaveAddresses[] = {I2C_SLAVE_ADDR1, I2C_SLAVE_ADDR2, I2C_SLAVE_ADDR3, I2C_SLAVE_ADDR4};
boolean i2cSlaveAvailable[NUMBER_OF_SLAVES];
boolean drv2667Reset = true;
boolean drv2667SwitchOn = true;
char drv2667Gain = 3;
char slaveInitCommand = SLAVE_INIT_COMMAND;

#if(I2C_FAST_MODE > 0)
  boolean i2cFastMode = true;
#else
  boolean i2cFastMode = false;
#endif


/******************************
 **          SETUP           **
 ******************************/
void setup()
{
  Serial.begin(SERIAL_SPEED);
  Wire.begin(); // Start i2c
  if(i2cFastMode) Wire.setClock(400000);

  if(debug) {
    Serial.println("\n\nBonjour... starting up!\n");
    Serial.print("********************************\nAvailable slaves: "); Serial.println(maxSlaveVal);
    Serial.print("# piezos / slave: "); Serial.println(piezoPerSlave);
    Serial.print("  -> Max available piezos: "); Serial.println(maxPiezoVal);
    Serial.println("********************************\n");
    Serial.print("I2C set @"); Serial.println((i2cFastMode) ? "400 kHz" : "100 kHz");
  }

  slaves_init();

}


/******************************
 **           LOOP           **
 ******************************/
void loop()
{
  int strLength;
  
  if(Serial.available()) {
    char c = Serial.read();
    
    if(c == '\n') {
      strLength = parse_command(command);
      command = "";
      if(strLength != -1) {
        distribute_coordinates(strLength);
        for(int i = 0; i < NUMBER_OF_SLAVES; i++) {
          if(i2cSlaveAvailable[i]) {
            send_to_slave(i2cSlaveAddresses[i], piezoIndex[i], pi[i]);
          }
          pi[i] = 0;
        }
      }
    }
    else {
      command += c;
    }
  }
}



/******************************
 **  DISTRIBUTE_COORDINATES  **
 ******************************/
void distribute_coordinates(int len) {
  int modulo, slaveNum, piezoPointer;

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


/******************************
 **      SEND_TO_SLAVE       **
 ******************************/
void send_to_slave(int slaveNumber, char *message, int len) {
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


/******************************
 **       SLAVES_INIT        **
 ******************************/
void slaves_init() {
  unsigned int i, receivedAddr;
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
        Serial.print(": 0x"); Serial.print(slaveInitCommand, HEX);
        Serial.print(" - "); Serial.print(drv2667Reset);
        Serial.print(" - "); Serial.print(drv2667SwitchOn);
        Serial.print(" - "); Serial.println(drv2667Gain, DEC);
      }
      Wire.beginTransmission(i2cSlaveAddresses[i]);
      Wire.write(slaveInitCommand);
      Wire.write((char)drv2667Reset);
      Wire.write((char)drv2667SwitchOn);
      Wire.write(drv2667Gain);
      Wire.endTransmission();
    }
  }
}




/******************************
 **      PARSE_COMMAND       **
 ******************************/
int parse_command(String com) {
  if(debug) Serial.println("Entering parser...");

  String subPart; // substring used for trimming
  int i, piezoPairs;
  boolean cont; // Flag to continue slicing the command string
//  boolean slave; // Flag to indicate that slave address has been read
  
  unsigned int coord, index;
  boolean startMarkerSet, byteOne, firstPair, lastPair;
  
  if(debug) {
    Serial.print("Received: ");
  }
  Serial.println(com);
  if(com.length() != 0) {
    cont = true;
    byteOne = true;
    com.concat(" "); // Work around to be able to terminate the parsing of the string
    i = 0;
    while(cont) {
      subPart = com.substring(0, com.indexOf(" "));
      subPart.trim();
            
      /* Byte marker logic:
       * - if B1 (col) & start marker      -> first byte
       * - if B1 (col) & NOT start marker  -> any B1 (col)
       * - if B2 (raw) & stop marker       -> last byte
       * - if B2 (raw) & NOT stop marker   -> any B2 (raw)
       */
      coord = subPart.toInt();
      if(byteOne) {
        if(coord & START_MARKER_MASK) {
          firstPair = true;
          lastPair = false;
        } else {
          firstPair = false;
        }
      } else {
        if(coord & STOP_MARKER_MASK) {
          lastPair = true;
        } else {
          lastPair = false;
        }
      }
      
      if(byteOne & firstPair) {
        if(debug) Serial.print("B1 (col) & ST --> "); Serial.println(coord, DEC);
        index = 0;
        coordinates[index][0] = (coord & ~START_MARKER_MASK);
        byteOne = false;
      } else if(byteOne & !firstPair) {
        if(debug) Serial.print("B1 (col) --> "); Serial.println(coord, DEC);
        coordinates[index][0] = coord;
        byteOne = false;
      } else if(!byteOne & !lastPair) {
        if(debug) Serial.print("B2 (raw) --> "); Serial.println(coord, DEC);
        coordinates[index][1] = coord;
        index += 1;
        byteOne = true;
      } else if(!byteOne & lastPair) {
        if(debug) Serial.print("B2 (raw) & SP --> "); Serial.println(coord, DEC);
        coordinates[index][1] = (coord & ~STOP_MARKER_MASK);
        byteOne = true;
        piezoPairs = index + 1;
      } else {
        if(debug) Serial.println("ERROR in pair matching!");
        return -1;
      }
      
      com.remove(0, com.indexOf(" ") + 1);
      if(com.length() <= 0) cont = false;
    }
    
    if(debug) {
      Serial.print("Coordinates table (#pairs: "); Serial.print(piezoPairs); Serial.println("):");
      for(i = 0; i < piezoPairs; i++) {
        Serial.print("x "); Serial.print(coordinates[i][0], DEC);
        Serial.print("   y "); Serial.println(coordinates[i][1], DEC);
      }
    }
  }

  return piezoPairs; // returns the length of the parsed command line
}
