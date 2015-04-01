
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
#define NUMBER_OF_SLAVES 3 // number of slaves to drive (max 4)
#define NUMBER_OF_COLUMNS 8 // number of columns per slave (usually 8)
#define PIEZO_RAWS_9 0 // piezo raws setting: 0 -> 5 items, 1 -> items

#define DRV_GAIN 1 // gain setting for drv2667 (0 - 3)
#define I2C_DRV2667_ADDRESS 0x59 // default i2c address of the drv2667
#define I2C_SWITCH_ADDRESS 0x23

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
 * DRV2667 control variables *
 * ------------------------- */
int i2cAddress = I2C_DRV2667_ADDRESS;
int drvGain = DRV_GAIN; // Gain value of the DRV2667. Possibilities: 0 (25V), 1 (50V), 2 (100V), 3 (200V)

boolean reset; // Flag to reset the DRV2667 (e.g. at startup)
boolean switchOn; // Flag to tell the DRV2667 if it has to switch ON or OFF

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

// Array of eight piezo columns (8 x 9) single addressed. To combine: bitwise AND (&)
unsigned long piezoArray1[4][9] = {
  {0xFFFFFFFE, 0xFFFFFFFD, 0xFFFFFFFB, 0xFFFFFFF7, 0xFFFFFFEF, 0xFFFFFFDF, 0xFFFFFFBF, 0xFFFFFF7F, 0xFFFFFEFF} , // Row 1
  {0XFFFFFDFF, 0XFFFFFBFF, 0XFFFFF7FF, 0XFFFFEFFF, 0XFFFFDFFF, 0XFFFFBFFF, 0XFFFF7FFF, 0XFFFEFFFF, 0XFFFDFFFF} , // Row 2
  {0XFFFBFFFF, 0XFFF7FFFF, 0XFFEFFFFF, 0XFFDFFFFF, 0XFFBFFFFF, 0XFF7FFFFF, 0XFEFFFFFF, 0XFDFFFFFF, 0XFBFFFFFF} , // Row 3
  {0XF7FFFFFF, 0xEFFFFFFF, 0xDFFFFFFF, 0xBFFFFFFF, 0x7FFFFFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000000}   // Row 4 (5/9)
};
unsigned long piezoArray2[4][9] = {
  {0xFFFFFFFE, 0xFFFFFFFD, 0xFFFFFFFB, 0xFFFFFFF7, 0xFFFFFFEF, 0xFFFFFFDF, 0xFFFFFFBF, 0xFFFFFF7F, 0xFFFFFEFF} , // Row 4 - 5
  {0XFFFFFDFF, 0XFFFFFBFF, 0XFFFFF7FF, 0XFFFFEFFF, 0XFFFFDFFF, 0XFFFFBFFF, 0XFFFF7FFF, 0XFFFEFFFF, 0XFFFDFFFF} , // Row 5 - 6
  {0XFFFBFFFF, 0XFFF7FFFF, 0XFFEFFFFF, 0XFFDFFFFF, 0XFFBFFFFF, 0XFF7FFFFF, 0XFEFFFFFF, 0XFDFFFFFF, 0XFBFFFFFF} , // Row 6 - 7
  {0XF7FFFFFF, 0xEFFFFFFF, 0xDFFFFFFF, 0xBFFFFFFF, 0x7FFFFFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000000}   // Row 7 (5/9)
};
byte piezoArray3[8] = {0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF, 0x7F}; // Row 8

// Resulting values of the bitwise AND of the peizo arrays
unsigned long piezoVal1;
unsigned long piezoVal2;
byte piezoVal3;


/******************************
 **          SETUP           **
 ******************************/
