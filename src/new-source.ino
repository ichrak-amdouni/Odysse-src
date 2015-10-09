#include <SimpleTimer.h>
#include <XBee.h>
#include <SoftwareSerial.h>
#include <SD.h>
#include "XBeeCommands.h"
#include "util.h"
#include "constants.h"
//--------------------------------------
//   #define WITH_CODING
#ifdef WITH_CODING
#include "CodedPacket.c"
uint8_t nextIndexCoded = 0;
#endif
//--------------------------------------
//Read Starting address      
boolean withLog = 0;
boolean should_log = true;
//--------------------------------------

void SendResetCmd();
void SendSizeCmd();
void SendTakePhotoCmd();
void SendReadDataCmd();
void StopTakePhotoCmd();
void FakeSendPhotos();
void ProcessBeacon(uint8_t[], int);  

//--------------------------------------

SimpleTimer t;
// --- Transceiver
XBee xbee = XBee();
Tx16Request levelTx = Tx16Request(0x0000ffff, LEVEL_MSG, sizeof(LEVEL_MSG)); 
Tx16Request beaconTx = Tx16Request(0x0000ffff, BEACON_MSG, sizeof(BEACON_MSG)); 
//--------------------------------------
XBeeResponse response = XBeeResponse();
// create reusable response objects for responses we expect to handle
Rx16Response rx16 = Rx16Response();
//TxStatusResponse txStatus = TxStatusResponse();
// Variable for sending and receiving packets
//uint8_t option = 0;

//--------------------------------------
boolean isCoded = 0;
uint8_t toGenerate = 0;
boolean increment = 1;
long t2 = 0;
uint8_t NB_RCV_REPLY = 0; 
long currentBestScore = 0;
// long idpacket = 0;
uint16_t idpacket = 0;


long tinit = 0;
uint8_t myLevel = INFINITE_LEVEL;
uint8_t myLink = INFINITE_LEVEL;
uint8_t bestTx [2] = {0,0};
uint16_t bestTx16 = 0; 
uint16_t BEACON_MSG_SEQ_NUM = 0;
uint16_t PHOTO_SEQ_NUM = 0;
uint16_t LEVEL_MSG_SEQ_NUM = 0;
uint16_t DATA_MSG_SEQ_NUM = 0;
//--------------------------------------

uint8_t myAddress[2];
uint16_t myAddr16;

//--------------------------------------




//-------------------------------------------------------

#define WITH_CAM
#define FILTER_ADDR
//--------------------------------------
#define xbee_serial Serial1 /*Serial 1: 19 (RX) and 18 (TX)*/

//--------------------------------------

#ifdef WITH_CAM
#include <LSY201.h>
#define camera_serial Serial2 /* Mega2560 Serial 2: 17 (RX) and 16 (TX)*/
LSY201 camera;
const uint8_t photoBufferSize = 32*2;
uint8_t photoBuf[photoBufferSize];   
//void takePhoto();

//--------------------------------------

void setupCamera()
{
	Serial.println("start setup camera...");
	//camera.reset();
	camera.setSerial(camera_serial);
	
	//camera.setBaudRate(19200);
	//camera_serial.begin(19200);
	//camera.setBaudRate(38400);

	camera_serial.begin(38400);
	delay(10000);
	
	//camera.setDebugSerial(Serial);
	
	//camera.setImageSize(camera.Large);
	//camera.reset();
	
	Serial.println("end setup...");
	camera.reset();
	camera.setCompressionRatio(0xFF);
	delay(10000);
}
//--------------------------------------
#ifdef WITH_CODING
void initPhoto()
{
    //codedPacket = 00; 
    nextIndexCoded = 0;
    isCoded = 0;
    for (int i = 0; i < NB_CODED_PACKET; i++)
       for (int j = 0; j < photoBufferSize; j++)
       {
           codedPacket[i][j] = 0;
       }
}

