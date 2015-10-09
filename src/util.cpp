// #---------------------------------------------------------------------------
// #  . Ichrak Amdouni,   Infine Project-Team, Inria Saclay
// #  Copyright 2015 Institut national de recherche en informatique et
// #  en automatique.  All rights reserved.  Distributed only with permission.
// #---------------------------------------------------------------------------
#define WITH_CODING
#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif
#include "HardwareSerial.h"
#include <EEPROM.h>


#define MIN_EEPROM_SIZE 512
#define UID_ADDRESS (MIN_EEPROM_SIZE-2)

#include <XBee.h>
#include <SD.h>  
File RCVD;   
File TXD;    
File SLEEP;  
#define FILE_RCV "receive.txt"
#define FILE_SENT "sent.txt"
#define FILE_SLEEP "sleep.txt"

  
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>

//#define WITH_FLUSH 1
//--------------------------------------------------------
#ifndef ARRAYELEMENTCOUNT
#define ARRAYELEMENTCOUNT(x) (sizeof (x) / sizeof (x)[0])
#endif
//#include "util.h"
//----------------------
uint16_t getLong(uint8_t input[2])
{
	uint16_t output = (((uint16_t)input[0]) << 8) | input[1];
	return output;  
}
/*uint16_t getLongNew(uint8_t input[2])
{
	uint16_t output = (((uint16_t)input[1]) << 8) | input[0];
	return output;  
}*/
//----------------------
uint16_t getTwoBytesFromRx(Rx16Response rx16, int index)
{
	uint8_t d[2] = {rx16.getData(index), rx16.getData(index+1)};
	uint16_t d16 = getLong(d);
	return d16;
}

/*uint16_t getMessageSeqNum(uint8_t message[])
{
	uint8_t seq[2] = {message[4], message[5]};
	uint16_t seq16 = getLong(seq);
	return seq16;
}

uint16_t getSeqNumFromRx(Rx16Response rx16)
{
	uint8_t seq[2] = {rx16.getData(4), rx16.getData(5)};
	uint16_t seq16 = getLong(seq);
	return seq16;
}

uint16_t getDataLotSeqNumFromRx(Rx16Response rx16)
{
	uint8_t seq[2] = {rx16.getData(6), rx16.getData(7)};
	uint16_t seq16 = getLong(seq);
	return seq16;
}
*/
void updateMessageWithLong(uint8_t message[], int length, int index, uint16_t toAdd)
{
	if (index > length)
		return;
	message[index] = toAdd >> 8;
	message[index+1] = toAdd & 0xFF; 
	//INIT_MSG[1] = myAddress[1]; 
	//INIT_MSG[2] = myAddress[0]; 
}

void updateMessage(uint8_t message[], int length, int index, uint8_t toAdd[2])
{
	if (index > length)
		return;
	message[index] = toAdd[1];
	message[index+1] = toAdd[0];
	//INIT_MSG[1] = myAddress[1]; 
	//INIT_MSG[2] = myAddress[0]; 
}

uint8_t getMessageType(Rx16Response rx16) //rx16.getData(2+1); //XXX fixed: getData(2)*
{
	return rx16.getData(0);
}

uint8_t getMessageSender(Rx16Response rx16)
{
	uint8_t transmitter[2] = {rx16.getData(2), rx16.getData(3)};
	uint16_t transmitter16 = getLong(transmitter);
	return transmitter16;						
}
//----------------------
void dump_eeprom()
{
  int i=0;
  Serial.print("EEPROM content:");
  for (i=0;i<MIN_EEPROM_SIZE;i++) {
    if ((i & 0xf) == 0) {
      Serial.println();
    }
    uint8_t value = EEPROM.read(i);
    Serial.print(value, HEX);
  }
  Serial.println();
}

uint16_t get_uid()
{
  uint16_t high = EEPROM.read(UID_ADDRESS);
  uint16_t low = EEPROM.read(UID_ADDRESS+1);
  return (high << 8) | low;
}

void write_uid(uint16_t new_uid)
{
  if (get_uid() == new_uid)
    return;

  EEPROM.write(UID_ADDRESS, new_uid>>8);
  EEPROM.write(UID_ADDRESS+1, new_uid&0xff);
}

