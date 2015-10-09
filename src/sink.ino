#include <SimpleTimer.h>
#include <XBee.h>
#include <SD.h>
#include "XBeeCommands.h"
#include "util.h"

//-----------------------------------------------

void ModeSleep();

SimpleTimer t;
// uint8_t Init[3+1] = {'I',0,0,0};
// le transeiver xbee s1
XBee xbee = XBee();
Tx16Request levelTx = Tx16Request(0x0000ffff, LEVEL_MSG, sizeof(LEVEL_MSG)); 
XBeeResponse response = XBeeResponse();
// create reusable response objects for responses we expect to handle
Rx16Response rx16 = Rx16Response();
//TxStatusResponse txStatus = TxStatusResponse();
// Variable for sending and receiving packets
uint8_t option = 0;
boolean withLog = 0;
uint8_t myAddress[2];
uint16_t myAddr16;
boolean should_log = true;
uint16_t LEVEL_MSG_SEQ_NUM = 0;
#define WITH_LARGE_LOG
#ifdef WITH_LOG_SD
File RXD;
#endif
//-----------------------------------------------
long tinit = 0;

int totDataPckts = 0;


//-----------------------------------------------
void generateLevelMessage()
{
	
	LEVEL_MSG[0] = 'L';
	LEVEL_MSG[1] = LEVEL_MSG_LENGTH;
	
	updateMessage(LEVEL_MSG, LEVEL_MSG_LENGTH, 2, myAddress);

	uint8_t seqNumTable[2] = {0,0};
	if (LEVEL_MSG_SEQ_NUM < MAX_SEQ_NUM){
		LEVEL_MSG_SEQ_NUM = LEVEL_MSG_SEQ_NUM + 1;
	}else{
		LEVEL_MSG_SEQ_NUM = 1;
	}
	seqNumTable [1] = LEVEL_MSG_SEQ_NUM >> 8; //@1
	seqNumTable [0] = LEVEL_MSG_SEQ_NUM & 0xFF; // @0
	updateMessage(LEVEL_MSG, LEVEL_MSG_LENGTH, 4, seqNumTable);
	
	LEVEL_MSG[6] = 0; // level
}

void initProcess()
{
	int i;
	myAddr16 = getMyAddress16(xbee);

	if (myAddr16 == 0)
	{
		Serial.println("SINK-Failed to get Address From Xbee");
		return;
	}
	else
	{
		Serial.print("[SINK] myAddr16 is ");
		Serial.println(myAddr16, HEX);
		myAddress[0] = myAddr16 & 0xFF; // @0
		myAddress[1] = (myAddr16 >> 8); // @1
		Serial.print("MY address is ");
		for (i = sizeof(myAddress)-1; i >= 0; i--)
			Serial.print(myAddress[i]);
	}
	Serial.print("\n");
	generateLevelMessage();
}

//-----------------------------------------------

void setup()
{
	Serial.begin(38400);
	Serial1.begin(38400);
#ifdef WITH_SD
	pinMode(10, OUTPUT);
	pinMode(10, HIGH);
	pinMode(4, OUTPUT);
	pinMode(4, HIGH);
	if (!SD.begin(4)) 
	{
		Serial.println("SD Initialization Failed!");
		//return;
	}
#endif
	// Init the tranceiver
	xbee.setSerial(Serial1);
	//  xbee.begin(38400);
	
	sendRemoteAtCommand();
		
	t.run();
	initProcess();
	delay(3000);// XXX
	
	xbee.send(levelTx);
	//  delay (2000);
	//}
   
	tinit = millis();
	if (withLog == 1)
	{
		Serial.println ("\n Level Init in Progress (send Level Msg) at Setup and ack is...");
	}
	boolean ack = 1; //getAck(xbee, withLog);
	int size = ARRAYELEMENTCOUNT(LEVEL_MSG);
#ifdef WITH_LARGE_LOG	
	LOG_SEND(should_log, myAddr16, 0 /*BROADCAST_ADDRESS*/, 'L', size, LEVEL_MSG, LEVEL_MSG_SEQ_NUM, 0, 0, ack);
#endif
}

//-----------------------------------------------

