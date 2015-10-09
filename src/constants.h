long LEVEL_WAIT_PERIOD = 8000; //10000; // la période d'attente des messages d'initialisation

//uint8_t N = 1;  // nombre N de réponses désirées pour arreter l'envoi des beacons
//uint8_t MAX_SENT_NB = 1; 
uint8_t MAX_RCV_REPLY_NB = 1;

//suint8_t MAX_RCV_REPLY_NB = 3;

//long twait = 0; // la période d'attente de sélection  
//long WAIT_SELECT_PERIOD = 2*10000;  // le temps d'attente après l'envoi de la réponse beacon
long WAIT_DATA_PERIOD = 2000;
long MAX_BEACON_PERIOD = 10000;      // le temps maximum d'envoie de beacon
long MAX_SENT_BEACON_NB = 10;
//long RI = 200;       // intervalle entre l'envoie de deux beacons
long WAIT_REPLY_PERIOD = 200;      // intervalle entre l'envoie de deux beacons
long ACTIVE_PERIOD = 5000;   // équivalent à landa on
int XBEE_DELAY_BEFORE_SEND = 10; //20;