/*uint16_t getAddr16(uint8_t address[2]){
  
  uint16_t addr16 = (((uint16_t)address[0]) << 8) | address[1];
  return addr16;  
}*/




bool isfiltered( uint16_t address, uint16_t table[], int size){
  for (int i=0; i<size; i++){
    if (address == table[i])
      return true;
  }
  return false;
}
//----------------------------------------------------------------
void initFileSD(){
  RCVD = SD.open(FILE_RCV, FILE_WRITE);
  if (RCVD){
    RCVD.print("######################################################");
    RCVD.println("");
    RCVD.close();
  }
  TXD = SD.open(FILE_SENT, FILE_WRITE);
  if (TXD){
    TXD.print("######################################################");
    TXD.println("");
    TXD.close();
  }
  SLEEP = SD.open(FILE_SLEEP, FILE_WRITE);
  if (SLEEP){
    SLEEP.print("######################################################");
    SLEEP.println("");
    SLEEP.close();
  }
}

void logRCV(int nb, long time, int toPrint){
  RCVD = SD.open(FILE_RCV, FILE_WRITE);
  if (RCVD){
    RCVD.print(nb);
    nb++;
    RCVD.print(" ");
    RCVD.print(millis() - time);
    RCVD.print(" ");
    RCVD.print(toPrint);
    RCVD.println("");
    // close the file:
    RCVD.close();
  }
}


void logSENT(int nb, long t2, long t4, long time, int nb2){
  TXD = SD.open(FILE_SENT, FILE_WRITE);
  if (TXD){
    TXD.print(nb);
    TXD.print(" ");
    TXD.print(t2 - time);
    TXD.print(" ");
    TXD.print(t4 - time);
    TXD.print(" ");
    TXD.print(nb2);
    TXD.print(" ");
    TXD.println("");
    // close the file:
    TXD.close();
  }
}
 
void logSLEEP(long t1, long t2){
  SLEEP = SD.open(FILE_SLEEP, FILE_WRITE);
  if (SLEEP){
    SLEEP.print(t1);
    SLEEP.print(" ");
    SLEEP.print(t2);
    SLEEP.print(" ");
    SLEEP.println("");
  // close the file:
    SLEEP.close();
  }
}

//----------------------------------------------------------------
#ifdef WITH_FLUSH
void FlushInput()
{
  //https://www.safaribooksonline.com/library/view/arduino-cookbook-2nd/9781449321185/ch04.html
  //while (Serial.Available() > 0) 
  //	  Serial.Read(); //discard received data
	Serial.flush(); //waits for outgoing data to be sent rather than discarding them.
}
#endif

//----------------------------------------------------------------
#if 0
void prettySerialPrint(char *fmt, ...)
{
  va_list ap;
  int d;
  uint16_t l;
  float f;
  char c, *s;

  va_start(ap, fmt);
  while (*fmt)
      switch (*fmt++) {
      case 's':              /* string */
	  s = va_arg(ap, char *);
	  Serial.print(s);
	  break;
      case 'd':              /* int */
	  d = va_arg(ap, int);
	  Serial.print(d);
	  break;
      case 'ul':              /*  */
	  l = va_arg(ap, uint16_t);
	  Serial.print(l);
	  break;
      case 'f':              /* double */
	  f = va_arg(ap, double);
	  Serial.print(f);
	  break;
      case 'c':              /* char */
	  /* need a cast here since va_arg only
	      takes fully promoted types */
	  c = (char) va_arg(ap, int);
	  Serial.print(c);
	  break;
      default:
	Serial.print("Unknwon format");
	break;
      }
  va_end(ap);
}

#endif
//------------------------------------------------------------------

#define PRINTF_BUF 156 // define the tmp buffer size (change if desired)

void pSerialPrint(char *fmt, ... )
{
  char buf[PRINTF_BUF]; // resulting string limited 
  va_list args;
  va_start (args, fmt );
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end (args);
  Serial.print(buf);
#ifdef WITH_FLUSH
  FlushInput();
#endif
}

