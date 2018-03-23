// timeserver.h

#ifndef _TIMESERVER_h
#define _TIMESERVER_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

bool setupTime();
ulong  getTime();
void setupUDP();

extern char ssid[]; 
extern char pw[]; 
extern char wifiBuf[];
#endif

