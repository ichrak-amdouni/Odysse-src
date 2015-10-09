#include "XBee.h"
// #include "XBeeCommands.h"
XBee xbee = XBee();

uint16_t neighborTable[50];
uint16_t myAddress = 0; 
//-----------------------------------------------------------------------------------------------
uint16_t getMyAddress(XBee xbee){ //returns hexadecimal address
    uint8_t myCmd[] = {'M','Y'};
    //uint16_t myAddress = 0;
    // set command to read back MY 16-bit node's short address
    AtCommandRequest atRequestMy = AtCommandRequest(myCmd);
    AtCommandResponse atResponse = AtCommandResponse();
    xbee.send(atRequestMy);
    if(xbee.readPacket(10000)){
        if (xbee.getResponse().getApiId() == AT_COMMAND_RESPONSE) {
            xbee.getResponse().getAtCommandResponse(atResponse);
            if (atResponse.isOk()){
                char my_part[4];
                String myAddr = "";
                // get each value, convert it to string and append to the MY addr
                for (int i = 0; i < atResponse.getValueLength(); i++) {
                    sprintf(my_part, "%lX", (unsigned long)(atResponse.getValue()[i]));
                    if (atResponse.getValue()[i]<=0xF) 
                        myAddr += 0;
                    myAddr += my_part;
                }
                //Serial.print("Node's short address is now: ");
                //Serial.println((char*)&myAddr[0]);  
                myAddress = (uint16_t)strtoul((const char*)&myAddr[0], (char**)0, 16); 
            } 
        }                                                     
    }
    return myAddress;  
}

void getMyNeighbors(XBee xbee){ 
     /* int timeout = 5000;  // default value of NT (if NT command fails)
      // get the Node Discover Timeout (NT) value and set to timeout
      /*uint8_t NT[] = {'N','T'};  // Node Discover Timeout
      AtCommandRequest request = AtCommandRequest(NT);
      AtCommandResponse response = AtCommandResponse();
      request.setCommand(NT);
      Serial.print("Sending command to the XBee ");
      xbee.send(request);
      Serial.println("");
      if (xbee.getResponse().getApiId() == AT_COMMAND_RESPONSE) {
        xbee.getResponse().getAtCommandResponse(response);
        if (response.isOk()) {
          if (response.getValueLength() > 0) {
            // NT response range should be from 0x20 - 0xFF, but
            // I see an inital byte set to 0x00, so grab the last byte
            timeout = response.getValue()[response.getValueLength() - 1] * 100;
            Serial.print("NT  value: ");
            Serial.print(response.getValue()[response.getValueLength() - 1], HEX);
            
          }
        }
      }
*/
    
    Serial.print("{'arduino-address':");
    Serial.print(myAddress, HEX);
    
	uint8_t myCmd[] = {'N','D'};
	// set command to read back MY 16-bit node's short address
	AtCommandRequest atRequestMy = AtCommandRequest(myCmd);
	AtCommandResponse atResponse = AtCommandResponse();
	xbee.send(atRequestMy);
        //Serial.print("liste: ");
    //uint16_t ad = getMyAddress16(xbee);
    
    //Serial.print ("'arduino-adress':");
        
     
    Serial.print(",'Neighbors': [");

    int i = 0;
    while (xbee.readPacket(10000)){
		if (xbee.getResponse().getApiId() == AT_COMMAND_RESPONSE) {
			xbee.getResponse().getAtCommandResponse(atResponse);
			if (atResponse.isOk()){
                // Serial.print(atResponse.getValueLength());
                
            
                // get each value, convert it to string and append to the MY addr
                //for (int i = 0; i < atResponse.getValueLength(); i++) {
                if (atResponse.getValueLength() > 1){ 
                    //Serial.print(atResponse.getValue()[1], HEX);
                    neighborTable[i] = atResponse.getValue()[1];
                    i++;
                    //Serial.print(", ");
                    //sprintf(my_part, "%lX", (unsigned long)(atResponse.getValue()[i]));
                }	
                if (atResponse.getValueLength() == 0){
                    break;
                    //Serial.print(", ");
                }
            } 
        }                                                   
    }
    int j = 0;
    for( j = 0; j < i; j++){
        Serial.print(neighborTable[j]);
        if (j < i - 1)
            Serial.print(", ");
    }
    
    Serial.print("] ");
    Serial.print("} ");   
    Serial.println();
}

//---------------------------------------------------------------------------------------------------

void setup()
{
  Serial.begin(38400);  
  Serial1.begin(38400); 
  Serial1.flush();
  xbee.setSerial(Serial1);
  myAddress = getMyAddress(xbee); 
  Serial.print("MY ADDR : ");
  Serial.println(myAddress);
  randomSeed(analogRead(0));
  //randomSeed(myAddress);
  delay(500);
  Serial1.flush();
}


void loop()
{
  //getMyNeighbors(xbee);
  int delayTime = 0;
  while(delayTime <= 0)
    delayTime = random(50000);
  Serial.println(delayTime);
  delay(delayTime);
  
  getMyNeighbors(xbee);
}
//atResponse.getValueLength()


