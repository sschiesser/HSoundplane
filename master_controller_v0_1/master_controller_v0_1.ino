
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
#define NUMBER_OF_COLUMNS 8 // number of columns per slave (usually 8)
#define PIEZO_RAWS_9 0 // piezo raws setting: 0 -> 5 items, 1 -> items

#define I2C_SLAVE_ADDR1 0x51
#define I2C_SLAVE_ADDR2 0x52
#define I2C_SLAVE_ADDR3 0x53
#define I2C_SLAVE_ADDR4 0x54
#define TWI_FREQ 100000L

#define STARTUP_WAIT_MS 2000

#define MAX_COORD_PAIRS 16 // Maximum number of touch coordinates
#define START_MARKER_MASK 0xE0
#define STOP_MARKER_MASK 0xF0

/* ------------------------- *
 * Parsing command variables *
 * --------------------------*/
#if(PIEZO_RAWS_9 > 0)
  boolean piezoRaw9 = true;
  const int arrayItems = NUMBER_OF_COLUMNS * 9;
#else
  boolean piezoRaw9 = false;
  const int arrayItems = NUMBER_OF_COLUMNS * 5;
#endif

int piezoPerSlave = (piezoRaw9) ? (NUMBER_OF_COLUMNS * 9) : (NUMBER_OF_COLUMNS * 5);
int maxSlaveVal = NUMBER_OF_SLAVES;
int maxPiezoVal = maxSlaveVal * piezoPerSlave; // Current maximum number of available piezos

String command; // Serial command to parse
int swOff[arrayItems]; // Array of all possible piezos to switch OFF
int swOn[arrayItems]; // Array of all possible piezos to switch ON
int strLength; // Length of the entered command line string
boolean doSwitch; // Flag to enter the relay switching routine


/* ------------------------- *
 * Soundplane variables      *
 * ------------------------- */
char coordinates[MAX_COORD_PAIRS][2]; // Create the coordinate pairs table used in serial receiving


/* ------------------------- *
 * Arduino nano (slave) vars *
 * ------------------------- */
char i2cSlaveAddresses[] = {I2C_SLAVE_ADDR1, I2C_SLAVE_ADDR2, I2C_SLAVE_ADDR3, I2C_SLAVE_ADDR4};
boolean i2cSlaveAvailable[NUMBER_OF_SLAVES];
boolean drv2667Reset = true;
boolean drv2667SwitchOn = true;
char drv2667Gain = 3;


/* ------------------------------------------------- *
 * Arduino pins connected to the piezo driver header *
 * ------------------------------------------------- */
int LED1pin = 5; // LED1 -> Device started up
int LED2pin = 6; // LED2 -> Initialization successful
int LED3pin = 7; // LED3 -> SPI communication indication
int SCKpin = 13; // Shift clock (i.e. SPI clock)
int MISOpin = 12; //
int MOSIpin = 11; // Data serial (i.e. MOSI)
int OEpin = 10;
int LOADpin = 9; // Storage clock
int CLRpin = 8; // Master reset



/******************************
 **          SETUP           **
 ******************************/
void setup()
{
  Serial.begin(57600);

  if(debug) {
    Serial.println("\n\nBonjour... starting up!\n");
  }

  if(debug) {
    Serial.print("********************************\nAvailable slaves: "); Serial.println(maxSlaveVal);
    Serial.print("# piezos / slave: "); Serial.println(piezoPerSlave);
    Serial.print("  -> Max available piezos: "); Serial.println(maxPiezoVal);
    Serial.println("********************************\n");
  }

  Wire.begin(); // Start i2c
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
    }
    else {
      command += c;
    }
  }
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
        Serial.print(": "); Serial.print(drv2667Reset);
        Serial.print(" - "); Serial.print(drv2667SwitchOn);
        Serial.print(" - "); Serial.println(drv2667Gain, DEC);
      }
      Wire.beginTransmission(i2cSlaveAddresses[i]);
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
  int i, len;
  boolean cont; // Flag to continue slicing the command string
