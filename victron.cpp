// 
// 
// 

#include "victron.h"
#include "config.h"
#include <SoftwareSerial.h>


// Serial variables
//#define rxPin 13
//#define txPin 5                                    // TX Not used
//SoftwareSerial victronSerial(rxPin, txPin);         // RX, TX Using Software Serial so we can use the hardware serial to check the ouput
													// via the USB serial provided by the NodeMCU.
char receivedChars[buffsize];                       // an array to store the received data
char tempChars[buffsize];                           // an array to manipulate the received data
char recv_label[num_keywords][label_bytes] = { 0 };  // {0} tells the compiler to initalize it with 0. 
char recv_value[num_keywords][value_bytes] = { 0 };  // That does not mean it is filled with 0's
char value[num_keywords][value_bytes] = { 0 };  // The array that holds the verified data
static byte blockindex = 0;
bool new_data = false;
bool victron_valid = false;
bool blockend = false;
//int awakeSeconds = 0;
SoftwareSerial victronSerial(rxPin, txPin);
//SoftwareSerial* victronSerialP;

char keywords[num_keywords][label_bytes] = {
	"PID",
	"FW",
	"SER#",
	"V",
	"I",
	"VPV",
	"PPV",
	"CS",
	"ERR",
	"LOAD",
	"IL",
	"H19",
	"H20",
	"H21",
	"H22",
	"H23",
	"HSDS",
	"Checksum"
};

// data to send to DB
int battery_mV;
int battery_mA;
int panel_mV;
int load_mA;
int power_W;
int yield_kWh;
int max_W;

void setupVictron() {
	//victronSerialP = new SoftwareSerial(rxPin, txPin);
	victronSerial.begin(19200);
}
//void startVictronSerial() {
//	if (victronSerialP == NULL) {
//		
//		victronSerialP = new SoftwareSerial(rxPin, txPin);
//		(*victronSerialP).begin(19200);
//		Serial.println("started SoftwareSerial");
//		//victronSerial = *victronSerialP;
//	}
//}
//void stopVictronSerial() {
//	if (victronSerialP != NULL) {
//		delete victronSerialP;
//		Serial.println("stopped SoftwareSerial");
//	}
//
//}
// Serial Handling
// ---
// This block handles the serial reception of the data in a 
// non blocking way. It checks the Serial line for characters and 
// parses them in fields. If a block of data is send, which always ends
// with "Checksum" field, the whole block is checked and if deemed correct
// copied to the 'value' array. 


void RecvWithEndMarker() {
	static byte ndx = 0;
	char endMarker = '\n';
	char rc;

	//if (victronSerialP == NULL) {
	//	Serial.println("Error:  no softwraeSerial");
	//	return;
	//}

	while (victronSerial.available() > 0 && new_data == false) {
		rc = victronSerial.read();
		if (rc != endMarker) {
			receivedChars[ndx] = rc;
			ndx++;
			if (ndx >= buffsize) {
				ndx = buffsize - 1;
			}
		}
		else {
			receivedChars[ndx] = '\0'; // terminate the string
			ndx = 0;
			new_data = true;
		}
		yield();
	}
}

void HandleNewData() {
	// We have gotten a field of data 
	if (new_data == true) {
		//Copy it to the temp array because parseData will alter it.
		strcpy(tempChars, receivedChars);
		ParseData();
		new_data = false;
		victron_valid = true;
		
	}
}

void ParseData() {
	char * strtokIndx; // this is used by strtok() as an index
					   //Serial.print(tempChars);
	strtokIndx = strtok(tempChars, "\t");      // get the first part - the label
											   // The last field of a block is always the Checksum
	if (strcmp(strtokIndx, "Checksum") == 0) {
		blockend = true;
	}
	strcpy(recv_label[blockindex], strtokIndx); // copy it to label

												// Now get the value
	strtokIndx = strtok(NULL, "\r");    // This continues where the previous call left off until '/r'.
	if (strtokIndx != NULL) {           // We need to check here if we don't receive NULL.
		strcpy(recv_value[blockindex], strtokIndx);
	}
	blockindex++;

	if (blockend) {
		// We got a whole block into the received data.
		// Check if the data received is not corrupted.
		// Sum off all received bytes should be 0;
		byte checksum = 0;
		for (int x = 0; x < blockindex; x++) {
			// Loop over the labels and value gotten and add them.
			// Using a byte so the the % 256 is integrated. 
			char *v = recv_value[x];
			char *l = recv_label[x];
			while (*v) {
				checksum += *v;
				v++;
			}
			while (*l) {
				checksum += *l;
				l++;
			}
			// Because we strip the new line(10), the carriage return(13) and 
			// the horizontal tab(9) we add them here again.  
			checksum += 32;
		}
		// Checksum should be 0, so if !0 we have correct data.
		if (!checksum) {
			// Since we are getting blocks that are part of a 
			// keyword chain, but are not certain where it starts
			// we look for the corresponding label. This loop has a trick
			// that will start searching for the next label at the start of the last
			// hit, which should optimize it. 
			int start = 0;
			for (int i = 0; i < blockindex; i++) {
				for (int j = start; (j - start) < num_keywords; j++) {
					if (strcmp(recv_label[i], keywords[j % num_keywords]) == 0) {
						// found the label, copy it to the value array
						strcpy(value[j], recv_value[i]);
						start = (j + 1) % num_keywords; // start searching the next one at this hit +1
						break;
					}
				}
			}
			//Serial.println(" Checksum OK");
		}
		else
			Serial.println(" Checksum err");
		// Reset the block index, and make sure we clear blockend.
		//Serial.println("Blockend OK");
		blockindex = 0;
		blockend = false;
	}
	//else
	//	Serial.println("Blockend false");
}
//
//void PrintEverySecond() {
//	static unsigned long prev_millis;
//	if (millis() - prev_millis > 1000) {
//		PrintValues();
//		//PrintAllValues();
//
//		if (++awakeSeconds >= 3)
//		{
//			Serial.println("Sleeping");
//			delay(60000);
//			awakeSeconds = 0;
//		}
//		prev_millis = millis();
//	}
//}

void PrintValues() {
	for (int i = 0; i < num_keywords; i++) {
		int data = atoi(value[i]);
		if (strcmp(keywords[i], "V") == 0) {
			Serial.print("Batt mV \t");
			battery_mV = data;
			Serial.println(data);
		}
		else if (strcmp(keywords[i], "I") == 0) {
			Serial.print("Batt mA \t");
			Serial.println(data);
		}
		else if (strcmp(keywords[i], "VPV") == 0) {
			Serial.print("Panel mV \t");
			panel_mV = data;
			Serial.println(data);
		}
		else if (strcmp(keywords[i], "PPV") == 0) {
			Serial.print("Panel W \t");
			power_W = data;
			Serial.println(data);
		}
		else if (strcmp(keywords[i], "IL") == 0) {
			Serial.print("current mA \t");
			load_mA = data;
			Serial.println(data);
		}
		else if (strcmp(keywords[i], "H20") == 0) {
			Serial.print("Yield kWh\t");
			yield_kWh = data;
			Serial.println(data);
		}
		else if (strcmp(keywords[i], "H21") == 0) {
			Serial.print("Max power W \t");
			max_W = data;
			Serial.println(data);
		}
		else if (strcmp(keywords[i], "FW") == 0) {
			Serial.print("Firmware\t");
			Serial.println(data);
		}

	}
}
