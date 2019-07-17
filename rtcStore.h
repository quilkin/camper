// rtcStore.h

#ifndef _RTCSTORE_h
#define _RTCSTORE_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

// Example: Storing struct data in RTC user rtcDataory
//
// Struct data with the maximum size of 512 bytes can be stored
// in the RTC user rtcDataory using the ESP-specifc APIs.
// The stored data can be retained between deep sleep cycles.
// However, the data might be lost after power cycling the ESP8266.
//
// Created Mar 30, 2016 by Macro Yau.
//
// This example code is in the public domain.

// CRC function used to ensure data validity
uint32_t calculateCRC32(const uint8_t *data, size_t length);

// helper function to dump memory contents as hex
void printMemory();

// Structure which will be stored in RTC memory.
// First field is CRC32, which is calculated based on the rest of structure contents.
// Any fields can go after CRC32.

struct vitalData {
	int16_t EE_writePos;
	int16_t EE_readPos;
	int16_t EE_count;
	uint32_t ip_addr;
	uint32_t pseudo_time;
	bool realTimeJustEstablished;
};

struct rtcStore {
	uint32_t crc32;
	vitalData data;
};

extern rtcStore rtcData;

bool getRTCmem();
void putRTCmem();
void printMemory();
#endif

