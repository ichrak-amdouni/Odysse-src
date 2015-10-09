#include <XBee.h>
//https://code.google.com/p/xbee-arduino/wiki/DevelopersGuide
#include <SoftwareSerial.h>
# define BYTE 8
//#define MAX_FRAME_DATA_SIZE 110
//uint8_t Init[3]={'I',4,1};
// le tranceiver xbee s1
XBee xbee = XBee();
//Tx16Request bc = Tx16Request(17, Init, sizeof(Init)); 
//XBeeResponse response = XBeeResponse();
// create reusable response objects for responses we expect to handle
Rx16Response rx16 = Rx16Response();
//Rx64Response rx64 = Rx64Response();

TxStatusResponse txStatus = TxStatusResponse();
// Variable for sending and receiving packets
uint8_t option = 0;
uint8_t data = 0;

void setup()
{
   Serial.begin(38400);
   // Initialiser le transeiver
   xbee.setSerial(Serial);
   //xbee.begin(38400);
   //xbee.send(bc);
   Serial.println("End Setup At Receiver..."); 
}

void loop()
{
	int i ;
    xbee.readPacket();
    //Serial.println("Reading Data..."); 
	if (xbee.getResponse().isAvailable()) 
	{
	  // got something
	  
	  if (xbee.getResponse().getApiId() == RX_16_RESPONSE || 
			xbee.getResponse().getApiId() == RX_64_RESPONSE) 
	  {
		// got a rx packet
		if (xbee.getResponse().getApiId() == RX_16_RESPONSE) 
		{
				xbee.getResponse().getRx16Response(rx16);
				option = rx16.getOption();
				data = rx16.getData(2);
				Serial.println("Received LOL");
				for (int i = 0; i < rx16.getDataLength(); i++) 
				{ 
					Serial.print(rx16.getData(i), BYTE);
				}
				//Serial.print(" Also, data is: ");
				//Serial.print(rx16.getData());
				Serial.print(" From: "),
				Serial.print(rx16.getRemoteAddress16());
				Serial.print(" Rssi: ");
				Serial.print(rx16.getRssi());
	            
		} 
		else
		{
			Serial.print("Unrecognized Address Size");
		}
	  }
	  else if (xbee.getResponse().isError())
			{
				// get the error code
				uint8_t error = xbee.getResponse().getErrorCode();
				if (error == CHECKSUM_FAILURE) Serial.print("Error: checksum");
				else if (error == CHECKSUM_FAILURE) Serial.print("Error: checksum");
				else if (error == PACKET_EXCEEDS_BYTE_ARRAY_LENGTH) Serial.print("Error: PACKET_EXCEEDS_BYTE_ARRAY_LENGTH");
				else if (error == UNEXPECTED_START_BYTE) Serial.print("Error: UNEXPECTED_START_BYTE");
			} 
	}
}