void addCodedPacket(int packetNumber)
{
    uint8_t indexOf[CODE_DENSITY]; // = codedPacketIndexOf[packetNumber];
    
    assignArray(indexOf, codedPacketIndexOf[packetNumber], CODE_DENSITY);
    
    for (int i = 0; i < CODE_DENSITY; i++)
    {
        uint8_t index = indexOf[i];
        xorArray(codedPacket[index], photoBuf, sizeof(photoBuf));
    }
}
#endif

bool getNextPhotoPacket(uint16_t offset)
{
#ifdef WITH_CODING  
    bool morePhotoPacket = camera.readJpegFileContent(offset, photoBuf, sizeof(photoBuf));
    if (!morePhotoPacket)
        isCoded = 1;
    else return morePhotoPacket;
    //Serial.print("is coded becomes True num:");
    Serial.println(nextIndexCoded);
    if (nextIndexCoded < NB_CODED_PACKET)
    {
        assignArray(photoBuf, codedPacket[nextIndexCoded], sizeof(photoBuf));
        nextIndexCoded ++;
        return 1;
    }
    else
    {
        initPhoto();
        Serial.println("Coding Done");
        return 0;
    }

#else
    return camera.readJpegFileContent(offset, photoBuf, sizeof(photoBuf));
#endif
}


//--------------------------------------

void takePhoto()
{
#ifdef NOT_OPTIMIZED
	camera.reset();
	delay(10000);
#endif
	
	Serial.println("Taking picture...");
	camera.takePicture();
    
	Serial.println("Bytes:");
    
	uint16_t offset = 0;
    
    //while (camera.readJpegFileContent(offset, photoBuf, sizeof(photoBuf)))
    while(getNextPhotoPacket(offset))
	{
		for (int i = 0; i < sizeof(photoBuf); i ++)
		{
#ifdef NOT_OPTIMIZED
			Serial.print("@");
			Serial.println(photoBuf[i], HEX);
#endif
		//Serial.print((char)photoBuf[i]);
		}
#ifdef WITH_CODING
        if (!isCoded)
        {
            if (DATA_MSG_SEQ_NUM < MAX_SEQ_NUM){
                addCodedPacket(DATA_MSG_SEQ_NUM+1);
            }else{
                addCodedPacket(1);
            }
        }
#endif
		generatePayloadWithPhoto(photoBuf, sizeof(photoBuf));
		
		if (withLog)
		{
			Serial.print("@@@ PHOTO: ");
			int  i;
			for (i = 0; i < sizeof(photoBuf); i++)
				Serial.print(photoBuf[i], HEX);
			Serial.println();
		}
		if (increment) //last packet sent, should increment offset, otherwise, generate same photo segment 
            offset += sizeof(photoBuf);
	}

	Serial.println("Done.");
	delay(1000);
	camera.stopTakingPictures(); //Otherwise picture won't update
	delay(3000);
#ifdef NOT_OPTIMIZED
	delay(10000);
#endif
}

//--------------------------------------

