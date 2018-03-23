//#include "circBuffer.h"
#include "rtcStore.h"
#include "timeserver.h"
#include <EEPROM.h>
#include "eeCircBuffer.h"
#include <WiFiUdp.h>
#include <WiFiServer.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <ESP8266WiFiType.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiAP.h>
#include <DallasTemperature.h>
#include <OneWire.h>


#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

#include <ArduinoJson.h>
#include "config.h"
#include "victron.h"

#ifdef LOCAL_CE568
char endpoint[] = " /WebMap/WebMap.svc/SaveWeather HTTP/1.1";
char server[] = "CE568";  
#else
char endpoint[] = " /Service1.svc/SaveCamper HTTP/1.1";
char server[] = "www.timetrials.org.uk";
#endif



// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2
#define RED_LED_PIN 0
// connection to reset for deep sleep waking
#define SLEEP_WAKE 16
#define SMALLEST_REALTIME 1000000
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// arrays to hold device address
DeviceAddress insideTemp, outsideTemp, fridgeTemp;
unsigned long lastTempRequest = 0;
const int resolution = 9;
const int delayInMillis = 750 / (1 << (12 - resolution));
int idle = 0;
const int sleepSeconds = 30;
unsigned long  awakeTime = 0;
int awakeSeconds = 0;
int testCount = 0;
ulong realTime = 0;
ulong pseudoTime = 0;
bool realTimeJustEstablished = false;
bool realTimeValid() {
	return (realTime > SMALLEST_REALTIME);
}

struct VanData {
	ulong time;
//	Dallas temperatures
	int temp1;
	int temp2;
	int temp3;
// Victron data
	int battery_mV;
	int panel_mV;
	int load_mA;
	int power_W;
	int yield_kWh;
	int max_P;
	//int seq;

};
VanData nullData;
VanData currentValues;
//CircularBuffer<VanData,60> dataStore;
EECircularBuffer<VanData, 6> dataStore;


const byte maxlength = 16;
char ssid[maxlength + 1] = "XperiaZ2chrisF";
char pw[maxlength + 1] = "icespy1643";

void readstring(char* buf) {
  char buffer[maxlength + 1];
  for (byte index = 0; index < maxlength; index++)
    *(buffer + index) = 0;
  while (*buffer == 0) {
    Serial.readBytesUntil('\n', buffer, maxlength);
    Serial.print('.');
  }
  if (strlen(buffer) > 1) {
    strcpy(buf, buffer);
    Serial.println();
  }
  else {
    Serial.println("Default applied");
  }

}
char wifiBuf[64];

StaticJsonBuffer<200> jsonBuffer;
JsonObject& camper = jsonBuffer.createObject();

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
	for (uint8_t i = 0; i < 8; i++)
	{
		// zero pad the address if necessary
		if (deviceAddress[i] < 16) Serial.print("0");
		Serial.print(deviceAddress[i], HEX);
	}
}

void setup_ds18B20() {

	Serial.print("Dallas Temperature Library Version: ");
	Serial.println(DALLASTEMPLIBVERSION);
	Serial.println("\n");

	sensors.begin();
	Serial.print("Found ");
	Serial.print(sensors.getDeviceCount(), DEC);
	Serial.println(" devices.");

	// report parasite power requirements
	Serial.print("Parasite power is: ");
	if (sensors.isParasitePowerMode()) Serial.println("ON");
	else Serial.println("OFF");

	sensors.getAddress(insideTemp, 0);
	sensors.getAddress(outsideTemp, 1);
	sensors.getAddress(fridgeTemp, 2);

	sensors.setResolution(resolution);
	sensors.setWaitForConversion(false);
}

void redLed(int state) {
	pinMode(RED_LED_PIN, OUTPUT);
	digitalWrite(RED_LED_PIN, state-1);

}

