

/*
EEpromCircularBuffer.h - circular buffer library for Arduino, data stored in EEprom
*/

#ifndef EEPROMBUFFER_h
#define EEPROMBUFFER_h
#include <inttypes.h>
#include <EEPROM.h>
#include "rtcStore.h"
enum actions { PUSH, POP, SHIFT, UNSHIFT };
template <typename T, int Size>

class EECircularBuffer {
public:


	EECircularBuffer() :
		writePos(0), readPos(0), count(0)
	{ 
		EEPROM.begin(Size * sizeof(T));
	}

	~EECircularBuffer() {}

	// push a value on to list
	void push(T value) {
		EEPROM.put(writePos * sizeof(T), value);
		if (++writePos == Size) {
			writePos = 0;
		}

		if (count == Size) {
			Serial.println("overwriting ");
			if (++readPos == Size) {
				readPos = 0;
			}
		}
		else {
			if (++count == 0) {
				Serial.println("emptied ");
				readPos = writePos;
			}
		}
	//	savePos(PUSH);
		EEPROM.commit();
	}

	// get last value pushed, remove from list
	T pop() {
		if (count <= 0) return nullT;
		--writePos;
		if (writePos < 0) {
			writePos = Size - 1;
		}
		T result = EEPROM.get(writePos * sizeof(T), nullT);
		count--;
	//	savePos(POP);
		return result;
	}

	// get earliest value pushed, remove from list
	T shift() {
		if (count <= 0) return nullT;
		T result = EEPROM.get(readPos * sizeof(T), nullT);
		++readPos;
		if (readPos >= Size) {
			readPos = 0;
		}
		count--;
	//	savePos(SHIFT);
		return result;
	}

	// push back value to last read position
	void unshift(T value) {
		if (readPos == 0) {
			readPos = Size;
		}
		--readPos;
		EEPROM.put(readPos * sizeof(T), value);
		if (count == Size) {
			if (writePos-- == 0) {
				writePos = Size - 1;
			}
		}
		else {
			if (count++ == 0) {
				writePos = readPos;
			}
		}
	//	savePos(UNSHIFT);
		EEPROM.commit();
	}

	int remain() const {
		return count;
	}

	void reset() {
		writePos = 0; readPos = 0;  count = 0;
	}
	void reset(int16_t w, int16_t r, int16_t c) {
		writePos = w; readPos = r;  count = c;
	}
	void savePos(int which) {
		Serial.print("Saving latest ee pointers: "); Serial.print(which); Serial.print(": ");
		Serial.print(writePos);	Serial.print(" ");	Serial.print(readPos); Serial.print(" ");	Serial.println(count);
		// save pointers in rtc in case of reset etc
		rtcData.data.EE_writePos= writePos; 
		rtcData.data.EE_readPos = readPos;
		rtcData.data.EE_count = count;


		//Serial.print("Writing IP: "); Serial.println(rtcData.data.addr);
		//Serial.print("Writing wp: "); Serial.println(rtcData.data.EE_writePos);
		//Serial.print("Writing rp: "); Serial.println(rtcData.data.EE_readPos);
		//Serial.print("Writing ct: "); Serial.println(rtcData.data.EE_count);
		//Serial.print(rtcData.data.EE_writePos);	Serial.print(" ");	Serial.print(rtcData.data.EE_readPos); Serial.print(" ");	Serial.println(rtcData.data.EE_count);
		putRTCmem();

	}
	// get (but leave) value at position 'pos'
	T peek(int16_t pos) {
		int16_t peekP = writePos - 1 - pos;
		if (peekP < 0)
			peekP = peekP + Size;
		//Serial.print("peeking "); Serial.println(peekP);
		T result = EEPROM.get(peekP * sizeof(T), nullT);
		return result;
	}

private:
	//T buf_[Size];
	int16_t writePos;
	int16_t readPos;
	int16_t count;
	T nullT;
};

#endif


#pragma once
