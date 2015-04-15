#include <Wire.h>
#include <SPI.h>
#include <String.h>
#include "HSoundplane.h"
#include "masterSettings.h"

/******************************
 **          SETUP           **
 ******************************/
void setup()
{
  Serial.begin(SERIAL_SPEED);
  Wire.begin(); // Start i2c
  if(i2cFastMode) Wire.setClock(400000);

  if(debug) {
    Serial.println("\nStarting up master controller...");
    Serial.println("*************************************");
    Serial.print("serial:\n\t- port @ "); Serial.println(SERIAL_SPEED, DEC);
    Serial.print("i2c:\n\t- port @ "); Serial.println((i2cFastMode) ? "400 kHz" : "100 kHz");
    Serial.print("slaves:\n\t- quantity: "); Serial.println(NUMBER_OF_SLAVES, DEC);
    Serial.print("\t- piezos/slave: "); Serial.println(PIEZOS_PER_SLAVE, DEC);
    Serial.print("\t -> available piezos: "); Serial.println(MAX_PIEZO_VAL, DEC);
    // Serial.print("drivers:\n\t- gain: "); Serial.println(drv2667Gain, DEC);
    Serial.println("**************************************\n");
  }

  slaveInit();
}


/******************************
 **           LOOP           **
 ******************************/
void loop()
{
  uint8_t strLength;
  
  if(Serial.available()) {
    char c = Serial.read();
    
    if(c == '\n') {
      strLength = parseCommand(command);
      command = "";
      if(strLength != -1) {
        distributeCoordinates(strLength);
        for(int i = 0; i < NUMBER_OF_SLAVES; i++) {
          if(i2cSlaveAvailable[i]) {
            sendToSlave(i2cSlaveAddresses[i], piezoIndex[i], pi[i]);
          }
          // pi[i] = 0;
        }
      }
    }
    else {
      command += c;
    }
  }
}



int parseCommand(String com) {
	if(debug) Serial.println("Entering parser...");

	String subPart; // substring used for trimming
	uint8_t i, piezoPairs, coord, index;
	bool cont; // Flag to continue slicing the command string
	boolean startMarkerSet, byteOne, firstPair, lastPair;
  
	if(debug) {
		Serial.print("Received: "); Serial.println(com);
	}
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
