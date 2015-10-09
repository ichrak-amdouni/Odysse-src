// #---------------------------------------------------------------------------
// #  . Ichrak Amdouni,   Infine Project-Team, Inria Saclay
// #  Copyright 2015 Institut national de recherche en informatique et
// #  en automatique.  All rights reserved.  Distributed only with permission.
// #---------------------------------------------------------------------------
#include "XBeeCommands.h"

#if 0
bool getMyShortAddress(XBee xbee, uint8_t myAddress[2]){
  
  uint8_t myCmd[] = {'M','Y'};
  int i;
  AtCommandRequest atRequestMy = AtCommandRequest(myCmd);
  AtCommandResponse atResponse = AtCommandResponse();
  xbee.send(atRequestMy);
  if(xbee.readPacket(5000)){
    if (xbee.getResponse().getApiId() == AT_COMMAND_RESPONSE) {
      xbee.getResponse().getAtCommandResponse(atResponse);
      if (atResponse.isOk()){
        for(int i = 0; i < atResponse.getValueLength(); i++){
          myAddress[i] = atResponse.getValue()[i];
        }
      }
      
    }
  return true;
  }
  return false;
}
#endif

uint16_t getMyAddress16(XBee xbee){ //returns hexadecimal address
	uint8_t myCmd[] = {'M','Y'};
	uint16_t myAddress = 0;
	// set command to read back MY 16-bit node's short address
	AtCommandRequest atRequestMy = AtCommandRequest(myCmd);
	AtCommandResponse atResponse = AtCommandResponse();
	xbee.send(atRequestMy);
	if(xbee.readPacket(5000)){
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
	

boolean getAck(XBee xbee, boolean withLog){
	TxStatusResponse txStatus = TxStatusResponse();
	if(xbee.readPacket(10000)){ 
	//if (xbee.getResponse().isAvailable()) {
			   // got a response!
			  // should be a znet tx status            	
		if (xbee.getResponse().getApiId() == TX_STATUS_RESPONSE) {
			xbee.getResponse().getTxStatusResponse(txStatus);
			//xbee.getResponse().getZBTxStatusResponse(txStatus);
		// get the delivery status, the fifth byte
		if (txStatus.isSuccess()) {
			// success.  time to celebrate
			if (withLog) Serial.print("Positive ");
			return true;
		} 
		else{
			// the remote XBee did not receive our packet. is it powered on?
			if (withLog)
				Serial.print("Negative");
			return false;
			}
		}
	} 
	else 
		if (xbee.getResponse().isError()){
			if (withLog){
				Serial.print("Error reading packet, Code: ");  
				Serial.print(xbee.getResponse().getErrorCode());
			}
			return false;
		} 
		else
		{
			if (withLog) { // or flash error led
				Serial.print("local XBee did not provide a timely TX Status Response.");  
			}
			return false;
		}
	return false;
}
 