void generatePayloadWithPhoto(uint8_t photoPart[], int len)
{
	int i;
	idpacket++;
	int pathLength = 0; // this node is the generator
	int size = 9 + pathLength/*MAX_PATH_LENGTH*/+ len; /*header + path + photoPart*/
	uint8_t packet[size];
	packet[0] = 'D';
	packet[1] = size; //sizeof(packet);
	updateMessage(packet, size, 2, myAddress);
#ifdef WITH_CODING
    if (isCoded && DATA_MSG_SEQ_NUM < 0x256 /*799*/ )
    {
        DATA_MSG_SEQ_NUM = DATA_MSG_SEQ_NUM*0x256 - 1; //799;
    }
#endif
	uint8_t seqNumTable[2] = {0,0};
    if (DATA_MSG_SEQ_NUM < MAX_SEQ_NUM){
        DATA_MSG_SEQ_NUM = DATA_MSG_SEQ_NUM + 1;
    }else{
        DATA_MSG_SEQ_NUM = 1;
    }
        
	seqNumTable [1] = DATA_MSG_SEQ_NUM >> 8; //@1
	seqNumTable [0] = DATA_MSG_SEQ_NUM & 0xFF; // @0
	updateMessage(packet, size, 4, seqNumTable);
	
	seqNumTable [1] = PHOTO_SEQ_NUM >> 8; //@1
	seqNumTable [0] = PHOTO_SEQ_NUM & 0xFF; // @0
	updateMessage(packet, size, 6, seqNumTable);
	
	packet[8] = pathLength; //only one address
	
	int index = 9; //4 + MAX_PATH_LENGTH + 1;
	
	for (i = 0; i < len; i++)
		packet[i+index] = photoPart[i];

	if (withLog)
	{
		Serial.print("Preparing photo packet to send: [");
		for (i = 0; i < size/*ARRAYELEMENTCOUNT(paquet)*/; i++)
		{
			Serial.print(packet[i], HEX);
			Serial.print("|");
		}
		Serial.println("]");
	}
   
	if (myLevel == 1)
	{ //source attached to the sink
		if (withLog)
			pSerialPrint(" Level 1 at source, so relay packet to sink");
		tinit = millis();
		//sendPhotoPart(packet, size, tinit, SINK_ADDR);

        sendData(packet, size, tinit, SINK_ADDR);
            
	}
	else
	{
		findRelay(packet, size); 
	}
  //sendPhotoPart(paquet, size, tinit, SINK_ADDR);
}

#endif /*WITH_CAM*/

//---------------------------------------------------------

#ifdef FILTER_ADDR
int const MAX_FILTER_TABLE_SIZE = 20;
//uint16_t filterTable[MAX_FILTER_TABLE_SIZE] = {0x04, 0x05, 0x09, 0x10};
uint16_t filterTable[MAX_FILTER_TABLE_SIZE] = {0x10, 0x01, 0x06};
int sizeFilterTable = ARRAYELEMENTCOUNT(filterTable);
#endif

//---------------------------------------------------------

void generateBeaconMessage()
{
	uint8_t msgType = 'B';
	BEACON_MSG[0] = 'B';
	BEACON_MSG[1] = BEACON_MSG_LENGTH;
	updateMessage(BEACON_MSG, BEACON_MSG_LENGTH, 2, myAddress);
	uint8_t seqNumTable[2] = {0,0};
	
	if (BEACON_MSG_SEQ_NUM < MAX_SEQ_NUM){
		BEACON_MSG_SEQ_NUM = BEACON_MSG_SEQ_NUM + 1;
	}else{
		BEACON_MSG_SEQ_NUM = 1;
	}
	seqNumTable [1] = BEACON_MSG_SEQ_NUM >> 8; //@1
	seqNumTable [0] = BEACON_MSG_SEQ_NUM & 0xFF; // @0
	updateMessage(BEACON_MSG, BEACON_MSG_LENGTH, 4, seqNumTable);
	
	seqNumTable [1] = DATA_MSG_SEQ_NUM >> 8; //@1
	seqNumTable [0] = DATA_MSG_SEQ_NUM & 0xFF; // @0
	updateMessage(BEACON_MSG, BEACON_MSG_LENGTH, 6, seqNumTable);
	
	seqNumTable [1] = PHOTO_SEQ_NUM >> 8; //@1
	seqNumTable [0] = PHOTO_SEQ_NUM & 0xFF; // @0
	updateMessage(BEACON_MSG, BEACON_MSG_LENGTH, 8, seqNumTable);
	
	BEACON_MSG[10] = myLevel;
}

//---------------------------------------------------------

