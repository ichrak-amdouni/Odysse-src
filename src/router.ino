#include <SimpleTimer.h> 
#include <XBee.h>  
#include <SoftwareSerial.h> 
#include <assert.h>
#include "XBeeCommands.h"
#include "util.h"
#include "constants.h"

//-----------------------------------------------
#define WITH_REPEAT
//-----------------------------------------------
SimpleTimer t;

XBee xbee = XBee();
Tx16Request levelTx = Tx16Request(0x0000ffff, LEVEL_MSG, sizeof(LEVEL_MSG)); 
Tx16Request beaconTx = Tx16Request(0x0000ffff, BEACON_MSG, sizeof(BEACON_MSG)); 
XBeeResponse response = XBeeResponse();
// create reusable response objects for responses we expect to handle
Rx16Response rx16 = Rx16Response();
//Rx64Response rx64 = Rx64Response();
TxStatusResponse txStatus = TxStatusResponse();
// Variable for sending and receiving packets
uint8_t option = 0;
uint8_t data = 0;
#ifdef WITH_REPEAT
int retry = 0;
int retryData = 0;
#endif
//-----------------------------------------------

uint8_t bestTx [2] = {0,0};
uint16_t bestTx16 = 0; 
uint8_t myAddress[2];
uint16_t myAddr16;

//-----------------------------------------------

boolean withLog = 0;
boolean should_log = true;

//-----------------------------------------------
/*typedef struct{
	uint16_t SRC_ADDRESS;
	uint16_t SRC_PHOTO_SEQ_NUM;
	uint16_t SRC_DATA_MSG_SEQ_NUM;
} SOURCE;
uint8_t const MAX_SRC_NB = 4;
SOURCE SRC_TAB[MAX_SRC_NB];
*/
//-----------------------------------------------

int nb = 1;
long currentBestScore = 0;
long t1 = 0;
long t2 = 0;
//long idpacket;
uint16_t idpacket = 0;
boolean increment = 1; 
long tbeacon = 0;
long tinit = 0;
long tsleep = 0;    
long tactif = 0;
uint8_t myLevel = INFINITE_LEVEL;
uint8_t myLink = INFINITE_LEVEL;
uint8_t in = 0;
uint8_t hasSent = 0;
long TOT_SENT_RCV = 0; 
uint8_t NB_RCV_REPLY = 0; 
//-----------------------------------------------

#define FILTER_ADDR
#ifdef FILTER_ADDR
int const MAX_FILTER_TABLE_SIZE = 15;
uint16_t filterTable[MAX_FILTER_TABLE_SIZE];
int sizeFilterTable = ARRAYELEMENTCOUNT(filterTable);
//#define FOUR-HOP
#define THREE-HOP
//#define TWO-HOP
#endif
AtCommandRequest atRequest = AtCommandRequest(); //XXX 
AtCommandResponse atResponse = AtCommandResponse();

//-----------------------------------------------

uint16_t BEACON_MSG_SEQ_NUM = 0;
uint16_t REPLY_MSG_SEQ_NUM = 0;
uint16_t PHOTO_SEQ_NUM = 0;
uint16_t LEVEL_MSG_SEQ_NUM = 0;
uint16_t DATA_MSG_SEQ_NUM = 0;

//-----------------------------------------------