void setup()
{
    Serial.begin(9600);

  if(debug) {
    Serial.println("\n\nBonjour... starting up!\n");
  }

  // Converting maxPiezoVal to string
  if(debug) {
    Serial.print("********************************\nAvailable slaves: "); Serial.println(maxSlaveVal);
    Serial.print("# piezos / slave: "); Serial.println(piezoPerSlave);
    Serial.print("  -> Max available piezos: "); Serial.println(maxPiezoVal);
    Serial.println("********************************\n");
  }
  
  // Setting up flags
  doSwitch = false;
  
  // Setting up pin modes for LEDs (output)
  pinMode(LED1pin, OUTPUT);
  pinMode(LED2pin, OUTPUT);
  pinMode(LED3pin, OUTPUT);
  
  // Fancy LED dancing before starting
  for(int i = 0; i < 3; i++) { 
    digitalWrite(LED1pin, LOW);
    digitalWrite(LED2pin, HIGH);
    digitalWrite(LED3pin, HIGH);
    delay(50);
    digitalWrite(LED1pin, HIGH);
    digitalWrite(LED2pin, LOW);
    delay(50);
    digitalWrite(LED2pin, HIGH);
    digitalWrite(LED3pin, LOW);
    delay(50);
  }
  
  // Setting up LEDs to announce startup
  digitalWrite(LED1pin, LOW);
  digitalWrite(LED2pin, HIGH);
  digitalWrite(LED3pin, HIGH);

  // Setting up DRV2667 on i2c
  reset = true;
  switchOn = true;
//  drvGain = 3;
  Wire.begin(); // start i2c
  setupDriver(reset, switchOn, drvGain);
  
  // Setting up SPI for shift register control
   if(debug) Serial.print("Setting up serial communication... ");
   
   pinMode(OEpin, OUTPUT);
   pinMode(LOADpin, OUTPUT);
   pinMode(CLRpin, OUTPUT);

   SPI.begin();
   
   digitalWrite(CLRpin, HIGH); // Master reset active LOW
   digitalWrite(LOADpin, LOW); // Storage clock active on rising edge
   digitalWrite(OEpin, LOW); // Enable latch outputs
  
   // Initialization done!
   if(debug) Serial.println("done!");
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
      strLength = parseCommand(command);
      command = "";
    }
    else {
      command += c;
    }
  }
  
  if(doSwitch) {
    setValues(strLength);
    sendToPiezos();
  }
  
//  delay(50);
//  
//  setupDriver(false, false, 0);
//  
//  delay(50);
//  
//  setupDriver(false, true, 0);
}


/******************************
 **         SETVALUES        **
 ******************************/
void setValues(int len) {
  if(debug) Serial.println("Setting values");
  
  if(len >= 0) {
    int i, j;
    piezoVal1 = 0xFFFFFFFF;
    piezoVal2 = 0xFFFFFFFF;
    piezoVal3 = 0xFF;
//    bitWrite(byteToSend, swOn[0], LOW);
  
    for(i = 0; i < len; i++) {
      if((swOn[i] >= 0) && (swOn[i] < 9)) {
        piezoVal1 = piezoVal1 & piezoArray1[0][swOn[i]];
      }
      else if((swOn[i] >= 9) && (swOn[i] < 18)) {
        piezoVal1 = piezoVal1 & piezoArray1[1][swOn[i]-9];
      }
      else if((swOn[i] >= 18) && (swOn[i] < 27)) {
        piezoVal1 = piezoVal1 & piezoArray1[2][swOn[i]-18];
      }
      else if((swOn[i] >= 27) && (swOn[i] < 32)) {
        piezoVal1 = piezoVal1 & piezoArray1[3][swOn[i]-27];
      }
      else if((swOn[i] >= 32) && (swOn[i] < 41)) {
        piezoVal2 = piezoVal2 & piezoArray2[0][swOn[i]-32];
      }
      else if((swOn[i] >= 41) && (swOn[i] < 50)) {
        piezoVal2 = piezoVal2 & piezoArray2[1][swOn[i]-41];
      }
      else if((swOn[i] >= 50) && (swOn[i] < 59)) {
        piezoVal2 = piezoVal2 & piezoArray2[2][swOn[i]-50];
      }
      else if((swOn[i] >= 59) && (swOn[i] < 64)) {
        piezoVal2 = piezoVal2 & piezoArray2[3][swOn[i]-59];
      }
      else if((swOn[i] >= 64) && (swOn[i] < 72)) {
        piezoVal3 = piezoVal3 & piezoArray3[swOn[i]-64];
      }
    }
    
    if(debug) {
      Serial.print("Length: ");
      Serial.println(len);
      Serial.print("Values: ");
      Serial.print(piezoVal3, BIN);
      Serial.print(" - ");
      Serial.print(piezoVal2, BIN);
      Serial.print(" - ");
      Serial.println(piezoVal1, BIN);
    }
  }
  
//  digitalWrite(LOADpin, LOW);
//  Serial.print("Byte to send: 0x");
//  Serial.println(byteToSend, HEX);
//  shiftOut(MOSIpin, SCKpin, MSBFIRST, byteToSend);
//  digitalWrite(LOADpin, HIGH);
  doSwitch = false;  
}


  
/******************************
 **       SETUPDRIVER        **
 ******************************/