void generateLevelMessage()
{
	LEVEL_MSG[0] = 'L';
	LEVEL_MSG[1] = LEVEL_MSG_LENGTH;
	updateMessage(LEVEL_MSG, LEVEL_MSG_LENGTH, 2, myAddress);
	if (LEVEL_MSG_SEQ_NUM < MAX_SEQ_NUM){
		LEVEL_MSG_SEQ_NUM = LEVEL_MSG_SEQ_NUM + 1;
	}else{
		LEVEL_MSG_SEQ_NUM = 1;
	}
	uint8_t seqNumTable[2] = {0,0};
	seqNumTable [1] = LEVEL_MSG_SEQ_NUM >> 8; //@1
	seqNumTable [0] = LEVEL_MSG_SEQ_NUM & 0xFF; // @0
	updateMessage(LEVEL_MSG, LEVEL_MSG_LENGTH, 4, seqNumTable);
	LEVEL_MSG[6] = myLevel;
}

//---------------------------------------------------------

void initProcess()
{
	int i;
	myAddr16 = getMyAddress16(xbee);
	if (myAddr16 == 0)
	{
		Serial.println("SOURCE-Failed to get Address From Xbee");
		return;
	}
	else
	{
		String toPrint = "[SOURCE] myAddress is ";
		toPrint += myAddr16;
		Serial.print(toPrint);

		Serial.print(" myAddr16 is 0x");
		Serial.println(myAddr16, HEX);
		myAddress[0] = myAddr16 & 0xFF; // @0
		myAddress[1] = (myAddr16 >> 8); // @1
		Serial.print("MY address is ");
		for (i = sizeof(myAddress)-1; i >= 0; i--)
			Serial.print(myAddress[i]);
	}
	pSerialPrint("My Address16 %u\n", myAddr16);
	pSerialPrint("(in Bytes) %d%d\n", myAddress[1], myAddress[0]); 
	LOG(should_log, "\n\n\nMyADDR:%u\n",myAddr16);
	
}

//---------------------------------------------------------

void setup() 
{
	uint8_t msgType;
	long start = 0;
	long t1 = 0;
	uint8_t receivedLevel;
	uint16_t parent;
	uint8_t parentLevel = 0;
	uint8_t in = 0;
	Serial.begin(38400);
	xbee_serial.begin(38400);
   
#ifdef WITH_SD
  // initilisation of SD card PINs
	pinMode(10, OUTPUT);
	// pinMode(10, HIGH);
	pinMode(4, OUTPUT);
	pinMode(4, HIGH);
	if (!SD.begin(4))
	{
		LOG(should_log, "SD Initialization failed!\n");
		//Serial.println("initialization failed!");
		//return;
	}
#endif
	randomSeed(30);
	xbee.setSerial(xbee_serial);
	// xbee.begin(38400);
	
	sendRemoteAtCommand();
	
	initProcess();
  
	t.run();
	while (start == 0)
	{
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
#ifdef FILTER_ADDR
					if(!isfiltered(rx16.getRemoteAddress16(), filterTable, sizeFilterTable))
					{
#endif
						//option = rx16.getOption();
						msgType = getMessageType(rx16);//rx16.getData(0);
						if (msgType == 'L')
						{
							receivedLevel = rx16.getData(6);
							long rssi = rx16.getRssi();
                            LOG_RCV(myAddr16, rx16, (int) 'L', LEVEL_MSG_SEQ_NUM, 0, 0, rssi);
#ifdef WITH_LOG_SD
							logRCV(1, tinit, rx16.getData(1));
#endif
							in = 1;
							if (start == 0)
							{
								if (t1 == 0)
								{ 
									t1 = millis() + LEVEL_WAIT_PERIOD;  // la durée d'attente des messages level des autres voisins
								}
							}
							/*if (myLevel == 0 || myLevel > receivedLevel)
							{
								myLevel = receivedLevel + 1;
								parent = rx16.getRemoteAddress16();
								parentLevel = receivedLevel;
							}*/
                            int receivedLink = distanceFromRssi(rssi, 0);
                            int newLevel = receivedLevel +receivedLink;
                            
                            boolean shouldUpdate = (newLevel < myLevel);                         
                            shouldUpdate |= (newLevel == myLevel
                                                && receivedLink < myLink);
                            if (shouldUpdate)
                            {
                                myLevel = newLevel;
                                myLink = receivedLink;
                                parent = rx16.getRemoteAddress16();
                                parentLevel = receivedLevel;
                            }
						}
#ifdef FILTER_ADDR 	
					} 
					else if (withLog){
						Serial.print("Filtered Address: ");
						Serial.println(rx16.getRemoteAddress16());
					}
#endif
				} 
			} 
		}
		if (t1 < millis() && in == 1)
		{
			start = 1;
		}
	} //end init level phase
		
	if (in == 1)
	{
		LOG_LINKED(should_log, myAddr16, parent, myLevel, parentLevel);
		if (withLog == 1)
		{
			Serial.print("My level is:  ");
			Serial.println(myLevel);
		}
		
		generateLevelMessage();
		tinit = millis(); 
		
#ifdef WITH_LOG_SD    
		long t2 = millis();
#endif   
		xbee.send(levelTx);
		if (withLog)
		{
			Serial.print("Forwarding the Init Msg at Setup and the Ack is: ");
		}
		boolean ack = 1; // no ack for broadcast, getAck(xbee, withLog);
		int size = ARRAYELEMENTCOUNT(LEVEL_MSG);
		LOG_SEND(should_log, myAddr16, BROADCAST_ADDRESS, 'L', size, LEVEL_MSG, LEVEL_MSG_SEQ_NUM, DATA_MSG_SEQ_NUM, PHOTO_SEQ_NUM, ack); // no idpacket here, so put 0
#ifdef WITH_LOG_SD
		long t4 = millis();
		logSENT(1, t2, t4, tinit, 1);
#endif
	} //end in=1