#ifdef FILTER_ADDR
void initFilterTable(bool atStart)
{
	for (int i = 0; i < ARRAYELEMENTCOUNT(filterTable); i++) 
		filterTable [i] = 0;
  
#ifdef FOUR-HOP  //Sink(@1)--Router(@2)--Router(@4)--Router(@6)--Source(@5)
	switch (myAddr16)
	{
		case 0x02: 
			filterTable[0] = 0x06; //filterTable[1] = 0x04; 
			filterTable[1] = 0x05; 
			if (atStart)
				filterTable[2] = 0x04; 
			break;
		case 0x04: /*filterTable[0] = 0x06;*/ 
			filterTable[0] = 0x01;
			//if (atStart)
			filterTable[1] = 0x05;
			break;
		case 0x06: 
			filterTable[0] = 0x02; 
			filterTable[1] = 0x01; 
			if (atStart)
				filterTable[2] = 0x05;
			break;
	//case 0x05: filterTable[0] = 0x01; filterTable[1] = 0x02; filterTable[2] = 0x04; break;
	default: break;
	}
#endif
#ifdef THREE-HOP  //Sink(@5)--Router(@4)--Router(@3)--Source(@2) OR //Sink(@10)--Router(@9)--Router(@8)--Source(@6)
	switch (myAddr16){
		case 0x02: 
			filterTable[0] = 0x04; 
			if (atStart)
				filterTable[1] = 0x03;
			break;	
		case 0x05: 
            filterTable[0] = 0x10; 
            if (atStart)
                filterTable[1] = 0x02;
            break;  
        case 0x06: 
            filterTable[0] = 0x02; 
            break;  
        case 0x03: 
			filterTable[0] = 0x01; 
			if (atStart)
				filterTable[1] = 0x04;
			break;	
		case 0x04: 
			filterTable[0] = 0x02;
			if (atStart)
				filterTable[1] = 0x03; 
			break;
		
		case 0x08:
			filterTable[0] = 0x06;
			//if (atStart)
			//	filterTable[1] = 0x06;
			break;
		case 0x09: 
			filterTable[0] = 0x06;
			if (atStart)
				filterTable[1] = 0x08;
			break;
		case 0x10: 
			filterTable[0] = 0x02;
			if (atStart)
				filterTable[1] = 0x08; 
			break;
/*		case 0x08: 
			filterTable[0] = 0x03;
			if (atStart)
				filterTable[1] = 0x04; 
			break;*/
		default: break;
	}
#endif
#ifdef TWO-HOP //Sink(@5)--Router(@3)--Source(@2)
	switch (myAddr16){ //not really useful
		case 0x03: 
			//filterTable[0] = 0x02; filterTable[1] = 0x04; 
			if (atStart)
				filterTable[2] = 0x02;
			break;
		default: break;
	}
#endif
}
#endif

//-----------------------------------------------

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

//-----------------------------------------------

void generateBeaconMessage(uint16_t dataMsgSeqNum, uint16_t photoSeqNum)
{
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
	updateMessageWithLong(BEACON_MSG, BEACON_MSG_LENGTH, 6, dataMsgSeqNum);
	updateMessageWithLong(BEACON_MSG, BEACON_MSG_LENGTH, 8, photoSeqNum);
	BEACON_MSG[10] = myLevel; 
}

//-----------------------------------------------
void generateBeaconReplyMessage(uint16_t dataMsgSeqNum, uint16_t photoSeqNum, long rssi)
{
	REPLY_MSG[0] = 'R';
	REPLY_MSG[1] = REPLY_MSG_LENGTH;
	updateMessage(REPLY_MSG, REPLY_MSG_LENGTH, 2, myAddress);
	
	uint8_t seqNumTable[2] = {0,0};
	if (REPLY_MSG_SEQ_NUM < MAX_SEQ_NUM){
		REPLY_MSG_SEQ_NUM = REPLY_MSG_SEQ_NUM + 1;
	}else{
		REPLY_MSG_SEQ_NUM = 1;
	}
	updateMessageWithLong(REPLY_MSG, REPLY_MSG_LENGTH, 4, REPLY_MSG_SEQ_NUM);
	updateMessageWithLong(REPLY_MSG, REPLY_MSG_LENGTH, 6, dataMsgSeqNum);
	updateMessageWithLong(REPLY_MSG, REPLY_MSG_LENGTH, 8, photoSeqNum);
	
	REPLY_MSG[10] = myLevel;
	REPLY_MSG[11] = TOT_SENT_RCV;
	REPLY_MSG[12] = rssi;
	
}

//-----------------------------------------------

void processBeaconReplyMessage(Rx16Response rx16, uint16_t dataMsgSeqNum, uint16_t photoSeqNum)
{
	/*uint16_t seq16 = getTwoBytesFromRx(rx16, ); // getSeqNumFromRx(rx16);
	uint16_t receivedPhotoSeqNum = getTwoBytesFromRx(rx16, ) ; //getSeqNumFromRx(rx16);
	if (seq16 == BEACON_MSG_SEQ_NUM && receivedPhotoSeqNum == photoSeqNum) //XXX! recevoir une réponse pour le beacon envoyé
	{*/
	long F = 0;
	NB_RCV_REPLY++; //XXX
	if (withLog)
        Serial.print("Response received from: ");
	Serial.println(rx16.getRemoteAddress16());
#ifdef WITH_LOG_SD
	logRCV(3, tinit, rx16.getData(1));
#endif
	F = 0.25 * rx16.getData(11) + 0.25* rx16.getData(12) + 0.5 * rx16.getData(10);
	//F = 0.25*(1/((rx16.getData(2)*100)/2400))+0.25*rx16.getData(3)+0.5*(1/((rx16.getData(4)*100)/7));// XXX
	
	if (bestTx16 ==  0 || F < currentBestScore )
	{
		currentBestScore = F;// on cherche à maximiser F XXX
		bestTx[0] = rx16.getData(3);
		bestTx[1] = rx16.getData(2);
		bestTx16 = (((uint16_t)bestTx[1]) << 8) | bestTx[0];
	}
	/*
	if (NB_RCV_REPLY == 1){ 
		currentBestScore = F;
		bestTx[0] = rx16.getData(3);
		bestTx[1] = rx16.getData(2);
		bestTx16 = (((uint16_t)bestTx[1]) << 8) | bestTx[0];
	}
	if (F > currentBestScore){  
		sauv = F;
		bestTx[0] = rx16.getData(3); //rx16.getData(1); 
		bestTx[1] = rx16.getData(2); //rx16.getData(2);
		bestTx16 = (((uint16_t)bestTx[1]) << 8) | bestTx[0]; //getLong(bestTx);
	}*/
	if (withLog){
        Serial.print("best: ");
        Serial.println(bestTx16);
    }
	//}
}

