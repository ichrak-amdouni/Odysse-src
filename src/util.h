// #---------------------------------------------------------------------------
// #  . Ichrak Amdouni,   Infine Project-Team, Inria Saclay
// #  Copyright 2015 Institut national de recherche en informatique et
// #  en automatique.  All rights reserved.  Distributed only with permission.
// #---------------------------------------------------------------------------
//----------------------
#define WITH_CODING
const int MAX_PATH_LENGTH = 10;
const int MAX_DATA_SIZE = 61; /*(100 - headerLngth = 9 octets)*/
//const int MAX_DATA_SIZE = 100 - MAX_PAYLOAD_SIZE;
//uint16_t SINK_ADDR = 0x01;
#define SmallScenario1
#ifdef SmallScenario1 
uint16_t SINK_ADDR = 0x06; // 0x1d; //29; //0x06; //0x10; //0x01; //0x05; // 0x10;
#else
uint16_t SINK_ADDR = 0x02; //0x10; //0x01; //0x05; // 0x10;
#endif
#define INFINITE_LEVEL 10000 
//----------Messages--------------

const int LEVEL_MSG_LENGTH = 7;
const int BEACON_MSG_LENGTH = 11;
const int REPLY_MSG_LENGTH = 13;
//const int SELECT_MSG_LENGTH = 12;
//const int DATA_MSG_LENGTH = MAX_DATA_SIZE;

uint8_t LEVEL_MSG[LEVEL_MSG_LENGTH]; // = {'L',0,0,0,0,0,0};
uint8_t BEACON_MSG[BEACON_MSG_LENGTH]; //= {'B',0,0,0,0,0,0,0,0};
uint8_t REPLY_MSG[REPLY_MSG_LENGTH]; // = {'R',0,0,0,0,0,0,0,0,0,0}; 
//uint8_t SELECT_MSG[SELECT_MSG_LENGTH]; // = {'S',0,0,0,0,0,0,0,0,0}; 


uint8_t DATA_MSG[MAX_DATA_SIZE];
uint16_t MAX_SEQ_NUM = 65535;
//------------------------

//uint16_t BROADCAST_ADDRESS = 0xFFFF;

//------------------------

//uint16_t getMessageSeqNum(uint8_t message[]);
//uint16_t getSeqNumFromRx(Rx16Response rx16);
//uint16_t getPhotoSeqNumFromRx(Rx16Response rx16);
void updateMessage(uint8_t message[], int length, int index, uint8_t toAdd[2]);
void updateMessageWithLong(uint8_t message[], int length, int index, uint16_t toAdd);
uint8_t getMessageType(Rx16Response rx16);
uint8_t getMessageSender(Rx16Response rx16);
uint16_t getTwoBytesFromRx(Rx16Response rx16, int index);
//------------------------
void dump_eeprom();

uint16_t get_uid();

void write_uid(uint16_t);

uint16_t getAddr16(uint8_t [2]);

uint16_t getLong(uint8_t [2]);

bool isfiltered(uint16_t, uint16_t[], int);

#ifdef WITH_CODING
void xorArray(uint8_t[], uint8_t[], const uint8_t);
void assignArray(uint8_t [], const uint8_t [], const uint8_t);

//void init2DArray(uint8_t [][], uint8_t, const uint8_t);
#endif
//----------------------
#define ARRAYELEMENTCOUNT(x) (sizeof (x) / sizeof (x)[0])

void initFileSD();

void logRCV(int nb, long time, int toPrint);

void logSENT(int nb, long t2, long t4, long time, int nb2);

void logSLEEP(long t1, long t2);


//----------------------
#if 0
void prettySerialPrint(char *fmt, ...);
#endif

void pSerialPrint(char *fmt, ... );

#define BEGIN_MACRO do {
#define END_MACRO } while(__LINE__==-1)


#define LOG(should_log, ...) BEGIN_MACRO	\
  if (should_log) {	\
    pSerialPrint(__VA_ARGS__);		\
  }							\
  END_MACRO
  
void LOG_RCV(uint16_t, Rx16Response, int, long, long, long, long);
//void LOG_SEND(bool should_log, uint16_t sender, int msgType, int size, uint8_t msg[], long idpacket, bool ack);
void LOG_SEND(bool should_log, uint16_t sender, uint16_t destination, int msgType, int size, uint8_t msg[], long msgSeqNum, long dataMsgSeqNum, long photoSeqNum,  bool ack);
void LOG_SELECT(bool should_log, uint16_t myAddr16, uint16_t best, long F);
void LOG_LINKED(bool should_log, uint16_t myAddr, uint16_t parent, int level, int parentLevel);
void LOG_START_SLEEP(uint16_t, long);
void LOG_END_SLEEP(uint16_t, long);
uint8_t distanceFromRssi(long rssi, int margin);