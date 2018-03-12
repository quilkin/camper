// victron.h

#ifndef _VICTRON_h
#define _VICTRON_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#define rxPin 13
#define txPin 5                                    // TX Not used

#include <SoftwareSerial.h>


//extern SoftwareSerial* victronSerialP;
extern SoftwareSerial victronSerial;
void setupVictron();
//void startVictronSerial();
//void stopVictronSerial();

void RecvWithEndMarker();
void HandleNewData();
void ParseData();
void PrintEverySecond();
void PrintValues();

extern bool victron_valid ;
extern int battery_mV;
extern int panel_mV;
extern int load_mA;
extern int power_W;
extern int yield_kWh;
extern int max_W;
#endif