#ifdef F // check to see if F() macro is available
void pSerialPrint(const __FlashStringHelper *fmt, ... )
{
  char buf[PRINTF_BUF]; // resulting string limited to PRINTF_BUF chars
  va_list args;
  va_start (args, fmt);
#ifdef __AVR__
  vsnprintf_P(buf, sizeof(buf), (const char *)fmt, args); // progmem for AVR
#else
  vsnprintf(buf, sizeof(buf), (const char *)fmt, args); // for the rest of the world
#endif
  va_end(args);
  Serial.print(buf);
  Serial.print("end222");
#ifdef WITH_FLUSH
  FlushInput();
#endif
}
#endif

void LOG_RCV(uint16_t receiver, Rx16Response rx16, int msgType, long msgSeqNum, long dataMsgSeqNum, long photoSeqNum, long rssi)
{
#ifdef WITH_FLUSH
  FlushInput();
#endif 
  Serial.print(msgType);
  //pSerialPrint("\n{'time':%u,'event':'rcv','receiver':0x", millis() );
  pSerialPrint("\n{'event':'rcv','receiver':0x", millis() );
  Serial.print(receiver, HEX);
  Serial.print(",'src':0x");
  Serial.print(rx16.getRemoteAddress16(), HEX);
  Serial.print(",'idpacket':");
  Serial.print(msgSeqNum);
  Serial.print(",'dataMsgSeqNum':");
  Serial.print(dataMsgSeqNum);
  Serial.print(",'photoSeqNum':");
  Serial.print(photoSeqNum);
  Serial.print(",'type':");
  if (msgType == 'L')
    Serial.print("'level'");
  else if  (msgType == 'B')
    Serial.print("'beacon'");
  else if (msgType == 'R')
    Serial.print("'reply'");
  else if (msgType == 'D')
    Serial.print("'data'");
  else if (msgType == 'S')
    Serial.print("'select'");
  else Serial.print("Unknown");
  Serial.print(",'rssi':");
  Serial.print(rssi);
  pSerialPrint(",'size':%d", rx16.getDataLength());
#ifdef WITH_FLUSH
  FlushInput();
#endif  
  Serial.print(",'payload':");
  int i;
  Serial.print("'");
  for (i = 0; i < rx16.getDataLength() ; i++){
    //Serial.print("\\");
    if(rx16.getData(i) < 0x10)
    {
      Serial.print("x0");
      Serial.print(rx16.getData(i), HEX);
    }else{
	Serial.print("x");
	Serial.print(rx16.getData(i), HEX);
      //pSerialPrint("x%x", msg[i]);//print("\x");
    }
    
    //Serial.print("0x");
    //Serial.print(rx16.getData(i), HEX);
    
  }
  Serial.print("'");
  Serial.print("}\n");
#ifdef WITH_FLUSH
  FlushInput();
#endif
}

//LOG_SEND(should_log, myAddr16, 'I', Init, ack);
void LOG_SEND(bool should_log, uint16_t sender, uint16_t destination, int msgType, int size, uint8_t msg[], long msgSeqNum, long dataMsgSeqNum, long photoSeqNum, bool ack)
{
#ifdef WITH_FLUSH
  FlushInput();
#endif 
  //pSerialPrint("\n{'time':%u,'event':'send','src':0x", millis());
  pSerialPrint("\n{'event':'send','src':0x", millis());
  Serial.print(sender, HEX);
  Serial.print(",'dst':0x");    
  Serial.print(destination, HEX);
  Serial.print(",'type':");
  if (msgType == 'L')
    Serial.print("'level'");
  else if  (msgType == 'B')
    Serial.print("'beacon'");
  else if (msgType == 'R')
    Serial.print("'reply'");
  else if (msgType == 'D')
    Serial.print("'data'");
  else if (msgType == 'S')
    Serial.print("'select'");
  Serial.print(",'idpacket':");
  Serial.print(msgSeqNum);
  Serial.print(",'dataMsgSeqNum':");
  Serial.print(dataMsgSeqNum);
  Serial.print(",'photoSeqNum':");
  Serial.print(photoSeqNum);
  pSerialPrint(",'size':%d", size);
#ifdef WITH_FLUSH
  FlushInput();
#endif
  Serial.print(",'payload':");
  int i;
  Serial.print("'");
  for (i = 0; i < size ; i++){
    //Serial.print("\\");
    if(msg[i] < 0x10)
    {
      Serial.print("x0");
      Serial.print(msg[i], HEX);
    }else{
	Serial.print("x");
	Serial.print(msg[i], HEX);
      //pSerialPrint("x%x", msg[i]);//print("\x");
    }
    
    
    //Serial.print(msg[i], HEX);
    //Serial.print("");
  }
  Serial.print("'");
  pSerialPrint(",'ack':%d", ack);
  Serial.print("}\n");
#ifdef WITH_FLUSH
  FlushInput();
#endif
  /*for (int i = 0; i < request.getFrameDataLength(); i++) {
//	std::cout << "sending byte [" << static_cast<unsigned int>(i) << "] " << std::endl;
	sendByte(request.getFrameData(i), true);
	checksum+= request.getFrameData(i);
    }
   */
}