void loop()
{
	uint8_t msgType;
	xbee.readPacket();
	if (xbee.getResponse().isAvailable()) 
	{
		// got something

		if (xbee.getResponse().getApiId() == RX_16_RESPONSE || xbee.getResponse().getApiId() == RX_64_RESPONSE) 
		{
			// got a rx packet

			if (xbee.getResponse().getApiId() == RX_16_RESPONSE) 
			{			
				xbee.getResponse().getRx16Response(rx16);
				option = rx16.getOption();
				msgType = getMessageType(rx16); //rx16.getData(2+1); //XXX fixed: getData(2)
				if (withLog == 1) 
				{
					Serial.print("Receiving Something at loop: ");
					printRx();
					Serial.print(" of type: ");
				}
				if (msgType == 'D')
				{
					//uint16_t receivedSeqNum = getTwoBytesFromRx(rx16, 4); 
					uint16_t receivedDataSeqNum = getTwoBytesFromRx(rx16, 4);
					uint16_t receivedPhotoSeqNum = getTwoBytesFromRx(rx16, 6);
					
					//uint16_t seq16 = getSeqNumFromRx(rx16);
					//uint16_t dataLotSeqNum = getDataLotSeqNumFromRx(rx16);
#ifdef WITH_LARGE_LOG	  
					LOG_RCV(myAddr16, rx16, 'D', receivedDataSeqNum, receivedDataSeqNum, receivedPhotoSeqNum, rx16.getRssi()); 
#endif
					totDataPckts ++; 
					int indexData = 19;
					//Serial.print('@');
					/*while(indexData < rx16.getDataLength())
					{
						//Serial.print((char)rx16.getData(indexData));
						indexData++;
					}*/
					
					Serial.print("Data: (total = ");
					Serial.print(totDataPckts);
					Serial.println(")");
#ifdef WITH_LOG_SD
					// enregistrer le paquet reçu
					RXD = SD.open("receive.txt", FILE_WRITE);
					if (RXD) 
					{
						RXD.print(rx16.getData(1));
						RXD.print(" ");
						RXD.print(millis()-tinit);
						RXD.print(" ");
						RXD.println("");
						// close the file:
						RXD.close();
					}
#endif
				}
				else if (msgType == 'B') 
				{
#ifdef WITH_LARGE_LOG	
					//LOG_RCV(myAddr16, rx16, 'B', rx16.getData(1)); // no need to log beacon reception at sink
#endif
					if (withLog == 1)
					{
								Serial.print(" Beacon from : ");
								Serial.println(rx16.getRemoteAddress16()/*rx16.getData(1)*/);
					}
				}
				else if (withLog){
						Serial.print("Unkown format: ");
						Serial.println(msgType);
				}
			} 
		} 
		else {
		}
	}  
}

//-----------------------------------------------

void sendRemoteAtCommand() {
	//  Serial.println("Sending command to the XBee");
	// fixer le PID = 2 pour pouvoir communiquer avec tous les noeuds
	// variables pour changer le canal de transmission
	uint8_t idCmd[] = {'I','D'};
	uint8_t idValue[] = {2};
	//idValue[0] = 2;
	//AtRequest.setCommandValue(idValue);
	// Create a AT request with the ID command
	AtCommandRequest AtRequest = AtCommandRequest(idCmd, idValue, sizeof(idValue));
	// Create a Remote AT response object
	AtCommandResponse AtResponse = AtCommandResponse();

	uint8_t ok =0;
	while ( ok == 0) 
	{
	// send the command
		xbee.send(AtRequest);

		// wait up to 5 seconds for the status response
		if (xbee.readPacket(500)) 
		{
			// got a response!
			// should be an AT command response
			if (xbee.getResponse().getApiId() == AT_COMMAND_RESPONSE) 
			{
				xbee.getResponse().getAtCommandResponse(AtResponse);
				if (AtResponse.isOk()) 
				{
					if (withLog)
					{
						Serial.print("Command [");
						Serial.print(AtResponse.getCommand()[0]);
						Serial.print(AtResponse.getCommand()[1]);
						Serial.println("] was successful!");
					}
					
					ok = 1;

					if (AtResponse.getValueLength() > 0) 
					{
						if (withLog)
						{
							Serial.print("Command value length is ");
							Serial.println(AtResponse.getValueLength(), DEC);
							Serial.print("Command value: ");
							for (int i = 0; i < AtResponse.getValueLength(); i++) 
							{
								Serial.print(AtResponse.getValue()[i], HEX);
								Serial.print(" ");
							}
							Serial.println("");
						}
					}
				} 
				else 
				{ 
					if (withLog)
					{
						Serial.print("Command returned error code: ");
						Serial.println(AtResponse.getStatus(), HEX);
					}
				}
			} 
			else 
			{
				if (withLog)
				{
					Serial.print("Expected Remote AT response but got ");
					Serial.print(xbee.getResponse().getApiId(), HEX);
				}
			}
		} 
		else 
		{ 
			if (xbee.getResponse().isError()) 
			{
				if (withLog)
				{
					Serial.print("Error reading packet.  Error code: ");  
					Serial.println(xbee.getResponse().getErrorCode());
				}
			} 
			else 
			{
				if (withLog) 
					Serial.print("No response from radio");  
			}
		}
	}
}

//-----------------------------------------------

void printRx()
{
	int i;
	Serial.print("(size:");
	Serial.print( rx16.getDataLength());
	Serial.print(")");
	Serial.print("[");
	for (i = 0; i < rx16.getDataLength(); i++)
	{
		Serial.print(rx16.getData(i));
		Serial.print("|");
	}
	Serial.println("]");
}