void setupDriver(boolean startup, boolean on, int gain) {
  int ret; // returned ack value from TWI
  int muxVal = (1 << 2); // control register 1
//  int enVal = (1 << 1); // control register 2
//  int stbyVal = (1 << 6); // control register 2
  
  // Initialization
  if(startup) {
    if(debug) Serial.println("Starting up DRV2667...");
    Wire.beginTransmission(i2cAddress);
    Wire.write(0x02); // access control register 2
    Wire.write(0x80); // reset device
    ret = Wire.endTransmission();
    delay(10);
  }
  
  // Switching ON
  if(on) {
    if(debug) {
      Serial.println("\nWaking up!");
    }
    Wire.beginTransmission(i2cAddress);
    Wire.write(0x02); // access control register 2
    Wire.write(0x00); // wake up device from standby mode
    ret = Wire.endTransmission();
    
    if(debug) {
      Serial.print("Setting MUX and GAIN: 0x");
      Serial.println((muxVal | gain), HEX);
    }
    Wire.beginTransmission(i2cAddress);
    Wire.write(0x01); // access control register 1
    Wire.write(muxVal | gain); // id = 0xA; mux = 1 (analog); gain = 0 (25V full scale)
    ret = Wire.endTransmission();
  
    if(debug) {
      Serial.println("Enabling amplifier");
    }
    Wire.beginTransmission(i2cAddress);
    Wire.write(0x02); // access control register 2
    Wire.write(0x02); // enable amplifier
    if((ret = Wire.endTransmission()) == 0) {
      digitalWrite(LED2pin, LOW); // Success!
    }
    else {
      digitalWrite(LED2pin, HIGH); // Error! Driver not set!
    }
 
  }
  // Switching OFF
  else {
    if(debug) {
      Serial.println("\nStand by");
    }
    Wire.beginTransmission(i2cAddress);
    Wire.write(0x02); // access control register 2
    Wire.write(0x40); // go to standby mode
    if((ret = Wire.endTransmission()) == 0) {
//      for(int i = 0; i < 4; i++) { // Success! Saying byebye...
        digitalWrite(LED2pin, LOW);
//        delay(20);
//        digitalWrite(LED2pin, HIGH);
//        delay(20);
//      }
    }
    else {
      digitalWrite(LED2pin, HIGH); // Error
    }
  }
}
  
  
  
/******************************
 **       SENDTOPIEZOS       **
 ******************************/
void sendToPiezos(void) {
  byte byteToSend;
  
  digitalWrite(LED3pin, LOW); // Signalize piezo sending on LED 3
  digitalWrite(LOADpin, LOW); //
  digitalWrite(CLRpin, LOW); // Resets the registers
//  delay(1);
  digitalWrite(CLRpin, HIGH);
  digitalWrite(LOADpin, HIGH);
  digitalWrite(LOADpin, LOW);

  if(maxPiezoVal > 64) {
    byteToSend = piezoVal3;
    SPI.transfer(byteToSend);
//    shiftOut(MOSIpin, SCKpin, MSBFIRST, byteToSend);
  }
  if(maxPiezoVal > 56) {
    byteToSend = (byte)(piezoVal2 >> 24);
    SPI.transfer(byteToSend);
//    shiftOut(MOSIpin, SCKpin, MSBFIRST, byteToSend);
  }
  if(maxPiezoVal > 48) {
    byteToSend = (byte)(piezoVal2 >> 16);
    SPI.transfer(byteToSend);
//    shiftOut(MOSIpin, SCKpin, MSBFIRST, byteToSend);
  }
  if(maxPiezoVal > 40) {
    byteToSend = (byte)(piezoVal2 >> 8);
    SPI.transfer(byteToSend);
//    shiftOut(MOSIpin, SCKpin, MSBFIRST, byteToSend);
  }
  if(maxPiezoVal > 32) {
    byteToSend = (byte)piezoVal2;
    SPI.transfer(byteToSend);
//    shiftOut(MOSIpin, SCKpin, MSBFIRST, byteToSend);
  }
  if(maxPiezoVal > 24) {
    byteToSend = (byte)(piezoVal1 >> 24);
    SPI.transfer(byteToSend);
//    shiftOut(MOSIpin, SCKpin, MSBFIRST, byteToSend);
  }
  if(maxPiezoVal > 16) {
    byteToSend = (byte)(piezoVal1 >> 16);
    SPI.transfer(byteToSend);
//    shiftOut(MOSIpin, SCKpin, MSBFIRST, byteToSend);
  }
  if(maxPiezoVal > 8) {
    byteToSend = (byte)(piezoVal1 >> 8);
    SPI.transfer(byteToSend);
//    shiftOut(MOSIpin, SCKpin, MSBFIRST, byteToSend);
  }
  if(maxPiezoVal > 0) {
    byteToSend = (byte)piezoVal1;
    SPI.transfer(byteToSend);
//    shiftOut(MOSIpin, SCKpin, MSBFIRST, byteToSend);
  }
  if(debug) {
    Serial.print("# piezos: ");
    Serial.println(maxPiezoVal);
    Serial.print("Complete 72-bit word: ");
    Serial.print(piezoVal3, BIN);
    Serial.print(" ");
    Serial.print(piezoVal2, BIN);
    Serial.print(" ");
    Serial.println(piezoVal1, BIN);
  }
  
  digitalWrite(LOADpin, HIGH); // pulse STCP HIGH to store the shift register value
  digitalWrite(LED3pin, HIGH);
}



/******************************
 **      PARSECOMMAND        **
 ******************************/
int parseCommand(String com) {
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