//-----------------------------------------------

void initProcess()
{
	int i;
	myAddr16 = getMyAddress16(xbee);
	if (myAddr16 == 0){
		Serial.println("ROUTER-Failed to get Address From Xbee");
		return;
	}
	else
	{
		Serial.print("[ROUTER] myAddr16 is ");
		Serial.println(myAddr16, HEX);
		myAddress[0] = myAddr16 & 0xFF; // @0
		myAddress[1] = (myAddr16 >> 8); // @1
		Serial.print("MY address is ");
		for (i = sizeof(myAddress)-1; i >= 0; i--)
			Serial.print(myAddress[i]);
	}
	Serial.print("\n");
}

//-----------------------------------------------

void setup()
{
	uint8_t start = 0;
	uint8_t msgType;
	uint16_t parent;
	uint8_t parentLevel = 0;
	if (withLog)
		Serial.println ("***AT SETUP");
	Serial.begin(38400);  
	Serial1.begin(38400); 
	// initilisation des PIN de la carte SD
#ifdef WITH_SD
	pinMode(10, OUTPUT);
	pinMode(10, HIGH);
	pinMode(4, OUTPUT);
	pinMode(4, HIGH);
	if (!SD.begin(4)) {
		Serial.println("SD initialization failed");
		//return; //XXX 
	}
#endif
	randomSeed(30);
	xbee.setSerial(Serial1);
	int value = 2;
	sendRemoteAtCommand(value);
	t.run(); // activer le timer 
	initProcess();
    
#ifdef FILTER_ADDR
	initFilterTable(true);
#endif
#ifdef WITH_LOG_SD
	initFileSD();
#endif  
   
	while (start == 0) 
	{
		xbee.readPacket();
		if (xbee.getResponse().isAvailable())
		{
			if (withLog == 1)
				Serial.print("Received message (at setup) of type: ");
			// got something
			TOT_SENT_RCV++;
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
						option = rx16.getOption();
						msgType = rx16.getData(0);      
						if (msgType == 'L')
						{ 
							if (withLog == 1)
							{
								Serial.print("Level from: ");
								Serial.println(rx16.getRemoteAddress16());
							}
							uint8_t receivedLevel = rx16.getData(6);
                            long rssi = rx16.getRssi();
							LOG_RCV(myAddr16, rx16, (int) 'L', 0, 0, 0, rssi);
							in = 1;
							if (start == 0)
							{
								if (t1 == 0)
								{
									t1 = millis() + LEVEL_WAIT_PERIOD; 
									/*XXX synchro not validated 
									if (rx16.getData(1) == 6) 
									{
										tinit = millis();
									} 
									else 
									{
										tinit = millis() - LEVEL_WAIT_PERIOD*rx16.getData(2+1);
									}*/
								}
							}
							/*if (myLevel == 0 || myLevel > receivedLevel)
							{ 
								myLevel = receivedLevel + 1;
								parent = rx16.getRemoteAddress16();
								parentLevel = receivedLevel;
							}*/
                            int receivedLink = distanceFromRssi(rssi, 0);
                            //Serial.print("received RSSI ");
                            //Serial.println(rssi);
                            //Serial.print("computed");
                            //Serial.println(receivedLink);
							int newLevel = receivedLevel + receivedLink;
                            
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
							
#ifdef WITH_LOG_SD
							logRCV(1, tinit, rx16.getData(1));
#endif
						}
						else if (withLog == 1) Serial.println("Unknown");
#ifdef FILTER_ADDR	
					} 
					else  if (withLog){
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
	} //end init phase
	if (in == 1)
	{
		LOG_LINKED(should_log, myAddr16, parent, myLevel, parentLevel);
		if (withLog == 1)
		{
			Serial.print(" Level Phase Expired,  My level is:  ");
			Serial.println(myLevel);
		}
		
		generateLevelMessage();
		tinit = millis(); 
		delay(random(10,90)); //delay(10);
#ifdef WITH_LOG_SD
		long t2 = millis(); 
#endif
		xbee.send(levelTx);
 #ifdef WITH_LOG_SD
		long t4 = millis(); 
#endif  
		if (withLog == 1)
		{
			Serial.print("Level Msg Forwarded at Setup, ACK is : ");
		}
		boolean ack = 1; // no ack for the broadcast, getAck(xbee, withLog);
		int size = ARRAYELEMENTCOUNT(LEVEL_MSG);
		LOG_SEND(should_log, myAddr16, 0 /*BROADCAST_ADDRESS*/, 'L', size, LEVEL_MSG, LEVEL_MSG_SEQ_NUM, 0, 0, ack);
    
#ifdef WITH_LOG_SD

		logSENT(1, t2, t4, tinit, 1);
#endif
		TOT_SENT_RCV++; 
	}

	sleepMode(); //XXX why?
}

//-----------------------------------------------
void processBeaconMessage(Rx16Response rx16)
{
	uint16_t src16 = getMessageSender(rx16);
	uint16_t beaconMsgSeqNum = getTwoBytesFromRx(rx16, 4);
	uint16_t dataMsgSeqNum = getTwoBytesFromRx(rx16, 6);
	uint16_t photoSeqNum = getTwoBytesFromRx(rx16, 8);
	long rssi = rx16.getRssi();
	//assert(rx16.getRemoteAddress16() == src16);
	LOG_RCV(myAddr16, rx16, 'B', beaconMsgSeqNum, dataMsgSeqNum, photoSeqNum, rssi );
	if (withLog)
	{
		Serial.print("Beacon from: ");
		Serial.print(rx16.getRemoteAddress16()/*rx16.getData(1)*/);
		Serial.print(" OR ");
		Serial.println(src16, DEC);	      
		Serial.print(" having as level: ");
		Serial.println(rx16.getData(10));
	}			
	//if (rx16.getData(10) == myLevel+1)
    if (rx16.getData(10) == myLevel+1 && rssi < 85)
	{
		if (withLog)
		{
			Serial.println("Beacon from the higher level, so going to reply to: ");
		}
#ifdef WITH_LOG_SD
		nb++;
		logRCV(2, tinit, rx16.getData(1));
#endif
		generateBeaconReplyMessage(dataMsgSeqNum, photoSeqNum, rx16.getRssi());
		Tx16Request replyTx = Tx16Request(rx16.getRemoteAddress16(), REPLY_MSG, sizeof(REPLY_MSG));
		//delay(id*10);
		//jitter//XXX 
		//delay(XBEE_DELAY_BEFORE_SEND); //random(30,100)); 
		delay(random(10,30)); //jitter
		xbee.send(replyTx);
		if (withLog){
			Serial.print(rx16.getRemoteAddress16()/*rx16.getData(1)*/);
			printResponse();
			Serial.print(" and Ack is: ");
		}
		boolean ack = getAck(xbee, withLog);
		
        LOG_SEND(should_log, myAddr16, src16, 'R', ARRAYELEMENTCOUNT(REPLY_MSG), REPLY_MSG, REPLY_MSG_SEQ_NUM, dataMsgSeqNum, photoSeqNum, ack);
#ifdef WITH_LOG_SD
		long t2 = millis();
		long t4 = millis();
		TXD = SD.open("sent.txt", FILE_WRITE);
		if (TXD) 
		{ 
			TXD.print(-3);
			TXD.print(" ");
			TXD.print(t2 -tinit);
			TXD.print(" ");
			TXD.print(t4 -tinit);
			TXD.print(" ");
			TXD.print(rx16.getData(1));
			TXD.print(" ");
			TXD.println("");
			TXD.close();
		}
#endif
		TOT_SENT_RCV++;
		receiveDataMode(rx16, src16, dataMsgSeqNum, photoSeqNum);
		sleepMode();
		tactif = millis();
		ACTIVE_PERIOD = ACTIVE_PERIOD + millis();
		//waitSelect(src16, dataMsgSeqNum, photoSeqNum); 
	}
}
//-----------------------------------------------
void loop()
{
	uint8_t msgType;
	uint8_t j1 = 0;
	tactif = millis();
	ACTIVE_PERIOD = ACTIVE_PERIOD + millis();
	uint16_t transmitter16;
	uint16_t msgSeqNum;
	uint16_t photoSeqNum;
#ifdef FILTER_ADDR
	initFilterTable(false);
#endif

	while(tactif < ACTIVE_PERIOD)
	{
		//i++;
		xbee.readPacket();
		if (xbee.getResponse().isAvailable()) 
		{
			// got something
			TOT_SENT_RCV++;
			if (xbee.getResponse().getApiId() == RX_16_RESPONSE || xbee.getResponse().getApiId() == RX_64_RESPONSE) 
			{
				if (xbee.getResponse().getApiId() == RX_16_RESPONSE) 
				{
					xbee.getResponse().getRx16Response(rx16); 
#ifdef FILTER_ADDR
					if(!isfiltered(rx16.getRemoteAddress16(), filterTable, sizeFilterTable))
					{
#endif 
						option = rx16.getOption();
						msgType = getMessageType(rx16); //rx16.getData(0);
						if (withLog == 1)
						{ 
							Serial.print("[");
							Serial.print(millis()); 
							Serial.print("] Received (in Loop): ");
							//printRx();
							Serial.print(" of type: ");
						}
						if (msgType == 'B')
							processBeaconMessage(rx16);
						else {
							if (msgType != 'B' && msgType != 'D' && withLog)
							{
								Serial.print("Unknow type: ");
								Serial.println(rx16.getData(0));
							}
						}
#ifdef FILTER_ADDR
					}
					else if (withLog) {
						Serial.print("Filtered Address: ");
						Serial.println(rx16.getRemoteAddress16());  
					}
#endif
	    
				} else if (withLog == 1) Serial.print(" NOT RX_16_RESPONSE: ");  
			}
		}    
	}
	sleepMode();
	tactif = millis();
	ACTIVE_PERIOD = ACTIVE_PERIOD + millis();
}

//-----------------------------------------------

void sleepMode()
{
#ifdef WITH_SLEEP
  /// pour le mode sleep j'ai pensé à 3 méthodes
  //---------------------1-------------------------//
  // soit brancher une resitance entre le PIN 9 de transeiver et un pin d'arduino
  // le mode hibernate est activé lorsque pin 9 est à l'état haut
  // voici le code que j ai trouvé sur internet 
  
  //// wake up the XBee
  //pinMode(XBee_wake, OUTPUT);
  //digitalWrite(XBee_wake, LOW);

  // do whatever I need to do with the XBee

// put the XBee to sleep
//pinMode(XBee_wake, INPUT); // put pin in a high impedence state
// digitalWrite(XBee_wake, HIGH);
// cette méthode n'a pas fonctionnée 
//--------------------2-----------------------------//
// mettre SM = 4 et fixer la valeur de cycle en mode api 

//--------------------3----------------------------//
// -- changer le canal de transmission
// ainsi le transeiver n'écoutera pas les communications
	if (withLog == 1)
		Serial.println("** Start Mode Sleep");
	uint8_t j1 = 0;
	int value = 8; //idValue[0] = 8;
	//AtRequest.setCommandValue(idValue);
	sendRemoteAtCommand(value);
	t2 = millis();
	if (hasSent == 1)
	{ // si le transeiver a déja envoyé de message la période de sleep est plus grande
		hasSent = 0;
		tsleep = random(300,10000);
	} 
	else {
		tsleep = random(300,3000);
	}
	if (withLog == 1){
		Serial.print("Decided to Sleep with a period of: ");
		Serial.println(tsleep);
	}
	LOG_START_SLEEP(myAddr16, tsleep);
	while (millis () - t2 < tsleep) 
	{

	}
	LOG_END_SLEEP(myAddr16, tsleep);
#ifdef WITH_LOG_SD
	logSLEEP(t2, millis());
#endif
	//idValue[0] = 2;
	int value = 2
	//AtRequest.setCommandValue(idValue);
	sendRemoteAtCommand(value);
	if (withLog == 1) 
		Serial.println("** End Mode Sleep");
#endif
}

//-----------------------------------------------

void findRelay(uint8_t dataMsg[], int dataMsgLength, uint16_t dataMsgSeqNum, uint16_t photoSeqNum)
{
	long tStartReply;
	long tEndReply;
	long tBeaconStart = millis(); 
	long tBeaconEnd = millis() + MAX_BEACON_PERIOD;
	int nbSentBeacon = 0;
	int nbRcvReply = 0;
	//tbeacon = millis();
	//CP = millis() + CP;
	generateBeaconMessage(dataMsgSeqNum, photoSeqNum);
	uint8_t msgType;

	
	while (NB_RCV_REPLY < MAX_RCV_REPLY_NB/*nbSentBeacon < MAX_SENT_BEACON_NB && */&&  tBeaconStart <= tBeaconEnd)
	{ 
#ifdef WITH_LOG_SD
		long t2 = millis();
#endif
		//delay(10*id);
		delay(XBEE_DELAY_BEFORE_SEND); //delay(random(30,100));
		xbee.send(beaconTx);
		nbSentBeacon++;
#ifdef WITH_LOG_SD
		long t4 = millis();
#endif
		if (withLog)
		{
			Serial.print("Sending Beacon for idpacket: ");
			Serial.print(idpacket);
			Serial.print(" and ack is: ");
		}
		
		boolean ack = 1; // getAck(xbee, withLog);
		LOG_SEND(should_log, myAddr16, BROADCAST_ADDRESS, 'B', ARRAYELEMENTCOUNT(BEACON_MSG), BEACON_MSG, BEACON_MSG_SEQ_NUM, dataMsgSeqNum, photoSeqNum, ack);
		TOT_SENT_RCV++;
#ifdef WITH_LOG_SD
		logSENT(2, t2, t4, tinit, 1);
		TXD = SD.open("sent.txt", FILE_WRITE);
#endif
		tStartReply = millis();
		tEndReply = millis() + WAIT_REPLY_PERIOD;
		
		while (tStartReply < tEndReply && NB_RCV_REPLY < MAX_RCV_REPLY_NB)
		{ 
			xbee.readPacket();
			if (xbee.getResponse().isAvailable()) 
			{
				// got something
				TOT_SENT_RCV++;
				if (xbee.getResponse().getApiId() == RX_16_RESPONSE || xbee.getResponse().getApiId() == RX_64_RESPONSE) 
				{
					// got a rx packet
					if (xbee.getResponse().getApiId() == RX_16_RESPONSE) 
					{
						xbee.getResponse().getRx16Response(rx16);
						option = rx16.getOption();
						msgType = rx16.getData(0);
						if (msgType == 'R')
						{
							if (withLog)
                                Serial.print("Rcv reply with info :");
							uint16_t receivedSeqNum = getTwoBytesFromRx(rx16, 4);
							uint16_t receivedDataSeqNum = getTwoBytesFromRx(rx16, 6);
							uint16_t receivedPhotoSeqNum = getTwoBytesFromRx(rx16, 8);
							Serial.print(receivedDataSeqNum);
							Serial.print(receivedPhotoSeqNum);
							if (receivedDataSeqNum == dataMsgSeqNum && receivedPhotoSeqNum == photoSeqNum)
							{
								nbRcvReply++;
								LOG_RCV(myAddr16, rx16, 'R', receivedSeqNum, receivedDataSeqNum, receivedPhotoSeqNum, rx16.getRssi());
								processBeaconReplyMessage(rx16, dataMsgSeqNum, photoSeqNum);
							}
						}
					}
				}
			}
			tStartReply = millis();
			//tint=millis();
		}
		//RI = 200;
		//tbeacon = millis();
		tBeaconStart = millis();
	}
	//CP = 10000;
	NB_RCV_REPLY = 0;
  
	if (bestTx16 != 0)
	{
		relayFound(dataMsg, dataMsgLength, dataMsgSeqNum, photoSeqNum);
	}
	else{
      /*No neigh selected*/
#ifdef WITH_REPEAT
        //increment = 0;
        if (retry < 3)
        {
            Serial.println("no relay");
            retry = retry + 1;   
            findRelay(dataMsg, dataMsgLength, dataMsgSeqNum, photoSeqNum);
        } 
        else 
            retry = 0; 
#endif
      if (withLog)
			Serial.println("No Reponse");
	}
#ifdef WITH_LOG_SD
	logSENT(idpacket, t2, millis(), tinit, best);
#endif
}

//-----------------------------------------------
void relayFound(uint8_t dataMsg[], int dataMsgLength, uint16_t dataMsgSeqNum, uint16_t photoSeqNum)
{
#ifdef WITH_LOG_SD
	long t2 = millis();
#endif
	//if (generateSelectMessage(dataMsgSeqNum, photoSeqNum)){
#ifdef WITH_LOG_SD
	long t4 = millis();
	logSent(4, t2, t4, tinit, 1);
#endif
	Tx16Request dataTx = Tx16Request(bestTx16, dataMsg, dataMsgLength); 
#ifdef WITH_LOG_SD
	long t2 = millis();
#endif       
	xbee.send(dataTx);
	TOT_SENT_RCV++;
	if(withLog){
		Serial.print("Sending data to relay and ack is: ");
	}
	boolean ack = getAck(xbee, withLog);
	LOG_SEND(should_log, myAddr16, bestTx16, 'D', dataMsgLength, dataMsg, 
    dataMsgSeqNum, dataMsgSeqNum, photoSeqNum, ack);
    
    bestTx16 = 0;
    bestTx[0] = 0;
    bestTx[1] = 0;
    
#ifdef WITH_REPEAT_2
    if (ack == 0)
    {
        if (retryData < 3)
        {
            retryData = retryData + 1;
            findRelay(dataMsg, dataMsgLength, dataMsgSeqNum, photoSeqNum);
        }
        else
            retryData = 0;
    }
#endif    
	
}

//-----------------------------------------------

void generateDataMessage(Rx16Response rx16, uint8_t dataMsg[], int dataMsgLength)
{
	int i = 0;
    dataMsg[0] = 'D';
	dataMsg[1] = dataMsgLength;
	dataMsg[2] = myAddress[1];
	dataMsg[3] = myAddress[0];
	
	dataMsg[4] = rx16.getData(4);
	dataMsg[5] = rx16.getData(5);
	
	dataMsg[6] = rx16.getData(6);
	dataMsg[7] = rx16.getData(7);
	
	int pathLength = rx16.getData(8);
	//Serial.println(pathLength);
	dataMsg[8] = pathLength + 1;  // the router adds its address
	if (pathLength == MAX_PATH_LENGTH)
	{
		Serial.println("No place left for my Address!");
	}
	for (i = 0; i < pathLength*2; i++)
	{
		dataMsg[i+9] = rx16.getData(i+9);
	}
	int indexStartPath = 8 + pathLength * 2 + 1; 
	dataMsg[indexStartPath] = rx16.getData(2); //the sender of the message //myAddress[1];
	dataMsg[indexStartPath+1] = rx16.getData(3); //the sender of the message

	//i = 9 + pathLength*2 - 1;
	i = indexStartPath + 2;
	while (i < dataMsgLength)
	{
		dataMsg[i] = rx16.getData(i-2);
		i++;
	}
}

//-----------------------------------------------
void receiveDataMode(Rx16Response rx16, uint16_t  src16,  uint16_t dataMsgSeqNum, uint16_t photoSeqNum)
{
	uint8_t j = 2;
	uint8_t sent2 = 0;
	long tStart = millis();
	long tEnd = WAIT_DATA_PERIOD + millis();
	uint8_t msgType;
	uint8_t dataReceived = 0;
	
	while (dataReceived == 0 && tStart < tEnd) 
	{
		xbee.readPacket();
		if (xbee.getResponse().isAvailable()) 
		{
			// got something
			TOT_SENT_RCV++;
			if (xbee.getResponse().getApiId() == RX_16_RESPONSE || xbee.getResponse().getApiId() == RX_64_RESPONSE)
			{
				// got a rx packet
				if (xbee.getResponse().getApiId() == RX_16_RESPONSE)
				{
					xbee.getResponse().getRx16Response(rx16);
					option = rx16.getOption();
					msgType = rx16.getData(0);
					if (msgType == 'D')
					{
						uint16_t receivedDataSeqNum = getTwoBytesFromRx(rx16, 4);
						uint16_t receivedPhotoSeqNum = getTwoBytesFromRx(rx16, 6);
 						assert(receivedDataSeqNum == dataMsgSeqNum); //XXX: works only with blocking architecture, should modify
						assert(receivedPhotoSeqNum == photoSeqNum);
						assert(src16 == rx16.getRemoteAddress16());
						dataReceived = 1;
						processDataMessage(rx16);
					}
					else
                    {
                        /*if (withLog){
                            Serial.print("Unkown format : ");
                            Serial.println(msgType);
                        }*/
                    }
				}
			}
		}
		tStart = millis();
	}
	if (withLog)
        Serial.println("EEEEEEEND WAIT DATA ");
	dataReceived = 0;
}

//-----------------------------------------------
void processDataMessage(Rx16Response rx16)
{
	uint16_t receivedDataSeqNum = getTwoBytesFromRx(rx16, 4);
	uint16_t receivedPhotoSeqNum = getTwoBytesFromRx(rx16, 6);
	
	LOG_RCV(myAddr16, rx16, 'D', receivedDataSeqNum, receivedDataSeqNum, receivedPhotoSeqNum, rx16.getRssi());
	if (withLog)
		printRx();
	int dataMsgLength = rx16.getDataLength() + 2; //+2 for path adding
	uint8_t dataMsg[dataMsgLength];
	generateDataMessage(rx16, dataMsg, dataMsgLength);
#ifdef WITH_LOG_SD
	logRCV(idpacket, tinit, rx16.getRemoteAddress16());
#endif
	if (myLevel == 1)
	{
		if (withLog){
			Serial.print(" Am at level 1, farwarding data to the sink, and Ack is ");
		}
#ifdef WITH_LOG_SD
		long t2 = millis();
#endif	    
		Tx16Request dataTx = Tx16Request(SINK_ADDR, dataMsg, dataMsgLength);
		delay(XBEE_DELAY_BEFORE_SEND);//random(30,100)); //XXX why delay
		xbee.send(dataTx);
#ifdef WITH_LOG_SD
		long t4 = millis();
#endif
		boolean ack = getAck(xbee, withLog);
		if (withLog)
			Serial.print(ack);
		LOG_SEND(should_log, myAddr16, SINK_ADDR, 'D', ARRAYELEMENTCOUNT(dataMsg), dataMsg, receivedDataSeqNum, receivedDataSeqNum, receivedPhotoSeqNum, ack);
		TOT_SENT_RCV++;
        
#ifdef WITH_REPEAT_2
        boolean more = 1; 
        while (ack == 0 && more == 1)
        {
            if (retryData < 3)
            {
                retryData = retryData + 1;
                delay(XBEE_DELAY_BEFORE_SEND);//random(30,100)); //XXX why delay
                xbee.send(dataTx);
                ack = getAck(xbee, withLog);
                
                LOG_SEND(should_log, myAddr16, SINK_ADDR, 'D', ARRAYELEMENTCOUNT(dataMsg), dataMsg, receivedDataSeqNum, receivedDataSeqNum, receivedPhotoSeqNum, ack);
                TOT_SENT_RCV++;
            }
            else{
                retryData = 0;
                more = 0;
            }
        }
#endif  
        
#ifdef WITH_LOG_SD
		logSENT(idpacket, t2, t4, ADDR_SINK);
#endif
	}
	if (myLevel > 1)
	{
		if (withLog)
			Serial.println("Received Data, not close enough So Find Relay");		
		findRelay(dataMsg, dataMsgLength, receivedDataSeqNum, receivedPhotoSeqNum);
	}
}

//-----------------------------------------------

// une procédure pour changer le canal de transmission en mode API 
void sendRemoteAtCommand(int value) {
	Serial.println("Sending command to the XBee");
	uint8_t idCmd[] = {'I','D'};
	uint8_t idValue[] = {value};
	// Create a AT request with the ID command
	AtCommandRequest AtRequest = AtCommandRequest(idCmd, idValue, sizeof(idValue));
	// Create a Remote AT response object
	AtCommandResponse AtResponse = AtCommandResponse();

	uint8_t ok = 0;
	while (ok == 0)
	{
		// send the command
		xbee.readPacket();
		Serial.println("Check data");
		if (xbee.getResponse().isAvailable()) 
		{
			Serial.println("can not transmit command");
			Serial.println("wait");
		}
		else 
		{
			xbee.send(AtRequest);
			Serial.println(" Modify PID ");
			// wait up to 5 seconds for the status response
			if (xbee.readPacket(500)) 
			{
				// got a response!
				Serial.println("Checked ");
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
						ok=1;
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
			else if (xbee.getResponse().isError()) 
			{
				Serial.print("Error reading packet.  Error code: ");  
				Serial.println(xbee.getResponse().getErrorCode());
			} 
			else {
				Serial.print("No response from radio");  
			}
		}
	}
}

void printResponse()
{
	int i;
	int nb = ARRAYELEMENTCOUNT(REPLY_MSG);
	Serial.print("(size:");
	Serial.print(nb);
	Serial.print("): ");
	Serial.print("[");
	for (i = 0; i < nb; i++){
		Serial.print(REPLY_MSG[i], DEC);
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
	for (i = 0; i < rx16.getDataLength(); i++)
	{
		Serial.print(rx16.getData(i));
		Serial.print("|");
	}
	Serial.print("]");
}

