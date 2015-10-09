// #---------------------------------------------------------------------------
// #  . Ichrak Amdouni,   Infine Project-Team, Inria Saclay
// #  Copyright 2015 Institut national de recherche en informatique et
// #  en automatique.  All rights reserved.  Distributed only with permission.
// #---------------------------------------------------------------------------
// 

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif
#include<XBee.h>
//#include <errno.h>
//#define UINT16_MAX 65535
//#include <inttypes.h> /* strtoimax */

#if 0
bool getMyShortAddress(XBee xbee, uint8_t []);
#endif
uint16_t getMyAddress16(XBee);
boolean getAck(XBee, boolean);
