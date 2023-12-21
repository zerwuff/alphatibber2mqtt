/*
Name:		Alpha2MQTT.ino
Created:	8/24/2022
Author:		Daniel Young

This file is part of Alpha2MQTT (A2M) which is released under GNU GENERAL PUBLIC LICENSE.
See file LICENSE or go to https://choosealicense.com/licenses/gpl-3.0/ for full license details.

Notes

First, go and customise options at the top of Definitions.h!
*/

// Supporting files
#include <Arduino.h>
#include "RegisterHandler.h"
#include "RS485Handler.h"
#include "Definitions.h"
#include "trigger/trigger.h"
#include "Alpha.h"

#if defined MP_ESP8266
#include <ESP8266WiFi.h>
#elif defined MP_ESP32
#include <WiFi.h>
#endif
#include <PubSubClient.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include "webrender.h"
#include "trigger/trigger.h"

const char* ssid = "wuppb" ;
const char* password = "bilder1234567";

// Device parameters
char _version[6] = "v1.24";

// WiFi parameters
WiFiClient _wifi;

// MQTT parameters
PubSubClient _mqtt(_wifi);

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 30000;

// I want to declare this once at a modular level, keep the heap somewhere in check.
//char _mqttPayload[MAX_MQTT_PAYLOAD_SIZE] = "";


// RS485 and AlphaESS functionality are packed up into classes
// to keep separate from the main program logic.
RS485Handler* _modBus;
RegisterHandler* _registerHandler;
//WebRender* webRender;
//Trigger* trigger;

// Wemos OLED Shield set up. 64x48
// Pins D1 D2 if ESP8266
// Pins GPIO22 and GPIO21 (SCL/SDA) with optional reset on GPIO13 if ESP32
Adafruit_SSD1306 _display(-1);//, &Wire,-1); // No RESET Pin
Alpha alpha(&_display,_registerHandler,&_mqtt);

char _debugOutput[100];


uint32_t freeMemory()
{

	return ESP.getFreeHeap();
}

/*
setupWifi Connect to WiFi
*/
void setupWifi()
{
	sprintf(_debugOutput, "Free Memory : %d bytes", freeMemory());
	Serial.println(_debugOutput);
	// We start by connecting to a WiFi network
#ifdef DEBUG
	sprintf(_debugOutput, "Connecting to %s", WIFI_SSID);
	Serial.println(_debugOutput);
#endif

	// Set up in Station Mode - Will be connecting to an access point
	WiFi.mode(WIFI_STA);

	// And connect to the details defined at the top
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

	// And continually try to connect to WiFi.  If it doesn't, the device will just wait here before continuing
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(250);
		alpha.updateOLED(false, "Connecting", "WiFi...", _version);
	}

	// Set the hostname for this Arduino
	WiFi.hostname(DEVICE_NAME);

	// Output some debug information
#ifdef DEBUG
	Serial.print("WiFi connected, IP is");
	Serial.print(WiFi.localIP());
#endif

	// Connected, so ditch out with blank screen
	alpha.updateOLED(false, "", "", _version);
}