#ifdef WITH_CAM
  //mySerial.begin(38400);
  setupCamera();
#endif
}

//---------------------------------------------------------
void findRelay(uint8_t toSendData[], int toSendSize)
{
	uint8_t seq[2] = {0, 0};
	//tbeacon = millis();
	//CP = millis() + CP;
	long F = 20000; //score of router nodes
	long tStartReply;
	long tEndReply;
	long tBeaconStart = millis(); 
	long tBeaconEnd = millis() + MAX_BEACON_PERIOD;
	int nbSentBeacon = 0;
	
	uint8_t msgType;
	//while (NB < N && tbeacon <= CP)
	while(NB_RCV_REPLY < MAX_RCV_REPLY_NB && tBeaconStart <= tBeaconEnd)
	{
		//Serial.print(myLevel);
		//Serial.print(DATA_MSG_SEQ_NUM);
		//Serial.print(PHOTO_SEQ_NUM);
		
		generateBeaconMessage();
		
		delay(XBEE_DELAY_BEFORE_SEND);
#ifdef WITH_LOG_SD   
		long t2 = millis();
#endif
		xbee.send(beaconTx);
		if (withLog == 1){
			Serial.print (" Beacon Sent ");
			printBeacon();
			Serial.print(" at ProcessBeacon and Ack is: ");                          
		}
		boolean ack = 1; //no ack for the broadcastXXXX getAck(xbee, withLog);
		LOG_SEND(should_log, myAddr16, BROADCAST_ADDRESS, 'B', ARRAYELEMENTCOUNT(BEACON_MSG), BEACON_MSG, BEACON_MSG_SEQ_NUM, DATA_MSG_SEQ_NUM, PHOTO_SEQ_NUM, ack);
		if (withLog == 1) Serial.println("");
#ifdef WITH_LOG_SD
		long t4 = millis(); 
		logSENT(2, t2, t4, tinit, 1);
#endif    
		//tint = millis();
		//RI = millis() + RI;  // period to wait for beacon reply
		tStartReply = millis();
		tEndReply = millis() + WAIT_REPLY_PERIOD;
		// wait for response
		//while (tint < RI && NB < N)
		while(tStartReply < tEndReply && NB_RCV_REPLY < MAX_RCV_REPLY_NB)
		{
			xbee.readPacket();
			if (xbee.getResponse().isAvailable()) {
			// got something
				if (xbee.getResponse().getApiId() == RX_16_RESPONSE || xbee.getResponse().getApiId() == RX_64_RESPONSE) 
				{
					// got a rx packet
					if (xbee.getResponse().getApiId() == RX_16_RESPONSE) 
					{
						xbee.getResponse().getRx16Response(rx16);
				
#ifdef FILTER_ADDR
						if(!isfiltered(rx16.getRemoteAddress16(), filterTable, sizeFilterTable))
						{
#endif
							//option = rx16.getOption();
							msgType = getMessageType(rx16);
							if (withLog == 1)
							{
								Serial.print("Received (while wait reply) ");
								printRx();
								Serial.print(" of type: ");
							}
							
							if (msgType == 'R')
							{
								Serial.print("received reply");
								uint16_t receivedSeqNum = getTwoBytesFromRx(rx16, 4); 
								uint16_t receivedDataSeqNum = getTwoBytesFromRx(rx16, 6);//getSeqNumFromRx(rx16, 6);
								uint16_t receivedDataLotSeqNum = getTwoBytesFromRx(rx16, 8);//getDataLotSeqNumFromRx(rx16, 8);
								if (receivedDataSeqNum == DATA_MSG_SEQ_NUM && receivedDataLotSeqNum == PHOTO_SEQ_NUM)
								{
									LOG_RCV(myAddr16, rx16, 'R', receivedSeqNum, receivedDataSeqNum,  receivedDataLotSeqNum, rx16.getRssi());
									if (withLog) Serial.println("Reply ");
									NB_RCV_REPLY ++;
									//F = 0.25*MSR + 0.5 * level + 0.25/RSSI
									//F = 0.25*rx16.getData(11)+0.5*rx16.getData(10)+0.25*(rx16.getData(12)*(1/100));
                                    F = 0.25*rx16.getData(11)+0.5*rx16.getData(10)+0.25*rx16.getData(12);
                                    
#ifdef WITH_LOG_SD
									logRCV(3, tinit, rx16.getRemoteAddress16());
#endif
									
									if (bestTx16 == 0  || F < currentBestScore)
									{
										currentBestScore = F;
										bestTx[0] = rx16.getData(3/*2*/); 
										bestTx[1] = rx16.getData(2/*3*/);
										bestTx16 = (((uint16_t)bestTx[1]) << 8) | bestTx[0];
										//bestTx16 = getLong(bestTx); 
										//Serial.print("Best tx :");
										//Serial.print(bestTx16, HEX);
									}
									/*
									if (NB_RCV_REPLY == 1)
									{
										currentBestScore = F;
										bestTx[0] = rx16.getData(3); 
										bestTx[1] = rx16.getData(2);
										bestTx16 = (((uint16_t)bestTx[1]) << 8) | bestTx[0];
										//bestTx16 = getLong(bestTx); 
										Serial.print("Best tx :");
										Serial.print(bestTx16, HEX);
									}
									if (F < currentBestScore)
									{
										currentBestScore = F;
										bestTx[0] = rx16.getData(3); 
										bestTx[1] = rx16.getData(2);
										bestTx16 = (((uint16_t)bestTx[1]) << 8) | bestTx[0]; 
									}*/
								}
							}
							else if (withLog){
								Serial.print("Unkown Format: ");
								Serial.println(msgType);
							}
#ifdef FILTER_ADDR    
						} 
						else if (withLog){
								Serial.print("Filtered Address: ");
								Serial.println(rx16.getRemoteAddress16());
						}
#endif
					}
				}
			}
			//tint=millis();
			tStartReply = millis();
		}
		tBeaconStart = millis(); //tbeacon = millis();
		//RI = 200;
	}
	//CP = 10000;
	//NB = 0;
	NB_RCV_REPLY = 0;
	
	if (bestTx16 != 0)
	{
		

        sendData(toSendData, toSendSize, tinit, bestTx16);

        bestTx16 = 0;
		bestTx[0] = 0;
		bestTx[1] = 0;
        increment = 1;
	}
	else {
        increment == 0;
        Serial.println("no relay");
    /*No neigh selected*/
    if (withLog) Serial.println("No Response");
	}
}

