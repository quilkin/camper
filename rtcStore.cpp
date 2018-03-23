// 
// 
// 

#include "rtcStore.h"

rtcStore rtcData;

void putRTCmem() {
	// Update CRC32 of data
	rtcData.crc32 = calculateCRC32((uint8_t*)&rtcData.data, sizeof(rtcData.data) );
	// Write struct to RTC memory
	//Serial.print("Writing IP: "); Serial.println(rtcData.data.addr);
	//Serial.print("Writing wp: "); Serial.println(rtcData.data.EE_writePos);
	//Serial.print("Writing rp: "); Serial.println(rtcData.data.EE_readPos);
	//Serial.print("Writing ct: "); Serial.println(rtcData.data.EE_count);

	if (ESP.rtcUserMemoryWrite(0, (uint32_t*)&rtcData, sizeof(rtcData))) {
		//Serial.print("Writing "); Serial.print(sizeof(rtcData)); Serial.println(" bytes ");
		//printMemory();
		//Serial.println();
	}
}

bool getRTCmem() {

	// Read struct from RTC memory
	if (ESP.rtcUserMemoryRead(0, (uint32_t*)&rtcData, sizeof(rtcData))) {
		Serial.println("Read: ");
		printMemory();
		Serial.println();
		uint32_t crcOfData = calculateCRC32((uint8_t*)&rtcData+4, sizeof(rtcData)-4);
		Serial.print("CRC32 of data: ");
		Serial.println(crcOfData, HEX);
		Serial.print("CRC32 read from RTC: ");
		Serial.println(rtcData.crc32, HEX);
		if (crcOfData != rtcData.crc32) {
			Serial.println("CRC32 in RTC memory doesn't match CRC32 of data. Data is probably invalid!");
			// initialize the eedata
			rtcData.data.EE_count = 0;
			rtcData.data.EE_readPos = 0;
			rtcData.data.EE_writePos = 0;
			rtcData.data.ip_addr = 0x12345678;

			putRTCmem();

			return false;
		}
		else {
			Serial.println("CRC32 check ok, data is probably valid.");
			return true;
		}
	}
	Serial.println("RTC mem couldn't be found");
	return false;


}

uint32_t calculateCRC32(const uint8_t *data, size_t length) {
	uint32_t crc = 0xffffffff;
	while (length--) {
		uint8_t c = *data++;
		for (uint32_t i = 0x80; i > 0; i >>= 1) {
			bool bit = crc & 0x80000000;
			if (c & i) {
				bit = !bit;
			}
			crc <<= 1;
			if (bit) {
				crc ^= 0x04c11db7;
			}
		}
	}
	char buf[9];
	sprintf(buf, "%02X", crc);
//	Serial.print("crc: "); Serial.println(buf);
	return crc;
}

//prints all rtcData, including the leading crc32
void printMemory() {
	char buf[3];
	uint8_t *ptr = (uint8_t *)&rtcData;
	for (size_t i = 0; i < sizeof(rtcData); i++) {
		sprintf(buf, "%02X", ptr[i]);
		Serial.print(buf);
		if ((i + 1) % 32 == 0) {
			Serial.println();
		}
		else {
			Serial.print(" ");
		}
	}
	Serial.println();
}