void doSerialConnect(){
		// All for testing different baud rates to 'wake up' the inverter
	unsigned long knownBaudRates[7] = { 115200,  57600, 38400, 19200, 14400,  9600, 4800 };

	//unsigned long knownBaudRates[2] = { 9600, 4800 };

	bool gotResponse = false;
	modbusRequestAndResponseStatusValues result = modbusRequestAndResponseStatusValues::preProcessing;
	modbusRequestAndResponse response;
	char baudRateString[10] = "";
	int baudRateIterator = -1;

// Configure MQTT to the address and port specified above
	_mqtt.setServer(MQTT_SERVER, MQTT_PORT);
#ifdef DEBUG
	sprintf(_debugOutput, "About to request buffer");
	Serial.println(_debugOutput);
#endif

	int _maxPayloadSize = 0;
	int _bufferSize =  0 ;
	for (_bufferSize = (MAX_MQTT_PAYLOAD_SIZE + MQTT_HEADER_SIZE); _bufferSize >= MIN_MQTT_PAYLOAD_SIZE + MQTT_HEADER_SIZE; _bufferSize = _bufferSize - 1024)
	{
#ifdef DEBUG
		sprintf(_debugOutput, "Requesting a buffer of : %d bytes", _bufferSize);
		Serial.println(_debugOutput);
#endif

		sprintf(_debugOutput, "Free Memory : %d bytes", freeMemory());
		Serial.println(_debugOutput);

		if (_mqtt.setBufferSize(_bufferSize))
		{
			
			_maxPayloadSize = _bufferSize - MQTT_HEADER_SIZE;
#ifdef DEBUG
			sprintf(_debugOutput, "_bufferSize: %d,\r\n\r\n_maxPayload (Including null terminator): %d", _bufferSize, _maxPayloadSize);
			Serial.println(_debugOutput);
#endif
			


			// Example, 2048, if declared as 2048 is positions 0 to 2047, and position 2047 needs to be zero.  2047 usable chars in payload.		
			
			// Example, 2048, if declared as 2048 is positions 0 to 2047, and position 2047 needs to be zero.  2047 usable chars in payload.
			//_mqttPayload = new char[_maxPayloadSize];
			//emptyPayload();

			alpha.setMaxPayloadSize(_maxPayloadSize);
			alpha.setMqttPayload(_maxPayloadSize);
			alpha.emptyPayload();

			sprintf(_debugOutput, "Requested Buffer. Free Memory : %d bytes", freeMemory());
			Serial.println(_debugOutput);
			break;
		}
		else
		{
#ifdef DEBUG
			sprintf(_debugOutput, "Couldn't allocate buffer of %d bytes for mqtt buffer", _bufferSize);
			Serial.println(_debugOutput);
#endif
		}
	}
	

	// And any messages we are subscribed to will be pushed to the mqttCallback function for processing
	_mqtt.setCallback(alpha.mqttCallback);


	// Set up the serial for communicating with the MAX
	_modBus = new RS485Handler;
	_modBus->setDebugOutput(_debugOutput);
	// Set up the helper class for reading with reading registers
	_registerHandler = new RegisterHandler(_modBus);

	// Iterate known baud rates until we find a success
	

	while (!gotResponse)
	{
		// Starts at -1, so increment to 0 for example
		baudRateIterator++;

		// Go back to zero if beyond the bounds
		if (baudRateIterator > (sizeof(knownBaudRates) / sizeof(knownBaudRates[0])) - 1)
		{
			baudRateIterator = 0;
		}

		// Update the display
		sprintf(baudRateString, "%u", knownBaudRates[baudRateIterator]);

		alpha.updateOLED(false, "Test Bau1", baudRateString, "");
#ifdef DEBUG
		sprintf(_debugOutput, "About To Try Baud rate : %u", knownBaudRates[baudRateIterator]);
		Serial.println(_debugOutput);
#endif
		// Set the rate
		_modBus->setBaudRate(knownBaudRates[baudRateIterator]);

		alpha.updateOLED(false, "BR Set: ", baudRateString, "");

		// Ask for a reading
		result = _registerHandler->readHandledRegister(REG_SAFETY_TEST_RW_GRID_REGULATION, &response);
		if (result != modbusRequestAndResponseStatusValues::readDataRegisterSuccess)
		{
#ifdef DEBUG
			sprintf(_debugOutput, "Baud Rate Checker Problem: %s", response.statusMqttMessage);
			Serial.println(_debugOutput);
#endif
			alpha.updateOLED(false, "Test Bau2", baudRateString, response.displayMessage);

			// Delay a while before trying the next
			delay(10000);

		}
		else
		{
			// Excellent, baud rate is set in the class, we got a response.. get out of here
			gotResponse = true;
		}
	}

	// Get the serial number (especially prefix for error codes)
	alpha.getSerialNumber();

	// Connect to MQTT
	alpha.mqttReconnect();

	alpha.updateOLED(false, "", "", _version);
}
/*
loop

The loop function runs overand over again until power down or reset
*/
void loop()
{

	Serial.println("Begin Loop");
	
	// Refresh LED Screen, will cause the status asterisk to flicker
	alpha.updateOLED(true, "", "", "");

	// Make sure WiFi is good
	if (WiFi.status() != WL_CONNECTED)
	{
		setupWifi();
	}

	Serial.println("free memory: ");
	Serial.print(freeMemory());
	
	
	 // make sure mqtt is still connected
	 if ((!_mqtt.connected()) || !_mqtt.loop())
	 {
		Serial.println("free memory, doing some reconnect: ");
		Serial.print(freeMemory());
		alpha.mqttReconnect();
	 }

	 // Check and display the runstate on the display
	 alpha.updateRunstate();

	 // Read and transmit all configured data to MQTT
	 alpha.sendData();

	
	// Force Restart?
#ifdef FORCE_RESTART
	if (checkTimer(&autoReboot, FORCE_RESTART_HOURS * 60 * 60 * 1000))
	{
		ESP.restart();
	}
#endif



}

/*
setup

The setup function runs once when you press reset or power the board
*/
void setup()
{
	// Set up serial for debugging using an appropriate baud rate
	// This is for communication with the development environment, NOT the Alpha system
	// See Definitions.h for this.
	Serial.begin(9600);


	// Configure LED for output
	pinMode(LED_BUILTIN, OUTPUT);
	
	sprintf(_debugOutput, "Free Memory : %d bytes", freeMemory());
	Serial.println(_debugOutput);

	// Display time
	_display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize OLED with the I2C addr 0x3C (for the 64x48)
	_display.clearDisplay();
	_display.display();
	alpha.updateOLED(false, "", "", _version);


	// Bit of a delay to give things time to kick in
	delay(100);


	// Configure WIFI
	setupWifi();
	Serial.println("Setup Wifi Done.");

	sprintf(_debugOutput, "Free Memory : %d bytes", freeMemory());
	Serial.println(_debugOutput);

	//trigger = new Trigger;
    //webRender = new WebRender;
    
	Serial.println("Setup WebRenderer Done.");
	
	sprintf(_debugOutput, "Free Memory : %d bytes", freeMemory());
	Serial.println(_debugOutput);

	//webRender->setTrigger(trigger);
	//webRender->setupWebInterface();
	
	Serial.println("Setup WebInterface  Done.");

	delay(1000);

	doSerialConnect();
}
























/*
sendMqtt

Sends whatever is in the modular level payload to the specified topic.
*/
void sendMqtt(char *topic)
{
	// Attempt a send
	if (!_mqtt.publish(topic, alpha.getMqttPayload()))
	{
#ifdef DEBUG
		sprintf(_debugOutput, "MQTT publish failed to %s", topic);
		Serial.println(_debugOutput);
		Serial.println(alpha.getMqttPayload());
#endif
	}
	else
	{
#ifdef DEBUG
		//sprintf(_debugOutput, "MQTT publish success");
		//Serial.println(_debugOutput);
#endif
	}

	// Empty payload for next use.
	alpha.emptyPayload();
	return;
}


/*
mqttCallback()

*/