//---------------------------------------------------------

void sendData(uint8_t dataMsg[], int len, long tinit, uint16_t destination){
#ifdef WITH_LOG_SD
	long t4 = millis();
	logSENT(4, t2, t4, tinit, 1);
#endif     
	if (withLog)
		Serial.println("Sending Data:");
	Tx16Request dataTx = Tx16Request(destination, dataMsg, len);
#ifndef WITH_CAM
	delay(20*3); //XXX hack to let the source delay her data sending, otherwise the router would not receive, to be removed when select is removed
#else
	delay(XBEE_DELAY_BEFORE_SEND); //XXX
#endif 	
	
#ifdef WITH_LOG_SD
	long t5 = millis();
#endif
	delay(10);
    
#ifdef WITH_CODING
#ifdef WITH_LOST
    float r = random(1, 10)/10.0;
    Serial.print("rrrrrr ");
    Serial.println(r);
    if (r <= PDR)
#endif
#endif        
        xbee.send(dataTx);


 
    boolean ack = getAck(xbee, withLog);
	LOG_SEND(should_log, myAddr16, destination, 'D', len, dataMsg, DATA_MSG_SEQ_NUM, DATA_MSG_SEQ_NUM, PHOTO_SEQ_NUM, ack);
    /*if(ack == 0)
        increment = 0; // prevent source from generating new packet*/
#ifdef WITH_LOG_SD
	logSENT(idpacket, t5, t4, tinit, bestTx16);
#endif
}
//---------------------------------------------------------
#if 0
#ifdef WITH_CAM
void sendPhotoPart(uint8_t data[], int len, long tinit, uint16_t destination){
#ifdef WITH_LOG_SD 
	long t4 = millis();
	logSENT(4, t2, t4, tinit, 1);
#endif     
	if (withLog) 
		Serial.println("Sending Data:");

	Tx16Request dataTx = Tx16Request(destination, data, len);

	delay(20); //XXX 
#ifdef WITH_LOG_SD
	long t5 = millis();
#endif
	xbee.send(dataTx);
	bool ack = getAck(xbee, withLog);
	LOG_SEND(should_log, myAddr16, 'D', len, data, DATA_MSG_SEQ_NUM, DATA_MSG_SEQ_NUM, PHOTO_SEQ_NUM, ack);
#ifdef WITH_LOG_SD
	logSENT(idpacket, t5, t4, tinit, bestTx16);
#endif
}
#endif
#endif
//---------------------------------------------------------
// changer le canal de transmission
void sendRemoteAtCommand() {
	Serial.println("Sending command to the XBee");
	//uint8_t wrCmd[] = {'I', 'D'};
	uint8_t idCmd[] = {'I','D'};
	uint8_t idValue[1] = {2};//
	//idValue[0] = 2;
	// Create a AT request with the ID command
	AtCommandRequest AtRequest = AtCommandRequest(idCmd, idValue, sizeof(idValue));
	
	AtRequest.setCommandValue(idValue);
	//AtCommandRequest AtRequest1 = AtCommandRequest(wrCmd);
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
					Serial.print("Command [");
					Serial.print(AtResponse.getCommand()[0]);
					Serial.print(AtResponse.getCommand()[1]);
					Serial.println("] was successful!");
					ok = 1;

					if (AtResponse.getValueLength() > 0) 
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
				else {
					Serial.print("Command returned error code: ");
					Serial.println(AtResponse.getStatus(), HEX);
				}
			}
			else {
				Serial.print("Expected Remote AT response but got ");
				Serial.print(xbee.getResponse().getApiId(), HEX);
			}    
		} 
		else if (xbee.getResponse().isError()){
			Serial.print("Error reading packet.  Error code: ");  
			Serial.println(xbee.getResponse().getErrorCode());
		}
		else {
			Serial.print("No response from radio");  
		}
	}
}
//---------------------------------------------------------
void printBeacon()
{
	int i;
	int nb = ARRAYELEMENTCOUNT(BEACON_MSG);
	Serial.print("(size:");
	Serial.print(nb);
	Serial.print("): ");
	Serial.print("[");
	for (i = 0; i < nb; i++){
		Serial.print(BEACON_MSG[i], DEC);
		Serial.print("|");
	}
	Serial.print("]");
}