//  boolean slave; // Flag to indicate that slave address has been read
  
  unsigned int coord, index;
  boolean startMarkerSet, byteOne, firstPair, lastPair;
  
  if(debug) {
    Serial.print("Received: ");
    Serial.println(com);
  }
  if(com.length() != 0) {
    cont = true;
    byteOne = true;
    com.concat(" "); // Work around to be able to terminate the parsing of the string
    i = 0;
    while(cont) {
      subPart = com.substring(0, com.indexOf(" "));
      subPart.trim();
//      if(debug) Serial.print("Byte: "); Serial.println(subPart.toInt());
            
      /* Byte marker logic:
       * - if B1 (col) & start marker      -> first byte
       * - if B1 (col) & NOT start marker  -> any B1
       * - if B2 (raw) & stop marker       -> last byte
       * - if B2 (raw) & NOT stop marker   -> any B2
       */
//      if(debug) Serial.print("Working...\nbyteOne: "); Serial.println((byteOne) ? "true" : "false");
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
//      if(debug) {
//        Serial.print("firstPair: "); Serial.println((firstPair) ? "true" : "false");
//        Serial.print("lastPair: "); Serial.println((lastPair) ? "true" : "false");
//      }

      
      if(byteOne & firstPair) {
        if(debug) Serial.println("B1 & ST");
        index = 0;
        coordinates[index][0] = coord;
        byteOne = false;
      } else if(byteOne & !firstPair) {
        if(debug) Serial.println("B1 & ...");
        coordinates[index][0] = coord;
        byteOne = false;
      } else if(!byteOne & !lastPair) {
        if(debug) Serial.println("B2 & ...");
        coordinates[index][1] = coord;
        index += 1;
        byteOne = true;
      } else if(!byteOne & lastPair) {
        if(debug) Serial.println("B2 & SP");
        coordinates[index][1] = coord;
        byteOne = true;
        len = index + 1;
      } else {
        if(debug) Serial.println("ERROR in pair matching!");
      }
      
      com.remove(0, com.indexOf(" ") + 1);
      if(com.length() <= 0) cont = false;
    }
    
    if(debug) {
      Serial.println("Coordinates table:");
      for(i = 0; i < len; i++) {
        Serial.print("x "); Serial.print(coordinates[i][0], DEC);
        Serial.print("   y "); Serial.println(coordinates[i][1], DEC);
      }
    }
  }    
        
        
//      if(subPart.charAt(0) == 's') {
//        String str = subPart;
//        str.remove(0, 1); // Removing 's'
//        if(debug) {
//          Serial.print("'s' found... appending ");
//          Serial.println(str);
//        }
//        
//        if(switchOn == false) {
//          if(debug) {
//            Serial.print("Device standing by... waking up.\n");
//          }
//          reset = true;
//          switchOn = true;
//          setupDriver(reset, switchOn, drvGain);
//        }
//
//        if((str >= String(0)) && (str <= String(9))) {
//          if(str.toInt() < maxSlaveVal) {
//            if(debug) {
//              Serial.print("OK\n");
//              Serial.print("slave = ");
//              Serial.println(str.toInt());
//            }
//            
//            slave = true;
//          }
//          else {
//            if(debug) {
//              Serial.println("Wrong value!");
//            }
//            
//            cont = false;
//          }
//        }
//      }
//      else if(subPart.charAt(0) == 'x') {
//        if(debug) {
//          Serial.print("'x' found... exiting\n");
//        }
//        
//        reset = false;
//        switchOn = false;
////        drvGain = 0;
//        setupDriver(reset, switchOn, drvGain);
//
//        slave = false;
//      }
//      
//      if(slave == true) {
//        if((subPart >= String(0)) && (subPart <= String(9))) {
//          if(debug) Serial.println("Valid type");
//          if(subPart.toInt() < maxPiezoVal) {
//            if(debug) Serial.println("Valid number");
//            swOn[i] = subPart.toInt();
//            i += 1;
//            len = i;
//          }
//          else {
//            if(debug) Serial.println("Invalid number! Flushing...");
//          }
//        }
//        else {
//          if(debug) Serial.println("Invalid type! Flushing...");
//        }
//        
//        com.remove(0, com.indexOf(" ") + 1);
//        
//        if(debug) {
//          Serial.print("subPart: ");
//          Serial.println(subPart);
//          Serial.print("piezo#: ");
//          Serial.println(swOn[i-1]);
//          Serial.print("remaining string: ");
//          Serial.println(com);
//        }
//        if(com.length() <= 0) cont = false;
//      }
//      else {
//        cont = false;
//      }
//    }
//  
//      if(debug) {
//        Serial.print("Whole array: ");
//        for(i = 0; i < len; i++) {
//          Serial.print(swOn[i]);
//          Serial.print(" - ");
//        }
//        Serial.print("\n");
//      }
//    }
//    else {
//      len = 0;
//      cont = false;
//    }
//    
//  doSwitch = true;

  return len; // returns the length of the parsed command line
}