int wifiStatus() {
    int status = WiFi.status();
  //Serial.print("WiFi status: "); Serial.println(status);
  return status;
}
bool startWiFi() {

	// todo: need checksum on credentials to ensure validity
	// todo: need pushbutton (or use different reset route? ) to reset credentials

	if (wifiStatus()  != WL_CONNECTED) {
	//if (1) {
		if (strlen(ssid) < 3 || strlen(pw) < 3) {

			Serial.print("WiFi SSID? ");
			readstring(ssid);
			Serial.print("WiFi password? ");
			readstring(pw);
		}

		//Serial.print("SSID is: ");
		//Serial.println(ssid);
		//Serial.print("password is: ");
		//Serial.println(pw);


		// We start by connecting to a WiFi network

		// ToDo: save IP address etc and try to use on subsequent connections : saving of time over using DHCP for each connection

		//// config static IP
		//IPAddress ip(192, 168, 1, 113); // where xx is the desired IP Address
		//IPAddress gateway(192, 168, 1, 254); // set gateway to match your network
		//Serial.print(F("Setting static ip to : "));
		//Serial.println(ip);
		//IPAddress subnet(255, 255, 255, 0); // set subnet mask to match your network
		//WiFi.config(ip, gateway, subnet);

		Serial.println();
		Serial.println();
		Serial.print("Connecting to ");
		Serial.println(ssid);

		WiFi.begin(ssid, pw);
	}
	wifiStatus();

	int count = 0;

	Serial.print("Connecting........");
	while (WiFi.status() != WL_CONNECTED) {
		redLed(HIGH);
		delay(10);
		redLed(LOW);
		delay(90);
		Serial.print(".");
		++count;
		if ((count % 20) == 0)
		{
			Serial.println(F("Trying connection"));
			ESP.wdtFeed();
		}
		if (count > 100) {
			Serial.println("Cannot connect to WiFi, giving up...");
			// save this set until we get a connection.
			//saveTemps();
			//WiFi.disconnect();

			return false;
		}
	}

	Serial.println("");
	Serial.print("WiFi connected, ");
	IPAddress ipaddr = WiFi.localIP();
	Serial.print("IP address: ");	Serial.println(ipaddr);
	rtcData.data.ip_addr = ipaddr;
	//// save the IP address for later use
	//putRTCmem();
	if (pseudoTime > 0) {
		// need to get new real time
		setupUDP();
		byte attempt = 5;
		while (attempt-- > 0) {
			if ((realTime = getTime()) > 0) {
				realTimeJustEstablished = true;
				pseudoTime = 0;
				break;
			}
			delay(5000);

		}
	}
    wifiStatus();
	return true;
}


void setup() {
  awakeTime = millis();

  Serial.begin(115200);
  delay(100);
  
  // first, check data stored in RTC
  // if OK, can use this to find last-used EEprom data, and last-used IP address
  if (getRTCmem()) {
	  Serial.println("RTC memory intact:");
	  printMemory();
	  Serial.println(rtcData.data.EE_writePos);
	  Serial.println(rtcData.data.EE_readPos);
	  Serial.println(rtcData.data.EE_count);
	  dataStore.reset(rtcData.data.EE_writePos, rtcData.data.EE_readPos, rtcData.data.EE_count);
	  pseudoTime = rtcData.data.pseudo_time;
	  if (pseudoTime > SMALLEST_REALTIME) // hasn't yet been used
		  pseudoTime = 0;
  }
  else {
	  Serial.println("RTC memory failed");
  }

  setupTime();
  if (wifiStatus() == WL_CONNECTED) {
	  byte attempt = 5;
	  while (attempt-- > 0) {
		  if ((realTime = getTime()) > 0) {
			  realTimeJustEstablished = true;
			  break;
		  }
		  delay(5000);
	  }
  }
  if (realTime == 0)
  {
	  Serial.println("Unknown real time, will use pseudo time for now");
  }
  setupVictron();
  setup_ds18B20();
  delay(10);
  awakeSeconds = 0;
  ESP.wdtEnable(0);
}

void loop() {

	RecvWithEndMarker();
	HandleNewData();
	ESP.wdtFeed();

	static unsigned long prev_millis = 0;
	if (millis() - prev_millis > 1000) {
		// i.e. every second
		ESP.wdtFeed();
		if (++awakeSeconds >= 3)
		{
			// should be enough time for Victron to send data
			startWiFi();
			PrintValues();
			getTemps();

			if (wifiStatus() == 3) {
				sendData();
			}
			WiFi.disconnect();
			sleep();
			awakeSeconds = 0;
		}
		prev_millis = millis();
	}
}