void printRx()
{
	int i;
	Serial.print("(size:");
	Serial.print( rx16.getDataLength());
	Serial.print(")");
	Serial.print("[");
	for (i = 0; i <  rx16.getDataLength() ; i++){
		Serial.print(rx16.getData(i));
		Serial.print("|");
	}
	Serial.print("]");
}
//---------------------------------------------------------
#ifndef WITH_CAM

void FakeSendPhotos()
{
	int i;
	idpacket++;
	uint8_t dataMsg[MAX_DATA_SIZE];
	dataMsg[0] = 'D';
	dataMsg[1] = sizeof(DATA_MSG);

	updateMessage(dataMsg, MAX_DATA_SIZE, 2, myAddress);
	
	uint8_t seqNumTable[2] = {0,0};
	if (DATA_MSG_SEQ_NUM < MAX_SEQ_NUM){
		DATA_MSG_SEQ_NUM = DATA_MSG_SEQ_NUM + 1;
	}else{
		DATA_MSG_SEQ_NUM = 1;
	}
	seqNumTable [1] = DATA_MSG_SEQ_NUM >> 8; //@1
	seqNumTable [0] = DATA_MSG_SEQ_NUM & 0xFF; // @0
	updateMessage(dataMsg, MAX_DATA_SIZE, 4, seqNumTable);
	
	seqNumTable [1] = PHOTO_SEQ_NUM >> 8; //@1
	seqNumTable [0] = PHOTO_SEQ_NUM & 0xFF; // @0
	updateMessage(dataMsg, MAX_DATA_SIZE, 6, seqNumTable);
	
	int pathLength = 0; // this node is the generator
	dataMsg[8] = pathLength; //only one address
	
	int index = 9; //4 + MAX_PATH_LENGTH + 1;
	
	for (i = index; i < MAX_DATA_SIZE; i++)
		dataMsg[i] = 1; 
	
	if (withLog)
	{
		Serial.print("Preparing dataMsg to send: [");
		for (i = 0; i < ARRAYELEMENTCOUNT(DATA_MSG); i++)
		{
			Serial.print(DATA_MSG[i]);
			Serial.print("|");
		}
		Serial.println("]");
	}
	int size = ARRAYELEMENTCOUNT(dataMsg); 
	if (myLevel == 1){ //source attached to the sink
		pSerialPrint(" Level 1 at source, so relay packet to sink");
		tinit = millis();
		sendData(dataMsg, size, tinit, SINK_ADDR);//sendData(tinit, SINK_ADDR);
	}
	else
		findRelay(dataMsg, size);
}

#endif /*no WITH_CAM*/
//------------------------------------------------------------------
void loop()
{

#ifdef WITH_CAM
	if (toGenerate < 5)
#else
	if (toGenerate < 20)
#endif
	{
		if (PHOTO_SEQ_NUM < MAX_SEQ_NUM){
			PHOTO_SEQ_NUM = PHOTO_SEQ_NUM + 1;
		}else{
			PHOTO_SEQ_NUM = 1;
		}
		DATA_MSG_SEQ_NUM = 0;
#ifdef WITH_CAM
		if (withLog == 1){
			Serial.print("Send Photo Number : ");
			Serial.print(toGenerate);
		}
		//SendPhotos();
		takePhoto();
#else
		FakeSendPhotos();
#endif
		if (withLog == 1)
		{
			Serial.println("Ready to Send one Photo ");
		}
		toGenerate++;
	}
	if (withLog == 1)
	{
		Serial.println ("+");
		delay(10000);
	}
}

//---------------------------------------------------------
