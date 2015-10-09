/*
  SD card file dump
 
 This example shows how to read a file from the SD card using the
 SD library and send it over the serial port.
 	
 The circuit:
 * SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 4
 
 created  22 December 2010
 by Limor Fried
 modified 9 Apr 2012
 by Tom Igoe
 
 This example code is in the public domain.
 	 
 */

#include <SD.h>

// On the Ethernet Shield, CS is pin 4. Note that even if it's not
// used as the CS pin, the hardware CS pin (10 on most Arduino boards,
// 53 on the Mega) must be left as an output or the SD library
// functions will not work.
const int chipSelect = 4;
File root;

void printDirectory(File dir, int numTabs) {
   while(true) {
     
     File entry =  dir.openNextFile();
     if (! entry) {
       // no more files
       //Serial.println("**nomorefiles**");
       break;
     }
     for (uint8_t i=0; i<numTabs; i++) {
       Serial.print('\t');
     }
     //Serial.println("Another Directory: ");
     
     if (entry.isDirectory()) {
       Serial.print(entry.name());
       Serial.println("/");
       printDirectory(entry, numTabs+1);
     } else {
       // files have sizes, directories do not
       //Serial.println("Another File: ");
       Serial.print(entry.name());
       Serial.print("\t\t");
       Serial.println(entry.size(), DEC);
     }
     entry.close();
   }
}


void setup()
{
 // Open serial communications and wait for port to open:
  Serial.begin(9600);
  /* while (!Serial) {
	   Serial.println("Waiting for Serial Port to connect");
	   delay(50000);
    // wait for serial port to connect. Needed for Leonardo only
  }*/


  Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(4, HIGH);
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");
  
	 // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  
  //File dataFile = SD.open("datalog.txt");
  File dataFile = SD.open("last.txt", FILE_WRITE);
  dataFile.write("First line..");
  dataFile.write("Second line..");
 
  // if the file is available, write to it:
  if (dataFile) {
	  if (dataFile.available())
		Serial.println("File available ");
	  else Serial.println("File unavailable ");
    while (dataFile.available()) {
	  Serial.println("Reading from File:");
      Serial.write(dataFile.read());
    }
    dataFile.close();
  }
  
  dataFile = SD.open("last.txt", FILE_READ);
  // if the file is available, write to it:
  if (dataFile) {
	  if (dataFile.available())
		Serial.println("File1 available ");
	  else Serial.println("File1 unavailable ");
    while (dataFile.available()) {
	  Serial.println("Reading from File:");
      Serial.write(dataFile.read());
    }
    dataFile.close();
  }
  
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening datalog.txt");
  } 
  
  Serial.println("Listing Directory");
 
  root = SD.open("/");  
  printDirectory(root, 0);
  
  Serial.println("done!");
  
  Serial.println("END SETUP ..");
}

void loop()
{
}