void convertDelay() {
	pinMode(ONE_WIRE_BUS, OUTPUT);
	digitalWrite(ONE_WIRE_BUS, HIGH);
	int tempReq = millis();
	while (millis() - tempReq < delayInMillis) // waited long enough??
	{
		delay(1);
	}
	pinMode(ONE_WIRE_BUS, INPUT);
}
void getTemps() {
	float t1, t2, t3;

	sensors.requestTemperatures();
	convertDelay();

	t1 = sensors.getTempC(insideTemp);
	t2 = sensors.getTempC(outsideTemp);
	t3 = sensors.getTempC(fridgeTemp);

	Serial.print("inside  = ");		Serial.println(t1);
	Serial.print("outside = ");		Serial.println(t2);
	Serial.print("fridge  = ");		Serial.println(t3);

	if (t1 < -50 || t1 > 84) {
		if (t2 < -50 || t2 > 84) {
			if (t3 < -50 || t3 > 84) {
				Serial.println("Invalid temperatures");
				return;

			}
		}
	}
	if (realTimeValid())	
	{
		Serial.print("real time = ");		Serial.println(realTime);
		currentValues.time = realTime;
	}
	else
	{
		Serial.println("Unknown realtime, using pseudo-time");
		Serial.print("pseudo time = ");		Serial.println(pseudoTime);
		currentValues.time = pseudoTime;
	}
	
	currentValues.temp1 = (int)(t1 * 10);
	currentValues.temp2 = (int)(t2 * 10);
	currentValues.temp3 = (int)(t3 * 10);
	if (victron_valid) {
		// only if Victron has been successfully connected this period
		currentValues.battery_mV = battery_mV;
		currentValues.load_mA = load_mA;
		currentValues.panel_mV = panel_mV;
		currentValues.power_W = power_W;
		currentValues.max_P = max_W;
		currentValues.yield_kWh = yield_kWh;
		victron_valid = false;
	}
	else {
		currentValues.battery_mV = 0;
		currentValues.load_mA = 0;
		currentValues.panel_mV = 0;
		currentValues.power_W = 0;
	}

	dataStore.push(currentValues);
	int stored = dataStore.remain();
	Serial.println();
	Serial.print(stored); Serial.println(" value sets stored");
	for (int rec = 5; rec >= 0; rec--)
	{
		VanData tt = dataStore.peek(rec);
		Serial.print(tt.time); Serial.print(":");
		Serial.print(tt.temp1); Serial.print("  ");
		Serial.print(tt.temp2); Serial.print("  ");
		Serial.print(tt.temp3); Serial.print("  ");
		Serial.print(tt.battery_mV); Serial.print("  ");
		Serial.print(tt.load_mA); Serial.print("  ");
		Serial.print(tt.panel_mV); Serial.print("  ");
		Serial.print(tt.power_W); Serial.print("  ");
		Serial.print(tt.yield_kWh); Serial.print("  ");
		Serial.print(tt.max_P); Serial.println("  ");
	}
	//if (++testCount >= 9)
	//{
	//	testCount = 0;
	//	Serial.println("Shifting out:");
	//	for (int rec = stored - 1; rec >= 0; rec--)
	//	{
	//		VanData tt = dataStore.shift();
	//		Serial.print(tt.time); Serial.print(":");
	//		Serial.print(tt.temp1); Serial.print("  ");
	//		Serial.print(tt.temp2); Serial.print("  ");
	//		Serial.print(tt.temp3); Serial.println("  ");
	//	}
	//}
}
void sendData() {

  int numRecords = dataStore.remain();
  int sentRecords = 0;
  VanData tt;
  ulong largestPseudoTime = 0;

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  while (true)
  {
	  // get records out, newest one first
	  tt = dataStore.pop();
	  Serial.print("stored time: "); Serial.println(tt.time);
	  if (tt.time == 0)
		  // no more records in buffer
		  break;
	  if (numRecords - sentRecords <= 0)
	  {
		  // shouldn't get here...
		  Serial.print(numRecords);  Serial.print("....false end of store...."); Serial.println(sentRecords);
		  break;
	  }
	  if (tt.time == 0xFFFFFFFF) {
		  // invalid records; delete storage
		  Serial.println("deleting old store....");
		  dataStore.reset();
		  break;
	  }
	  redLed(HIGH);

	 // tt = *ttp;
	  camper["vanT"] = tt.temp1;
	  camper["shadeT"] = tt.temp2;
	  camper["fridgeT"] = tt.temp3;

	  // data from Victron
	  camper["battV"] = tt.battery_mV;
	  camper["panelV"] = tt.panel_mV;
	  camper["panelP"] = tt.power_W;
	  camper["loadC"] = tt.load_mA;
	  camper["yield"] = tt.yield_kWh;
	  camper["maxP"] = tt.max_P;

	  // need to adjust realtime if pseudo time was in use
	  if (realTimeJustEstablished) {
		  if (tt.time < SMALLEST_REALTIME) {
			  if (largestPseudoTime == 0 ) {
				  largestPseudoTime = tt.time;
			  }
			  int timeOffset = (largestPseudoTime - tt.time + sleepSeconds / 60);
			  Serial.print("offset time: ");	  Serial.println(timeOffset);
			  tt.time = realTime - timeOffset;
			  Serial.print("adjusted time: ");	  Serial.println(tt.time);
		  }
	  }

	  camper["seq"] = tt.time;

	  ++sentRecords;
	  Serial.println();
	  Serial.print("sending record for time ");	  Serial.println(tt.time); 
	  Serial.print("connecting to ");	  Serial.println(server);
	  const int httpPort = 80;

	  if (!client.connect(server, httpPort)) {
		  Serial.println("connection failed");
		  dataStore.unshift(tt);
		  return;
	  }

	  // We now create a URI for the request
    Serial.println();
	  //Serial.print("Requesting URL: ");	  Serial.println(endpoint);
	  Serial.print("Sending value set ");	  Serial.println(numRecords - sentRecords);
	  int dataLen = camper.measureLength();
	  Serial.print("Data set length: ");	  Serial.println(dataLen);

	  // This will send the request to the server, one set for each temperature stored (usually just one)
	  client.print("POST "); client.println(endpoint);
	  client.print("Host: "); client.println(server);


	  client.println("Connection: close\r\nContent-Type: application/json");
	  sprintf(wifiBuf, "Content-Length: %u\r\n", dataLen);
	  client.println(wifiBuf);
	  camper.printTo(client);
	  camper.printTo(Serial);
	  unsigned long timeout = millis();
	  while (client.available() == 0) {
		  if (millis() - timeout > 5000) {
			  Serial.println(">>> Client Timeout !");
			  client.stop();
			  dataStore.unshift(tt);
			  return;
		  }
	  //while (!client.available()) {
		 // delay(1);

	  }
	  redLed(LOW);
	  // Read all the lines of the reply from server and print them to Serial
	  while (client.available()) {
		  String line = client.readStringUntil('\r');
		  Serial.print(line);
	  }

  }
  realTimeJustEstablished = false;
  client.stop();
  Serial.println();
  Serial.println("closing connection");
  wifiStatus();
}



void sleep() {

  unsigned long awake = millis() - awakeTime;
	Serial.print("Awake time: "); Serial.println(awake);

	long sleepTime = 1000L * sleepSeconds -awake;
	if (sleepTime < 1000)
		sleepTime = 1000;
    Serial.print("Modem sleep for "); Serial.print(sleepTime/1000); Serial.println(" seconds");
	//WiFi.forceSleepBegin(1000L * sleepTime);
	WiFi.forceSleepBegin();
	delay(sleepTime);
	WiFi.forceSleepWake();
    awakeTime = millis();
	delay(1);


	if (realTimeValid()) {
		//realTime += sleepSeconds / 60;
		realTime += 1;
	}
	else {
		//pseudoTime += sleepSeconds / 60;
		pseudoTime += 1;
		rtcData.data.pseudo_time = pseudoTime;
	}
}