void LOG_SELECT(bool should_log, uint16_t myAddr16, uint16_t best, long F)
{
#ifdef WITH_FLUSH
  FlushInput();
#endif 
  pSerialPrint("\n{'time':%u,'event':'select','myAddr':0x", millis());
  Serial.print(myAddr16, HEX);
  Serial.print(",'selected':");
  Serial.print(best, HEX);
  Serial.print(",'score':");
  Serial.print(F);
  Serial.print("}\n");
//Serial.read() takes the first byte from the queue, reads it and deletes it. Next time Serial.read is called the next byte in the queue is available.
#ifdef WITH_FLUSH
  FlushInput();
#endif
}

void LOG_LINKED(bool should_log, uint16_t myAddr16 , uint16_t parent, int level, int parentLevel)
{
#ifdef WITH_FLUSH
  FlushInput();
#endif 
  //pSerialPrint("\n{'time':%u,'event':'linked','myAddr':0x", millis());
  pSerialPrint("\n{'event':'linked','myAddr':0x", millis());
  Serial.print(myAddr16, HEX);
  Serial.print(",'parent':0x");
  Serial.print(parent, HEX);
  pSerialPrint(",'myLevel':%d", level);
  pSerialPrint(",'parentLevel':%d}\n", parentLevel);
#ifdef WITH_FLUSH
  FlushInput();
#endif
}

void LOG_START_SLEEP(uint16_t myAddr, long tsleep)
{
#ifdef WITH_FLUSH
  FlushInput();
#endif 
  //pSerialPrint("\n{'time':%u,'event':'start-sleep','myAddr':", millis());
  pSerialPrint("\n{'event':'start-sleep','myAddr':", millis());
  Serial.print(myAddr);
  Serial.print(", 'tsleep':");
  Serial.print(tsleep);
  Serial.print("}\n");
#ifdef WITH_FLUSH
  FlushInput();
#endif
}

void LOG_END_SLEEP(uint16_t myAddr, long tsleep)
{
#ifdef WITH_FLUSH
  FlushInput();
#endif 
  //pSerialPrint("\n{'time':%u,'event':'end-sleep','myAddr':", millis());
  pSerialPrint("\n{'event':'end-sleep','myAddr':", millis());
  Serial.print(myAddr);
  Serial.print(", 'tsleep':");
  Serial.print(tsleep);
  Serial.print("}\n");
#ifdef WITH_FLUSH
  FlushInput();
#endif
}

uint8_t distanceFromRssi(long rssi, int margin)
{
    if (rssi < 75 + margin)
        return 1;
    else return 2;
}

#ifdef WITH_CODING
void xorArray(uint8_t output[], uint8_t input[], const uint8_t size)
{
    for (int i=0; i < size; i++)
        output[i] = output[i] ^ input[i];
}

void assignArray(uint8_t dstArray[], const uint8_t srcArray[], const uint8_t size)
{
    for(int i=0; i<size; i++)
    {
        dstArray[i] = srcArray[i]; //XXX sizeof srcArray
    }
    
}

// void init2DArray(uint8_t rowNb, const uint8_t columnNb)
// {
//     for (int i=0; i< rowNb; i++)
//         for (int j=0; j< columnNb; j++)
//         {
//             codedPacket[i][j] = 0;
//         }
// }

#endif