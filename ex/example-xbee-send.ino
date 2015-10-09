#include <XBee.h>
#include <SoftwareSerial.h>

uint8_t payload[3] = {'A',1,5};
// le tranceiver xbee s1
XBee xbee = XBee();
Tx16Request bc = Tx16Request(0xffff, payload, sizeof(payload)); //broadcast
//Tx16Request bc = Tx16Request(0x17, payload, sizeof(payload));
TxStatusResponse txStatus = TxStatusResponse();
//Series 1 radios can be addressed by their 16-bit or 64-bit address. 

//XBeeResponse response = XBeeResponse();
// create reusable response objects for responses we expect to handle
//Rx16Response rx16 = Rx16Response();
//Rx64Response rx64 = Rx64Response();
//TxStatusResponse txStatus = TxStatusResponse();
// Variable for sending and receiving packets

uint8_t option = 0;
uint8_t data = 0;
boolean withLog = 1;
void setup()
{
  Serial.begin(38400); //baud rate
  // Initialiser le tranceiver
  xbee.setSerial(Serial);
  //xbee.begin(38400);
  //xbee.send(bc);
  //Serial.println("envoi ok");
  Serial.println("end setup...");

}
void loop()
{
  xbee.send(bc);
  /* test ACK */
  xbee.readPacket(10); 
  if (xbee.getResponse().isAvailable()) {
			   // got a response!
			  // should be a znet tx status            	
				if (xbee.getResponse().getApiId() == TX_STATUS_RESPONSE) {
					   xbee.getResponse().getTxStatusResponse(txStatus);
					   // get the delivery status, the fifth byte
					   if (txStatus.isSuccess()) {
						  // success.  time to celebrate
							  if (withLog) Serial.print("Positive ");
					   }   
					   else {
						if (withLog) Serial.print("Negative");
			// the remote XBee did not receive our packet. is it powered on?
			           } 
				}
  }
  else {
		if (withLog) Serial.print("Absent");
  }
  
  Serial.println("sending done.");
  delay(2000);
}
