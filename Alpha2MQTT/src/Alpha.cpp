#include "Alpha.h" 

	// Device parameters
	static char _version[6] = "v1.24";

    // OLED variables
    static char _oledOperatingIndicator = '*';
    static char _oledLine2[OLED_CHARACTER_WIDTH] = "";
    static char _oledLine3[OLED_CHARACTER_WIDTH] = "";	
    static char _oledLine4[OLED_CHARACTER_WIDTH] = "";


  // Buffer Size (and therefore payload size calc)
    static int _maxPayloadSize;
    char* _mqttPayload;//[MAX_MQTT_PAYLOAD_SIZE] = "";

    static PubSubClient* _mqtt;
    static Adafruit_SSD1306* _display;
    static RegisterHandler* _registerHandler ;


    // Fixed char array for messages to the serial port
    static char _debugOutput[100];


Alpha::Alpha(Adafruit_SSD1306* disp, RegisterHandler* _regHandler,PubSubClient* mqtt){
	_display =  disp;
	_registerHandler =  _regHandler;
	_mqtt = mqtt;
}
/*
emptyPayload

Clears every char so end of string can be easily found
*/
void Alpha::emptyPayload()
{
	for (int i = 0; i < _maxPayloadSize; i++)
	{
		_mqttPayload[i] = '\0';
	}
}


/*
checkTimer

Check to see if the elapsed interval has passed since the passed in millis() value. If it has, return true and update the lastRun.
Note that millis() overflows after 50 days, so we need to deal with that too... in our case we just zero the last run, which means the timer
could be shorter but it's not critical... not worth the extra effort of doing it properly for once in 50 days.
*/
bool Alpha::checkTimer(unsigned long *lastRun, unsigned long interval)
{
	unsigned long now = millis();

	if (*lastRun > now)
		*lastRun = 0;

	if (now >= *lastRun + interval)
	{
		*lastRun = now;
		return true;
	}

	return false;
}


/*
updateOLED

Update the OLED. Use "NULL" for no change to a line or "" for an empty line.
Three parameters representing each of the three lines available for status indication - Top line functionality fixed
*/
void Alpha::updateOLED(bool justStatus, const char* line2, const char* line3, const char* line4)
{
	static unsigned long updateStatusBar = 0;


	_display->clearDisplay();
	_display->setTextSize(1);
	_display->setTextColor(WHITE);
	_display->setCursor(0, 0);

	char line1Contents[OLED_CHARACTER_WIDTH];
	char line2Contents[OLED_CHARACTER_WIDTH];
	char line3Contents[OLED_CHARACTER_WIDTH];
	char line4Contents[OLED_CHARACTER_WIDTH];

	// Ensure only dealing with 10 chars passed in, and null terminate.
	if (strlen(line2) > OLED_CHARACTER_WIDTH - 1)
	{
		strncpy(line2Contents, line2, OLED_CHARACTER_WIDTH - 1);
		line2Contents[OLED_CHARACTER_WIDTH - 1] = 0;
	}
	else
	{
		strcpy(line2Contents, line2);
	}


	if (strlen(line3) > OLED_CHARACTER_WIDTH - 1)
	{
		strncpy(line3Contents, line3, OLED_CHARACTER_WIDTH - 1);
		line3Contents[OLED_CHARACTER_WIDTH - 1] = 0;
	}
	else
	{
		strcpy(line3Contents, line3);
	}


	if (strlen(line4) > OLED_CHARACTER_WIDTH - 1)
	{
		strncpy(line4Contents, line4, OLED_CHARACTER_WIDTH - 1);
		line4Contents[OLED_CHARACTER_WIDTH - 1] = 0;
	}
	else
	{
		strcpy(line4Contents, line4);
	}

	// Only update the operating indicator once per half second.
	if (checkTimer(&updateStatusBar, UPDATE_STATUS_BAR_INTERVAL))
	{
		// Simply swap between space and asterisk every time we come here to give some indication of activity
		_oledOperatingIndicator = (_oledOperatingIndicator == '*') ? ' ' : '*';
	}

	// There's ten characters we can play with, width wise.
	sprintf(line1Contents, "%s%c%c%c", "A2M    ", _oledOperatingIndicator, (WiFi.status() == WL_CONNECTED ? 'W' : ' '), (_mqtt->connected() && _mqtt->loop() ? 'M' : ' ') );
	_display->println(line1Contents);


	// Next line

	_display->setCursor(0, 12);
	if (!justStatus)
	{
		_display->println(line2Contents);
		strcpy(_oledLine2, line2Contents);
	}
	else
	{
		_display->println(_oledLine2);
	}

	_display->setCursor(0, 24);
	if (!justStatus)
	{
		_display->println(line3Contents);
		strcpy(_oledLine3, line3Contents);
	}
	else
	{
		_display->println(_oledLine3);
	}

	_display->setCursor(0, 36);
	if (!justStatus)
	{
		_display->println(line4Contents);
		strcpy(_oledLine4, line4Contents);
	}
	else
	{
		_display->println(_oledLine4);
	}
	// Refresh the display
	_display->display();
}


modbusRequestAndResponseStatusValues Alpha::addToPayload(char* addition)
{
	int targetRequestedSize = strlen(_mqttPayload) + strlen(addition);

	// If max payload size is 2048 it is stored as (0-2047), however character 2048  (position 2047) is null terminator so 2047 chars usable usable
	if (targetRequestedSize > _maxPayloadSize - 1)
	{
		// Safely print using snprintf
		snprintf(_mqttPayload, _maxPayloadSize, "{\	r\n    \"mqttError\": \"Length of payload exceeds %d bytes.  Length would be %d bytes.\"\r\n}", _maxPayloadSize - 1, targetRequestedSize);

		return modbusRequestAndResponseStatusValues::payloadExceededCapacity;
	}
	else
	{
		// Add to the payload by sprintf back on itself with the addition
		sprintf(_mqttPayload, "%s%s", _mqttPayload, addition);

		return modbusRequestAndResponseStatusValues::addedToPayload;
	}
}


/*
getSerialNumber

Display on load to demonstrate connectivty and send the prefix into RegisterHandler as
some system fault descriptions depend on knowing whether an AL based or AE based inverter.
*/
modbusRequestAndResponseStatusValues Alpha::getSerialNumber()
{
	static unsigned long lastRun = 0;
	modbusRequestAndResponseStatusValues result = modbusRequestAndResponseStatusValues::preProcessing;
	modbusRequestAndResponse response;

	char oledLine3[OLED_CHARACTER_WIDTH];
	char oledLine4[OLED_CHARACTER_WIDTH];

	// Get the serial number
	result = _registerHandler->readHandledRegister(REG_SYSTEM_INFO_R_EMS_SN_BYTE_1_2, &response);


	if (result == modbusRequestAndResponseStatusValues::readDataRegisterSuccess)
	{
		// strncpy doesn't null terminate.
		strncpy(oledLine3, &response.dataValueFormatted[0], 10);
		oledLine3[10] = 0;
		strncpy(oledLine4, &response.dataValueFormatted[10], 5);
		oledLine4[5] = 0;
		updateOLED(false, "Hello", oledLine3, oledLine4);
#ifdef DEBUG
		sprintf(_debugOutput, "Alpha Serial Number: %s", response.dataValueFormatted);
		Serial.println(_debugOutput);
#endif

		_registerHandler->setSerialNumberPrefix(response.dataValueFormatted[0], response.dataValueFormatted[1]);
	}
	else
	{
		updateOLED(false, "Alpha sys", "not known", "");
	}

	delay(4000);

	//Flash the LED
	digitalWrite(LED_BUILTIN, LOW);
	delay(4);
	digitalWrite(LED_BUILTIN, HIGH);

	return result;
}

int Alpha::getMaxPayloadSize(){
	return _maxPayloadSize;
}

/*
updateRunstate

Determines a few things about the sytem and updates the display
Things updated - Dispatch state discharge/charge, battery power, battery percent
*/
void Alpha::updateRunstate()
{
	
	static unsigned long lastRun = 0;
	modbusRequestAndResponse response;
	modbusRequestAndResponseStatusValues request;

	char runningMode[OLED_CHARACTER_WIDTH] = "Other";
	char batteryPower[OLED_CHARACTER_WIDTH] = "";
	char batterySOC[OLED_CHARACTER_WIDTH] = "";



	if (checkTimer(&lastRun, RUNSTATE_INTERVAL))
	{
		//Flash the LED
		digitalWrite(LED_BUILTIN, LOW);
		delay(4);
		digitalWrite(LED_BUILTIN, HIGH);


		// Get Dispatch Start - Is Alpha2MQTT controlling the inverter?
		request = _registerHandler->readHandledRegister(REG_DISPATCH_RW_DISPATCH_START, &response);
		if (request == modbusRequestAndResponseStatusValues::readDataRegisterSuccess)
		{
			if (response.unsignedShortValue == DISPATCH_START_START)
			{
				// Yep, so get the mode.  And is it state of charge control or is the user using a different dispatch mode?
				request = _registerHandler->readHandledRegister(REG_DISPATCH_RW_DISPATCH_MODE, &response);
				if (request == modbusRequestAndResponseStatusValues::readDataRegisterSuccess)
				{
					if (response.unsignedShortValue == DISPATCH_MODE_STATE_OF_CHARGE_CONTROL)
					{
						// Yep, SOC, so determine if charging or charging by looking at power
						request = _registerHandler->readHandledRegister(REG_DISPATCH_RW_ACTIVE_POWER_1, &response);
						if (request == modbusRequestAndResponseStatusValues::readDataRegisterSuccess)
						{
							if (response.unsignedShortValue == DISPATCH_MODE_STATE_OF_CHARGE_CONTROL)
							{
								if (response.signedIntValue < 32000)
								{
									strcpy(runningMode, "Charge");
								}
								else if (response.signedIntValue > 32000)
								{
									strcpy(runningMode, "Discharge");
								}
								else
								{
									strcpy(runningMode, "Hold");
								}
							}
						}
					}
				}
			}
			else
			{
				strcpy(runningMode, "Normal");
			}
		}

		// Get battery power for line 3
		if (request == modbusRequestAndResponseStatusValues::readDataRegisterSuccess)
		{
			request = _registerHandler->readHandledRegister(REG_BATTERY_HOME_R_BATTERY_POWER, &response);
			if (request == modbusRequestAndResponseStatusValues::readDataRegisterSuccess)
			{
				sprintf(batteryPower, "Bat:%dW", response.signedShortValue);
			}
		}

		// And percent for line 4
		if (request == modbusRequestAndResponseStatusValues::readDataRegisterSuccess)
		{
			request = _registerHandler->readHandledRegister(REG_BATTERY_HOME_R_SOC, &response);
			if (request == modbusRequestAndResponseStatusValues::readDataRegisterSuccess)
			{
				sprintf(batterySOC, "%0.02f%%", response.unsignedShortValue * 0.1);
			}
		}

		if (request == modbusRequestAndResponseStatusValues::readDataRegisterSuccess)
		{
			updateOLED(false, runningMode, batteryPower, batterySOC);
		}
		else
		{
#ifdef DEBUG
			Serial.println(response.statusMqttMessage);
#endif
			updateOLED(false, "", "", response.displayMessage);
		}
	}
}



void Alpha::sendDataFromAppropriateArray(mqttState* registerArray, int numberOfRegisters, char* topic)
{
	int	l = 0;

	// For getting back out of flash - Storing these arrays in PROGMEM so they don't use valuable RAM.
	mqttState singleRegister;

	// For storing results
	modbusRequestAndResponseStatusValues result;
	modbusRequestAndResponseStatusValues resultAddedToPayload;

	emptyPayload();

	resultAddedToPayload = addToPayload("{\r\n");
	if (resultAddedToPayload == modbusRequestAndResponseStatusValues::addedToPayload)
	{
		for (l = 0; l < numberOfRegisters; l++)
		{
			memcpy_P(&singleRegister.registerAddress, &registerArray[l].registerAddress, 2);
			strcpy_P(singleRegister.mqttName, registerArray[l].mqttName);

			result = addStateInfo(singleRegister.registerAddress, singleRegister.mqttName, l < (numberOfRegisters - 1), resultAddedToPayload);
			
			if (resultAddedToPayload == modbusRequestAndResponseStatusValues::payloadExceededCapacity)
			{
				// If the response to addStateInfo is payload exceeded get out, We will permit failing registers to carry on and try the next one.
				break;
			}
		}
		if (resultAddedToPayload != modbusRequestAndResponseStatusValues::payloadExceededCapacity)
		{
			// Footer, if we haven't already bombed out with payload issues
			resultAddedToPayload = addToPayload("}");
		}

		// And send
		sendMqtt(topic);
	}
	else
	{
#ifdef DEBUG
		Serial.println("Invalid buffer");
#endif
	}
}



/*
addStateInfo

Query the handled register in the usual way, and add the cleansed output to the buffer
*/
modbusRequestAndResponseStatusValues Alpha::addStateInfo(uint16_t registerAddress, char* registerName, bool addComma, modbusRequestAndResponseStatusValues& resultAddedToPayload)
{
	char stateAddition[128] = ""; // 128 should cover individual additions to the payload
	char addQuote = false;
	modbusRequestAndResponse response;
	modbusRequestAndResponseStatusValues result = modbusRequestAndResponseStatusValues::preProcessing;

	// Read the register
	result = _registerHandler->readHandledRegister(registerAddress, &response);

	if (result == modbusRequestAndResponseStatusValues::readDataRegisterSuccess)
	{
		// Add a quote if the return data type is character or has been converted from lookup to description.
		addQuote = (response.returnDataType == modbusReturnDataType::character || response.hasLookup);

		sprintf(stateAddition, "    \"%s\": %s%s%s%s\r\n", registerName, addQuote ? "\"" : "", response.dataValueFormatted, addQuote ? "\"" : "", addComma ? "," : "");

		/*
		ABC,
		1234 LEN
		0123
		*/

		// Let the onward process also know if the buffer failed.
		resultAddedToPayload = addToPayload(stateAddition);
	}
	else
	{
#ifdef DEBUG
		sprintf(_debugOutput, "Failed to addStateInfo for: %u, Result was: %d", registerAddress, result);
		Serial.println(_debugOutput);
#endif
	}
	return result;
}


/*
sendData

Runs once every loop, checks to see if time periods have elapsed to allow the schedules to run.
Each time, the appropriate arrays are iterated, processed and added to the payload.
*/
void Alpha::sendData()
{
	static unsigned long lastRunTenSeconds = 0;
	static unsigned long lastRunOneMinute = 0;
	static unsigned long lastRunFiveMinutes = 0;
	static unsigned long lastRunOneHour = 0;
	static unsigned long lastRunOneDay = 0;
	int numberOfRegisters;
	// Update all parameters and send to MQTT.
	if (checkTimer(&lastRunTenSeconds, STATUS_INTERVAL_TEN_SECONDS))
	{
		numberOfRegisters = sizeof(_mqttTenSecondStatusRegisters) / sizeof(struct mqttState);
		sendDataFromAppropriateArray(_mqttTenSecondStatusRegisters, numberOfRegisters, DEVICE_NAME MQTT_MES_STATE_SECOND_TEN);
	}

	// Update all parameters and send to MQTT.
	if (checkTimer(&lastRunOneMinute, STATUS_INTERVAL_ONE_MINUTE))
	{
		numberOfRegisters = sizeof(_mqttOneMinuteStatusRegisters) / sizeof(struct mqttState);
		sendDataFromAppropriateArray(_mqttOneMinuteStatusRegisters, numberOfRegisters, DEVICE_NAME MQTT_MES_STATE_MINUTE_ONE);
	}

	// Update all parameters and send to MQTT.
	if (checkTimer(&lastRunFiveMinutes, STATUS_INTERVAL_FIVE_MINUTE))
	{
		numberOfRegisters = sizeof(_mqttFiveMinuteStatusRegisters) / sizeof(struct mqttState);
		sendDataFromAppropriateArray(_mqttFiveMinuteStatusRegisters, numberOfRegisters, DEVICE_NAME MQTT_MES_STATE_MINUTE_FIVE);
	}

	// Update all parameters and send to MQTT.
	if (checkTimer(&lastRunOneHour, STATUS_INTERVAL_ONE_HOUR))
	{
		numberOfRegisters = sizeof(_mqttOneHourStatusRegisters) / sizeof(struct mqttState);
		sendDataFromAppropriateArray(_mqttOneHourStatusRegisters, numberOfRegisters, DEVICE_NAME MQTT_MES_STATE_HOUR_ONE);
	}

	// Update all parameters and send to MQTT.
	if (checkTimer(&lastRunOneDay, STATUS_INTERVAL_ONE_DAY))
	{
		numberOfRegisters = sizeof(_mqttOneDayStatusRegisters) / sizeof(struct mqttState);
		sendDataFromAppropriateArray(_mqttOneDayStatusRegisters, numberOfRegisters, DEVICE_NAME MQTT_MES_STATE_DAY_ONE);
	}
}

void Alpha::setMqttPayload(int newMaxPayloadSize){
	_mqttPayload = new char[newMaxPayloadSize];
}


char* Alpha::getMqttPayload(){
	return _mqttPayload ;
}

void Alpha::setMaxPayloadSize(int maxPayloadSize){
	_maxPayloadSize = maxPayloadSize;
} 

// This function is executed when an MQTT message arrives on a topic that we are subscribed to.

void Alpha::mqttCallback(char* topic, byte* message, unsigned int length)
{
	modbusRequestAndResponseStatusValues result = modbusRequestAndResponseStatusValues::preProcessing;
	modbusRequestAndResponse response;
	modbusRequestAndResponseStatusValues resultDispatch = modbusRequestAndResponseStatusValues::preProcessing;
	modbusRequestAndResponse responseDispatch;
	modbusRequestAndResponseStatusValues resultAddToPayload = modbusRequestAndResponseStatusValues::addedToPayload;

	char stateAddition[256] = ""; // 256 should cover individual additions to be added to the payload.
	char topicResponse[100] = ""; // 100 should cover a topic name


	// Variables for new JSON parser
	int iSegNameCounter;
	int iSegValueCounter;
	int iPairNameCounter;
	int iPairValueCounter;
	int iCleanCounter;
	int iCounter;

	// All are emptied on creation as new arrays will just tend to have garbage in which would be recognised as actual content.
	char pairNameRaw[32] = "";
	char pairNameClean[32] = "";
	char pairValueRaw[32] = "";
	char pairValueClean[32] = "";
	char registerAddress[32] = "";
	char dataBytes[32] = "";
	char value[32] = "";
	char watts[32] = "";
	char duration[32] = "";
	char socPercent[32] = "";
	char startPos[32] = "";
	char endPos[32] = "";

	// Convert incoming values into native data types for sending
	uint16_t registerAddressConverted;
	uint16_t singleRegisterValueConverted;
	uint32_t dataRegisterValueConverted;
	uint32_t chargeDischargeWattsConverted;
	uint32_t durationSecondsConverted;
	uint16_t batterySocPercentConverted;
	uint16_t startPosConverted;
	uint16_t endPosConverted;

	uint8_t registerCount;

	// Bytes are received back as base ten, 0-255, so four chars to account for null terminator
	char rawByteForPayload[4] = "";
	char rawDataForPayload[100] = "";
	char mqttIncomingPayload[128] = ""; // Should be enough to cover request JSON.

	mqttSubscriptions subScription = mqttSubscriptions::unknown;

	// For getting back out of flash
	mqttState singleRegister;

	// Start by clearing out the payload
	emptyPayload();


#ifdef DEBUG
	sprintf(_debugOutput, "Topic: %s", topic);
	Serial.println(_debugOutput);
#endif


	// Get the payload
	for (int i = 0; i < length; i++)
	{
		mqttIncomingPayload[i] = message[i];
	}
#ifdef DEBUG
	Serial.println("Payload:");
	Serial.println(mqttIncomingPayload);
#endif


	// Get an easy to use subScription type for later
	if (strcmp(topic, DEVICE_NAME MQTT_SUB_REQUEST_READ_HANDLED_REGISTER) == 0)
	{
		subScription = mqttSubscriptions::readHandledRegister;
		strcpy(topicResponse, DEVICE_NAME MQTT_MES_RESPONSE_READ_HANDLED_REGISTER) == 0;
	}
	else if (strcmp(topic, DEVICE_NAME MQTT_SUB_REQUEST_READ_RAW_REGISTER) == 0)
	{
		subScription = mqttSubscriptions::readRawRegister;
		strcpy(topicResponse, DEVICE_NAME MQTT_MES_RESPONSE_READ_RAW_REGISTER);
	}
	else if (strcmp(topic, DEVICE_NAME MQTT_SUB_REQUEST_WRITE_RAW_SINGLE_REGISTER) == 0)
	{
		subScription = mqttSubscriptions::writeRawSingleRegister;
		strcpy(topicResponse, DEVICE_NAME MQTT_MES_RESPONSE_WRITE_RAW_SINGLE_REGISTER);
	}
	else if (strcmp(topic, DEVICE_NAME MQTT_SUB_REQUEST_WRITE_RAW_DATA_REGISTER) == 0)
	{
		subScription = mqttSubscriptions::writeRawDataRegister;
		strcpy(topicResponse, DEVICE_NAME MQTT_MES_RESPONSE_WRITE_RAW_DATA_REGISTER);
	}
	else if (strcmp(topic, DEVICE_NAME MQTT_SUB_REQUEST_READ_HANDLED_REGISTER_ALL) == 0)
	{
		subScription = mqttSubscriptions::readHandledRegisterAll;
		strcpy(topicResponse, DEVICE_NAME MQTT_SUB_RESPONSE_READ_HANDLED_REGISTER_ALL);
	}
	else if (strcmp(topic, DEVICE_NAME MQTT_SUB_REQUEST_SET_CHARGE) == 0)
	{
		subScription = mqttSubscriptions::setCharge;
		strcpy(topicResponse, DEVICE_NAME MQTT_SUB_RESPONSE_SET_CHARGE);
	}
	else if (strcmp(topic, DEVICE_NAME MQTT_SUB_REQUEST_SET_DISCHARGE) == 0)
	{
		subScription = mqttSubscriptions::setDischarge;
		strcpy(topicResponse, DEVICE_NAME MQTT_SUB_RESPONSE_SET_DISCHARGE);
	}
	else if (strcmp(topic, DEVICE_NAME MQTT_SUB_REQUEST_SET_NORMAL) == 0)
	{
		subScription = mqttSubscriptions::setNormal;
		strcpy(topicResponse, DEVICE_NAME MQTT_SUB_RESPONSE_SET_NORMAL);
	}
	else
	{
		mqttSubscriptions::unknown;
		result = modbusRequestAndResponseStatusValues::notValidIncomingTopic;
	}

	if (result == modbusRequestAndResponseStatusValues::preProcessing)
	{
		if (length == 0)
		{
			// We won't be doing anything if no payload
			result = modbusRequestAndResponseStatusValues::noMQTTPayload;
			strcpy(response.statusMqttMessage, MODBUS_REQUEST_AND_RESPONSE_NO_MQTT_PAYLOAD_MQTT_DESC);
		}
	}

	if (result == modbusRequestAndResponseStatusValues::preProcessing)
	{
		// Rudimentary JSON parser here, saves on using a library
		// Go through character by character
		for (iCounter = 0; iCounter < length; iCounter++)
		{
			// Find a colon
			if (mqttIncomingPayload[iCounter] == ':')
			{
				// Everything to left is name until reached the start, a comma or a left brace.
				for (iSegNameCounter = iCounter - 1; iSegNameCounter >= 0; iSegNameCounter--)
				{
					if (mqttIncomingPayload[iSegNameCounter] == ',' || mqttIncomingPayload[iSegNameCounter] == '{')
					{
						iSegNameCounter++;
						break;
					}
				}
				if (iSegNameCounter < 0)
				{
					// If went beyond the start, correct
					iSegNameCounter = 0;
				}
				// Segment name is now from the following character until before the colon
				iPairNameCounter = 0;
				for (int x = iSegNameCounter; x < iCounter; x++)
				{
					pairNameRaw[iPairNameCounter] = mqttIncomingPayload[x];
					iPairNameCounter++;
				}
				pairNameRaw[iPairNameCounter] = '\0';


				// Everything to right is value until reached the end, a comma or a right brace.
				for (iSegValueCounter = iCounter + 1; iSegValueCounter < length; iSegValueCounter++)
				{
					if (mqttIncomingPayload[iSegValueCounter] == ',' || mqttIncomingPayload[iSegValueCounter] == '}')
					{
						iSegValueCounter--;
						break;
					}
				}
				// Correct if went beyond the end
				if (iSegValueCounter >= length)
				{
					// If went beyond end, correct
					iSegValueCounter = length - 1;
				}
				// Segment value is now from the after the colon until the found character
				iPairValueCounter = 0;
				for (int x = iCounter + 1; x <= iSegValueCounter; x++)
				{
					pairValueRaw[iPairValueCounter] = mqttIncomingPayload[x];
					iPairValueCounter++;
				}
				pairValueRaw[iPairValueCounter] = '\0';


				// Transfer to a cleansed copy, without unwanted chars

				iPairNameCounter = 0;
				iCleanCounter = 0;
				while (pairNameRaw[iCleanCounter] != 0)
				{
					// Allow alpha numeric, upper case and lower case
					if ((pairNameRaw[iCleanCounter] >= 'a' && pairNameRaw[iCleanCounter] <= 'z') || (pairNameRaw[iCleanCounter] >= 'A' && pairNameRaw[iCleanCounter] <= 'Z') || (pairNameRaw[iCleanCounter] >= '0' && pairNameRaw[iCleanCounter] <= '9'))
					{
						// Transfer Over
						pairNameClean[iPairNameCounter] = pairNameRaw[iCleanCounter];
						iPairNameCounter++;
					}
					iCleanCounter++;
				}
				pairNameClean[iPairNameCounter] = '\0';



				iPairValueCounter = 0;
				iCleanCounter = 0;
				while (pairValueRaw[iCleanCounter] != 0)
				{
					// Allow a minus, x (for hex), and 0-9, and a-f A-F for hex
					if ((pairValueRaw[iCleanCounter] == '-' || pairValueRaw[iCleanCounter] == 'x' || (pairValueRaw[iCleanCounter] >= '0' && pairValueRaw[iCleanCounter] <= '9') || (pairValueRaw[iCleanCounter] >= 'A' && pairValueRaw[iCleanCounter] <= 'F')) || (pairValueRaw[iCleanCounter] >= 'a' && pairValueRaw[iCleanCounter] <= 'f'))
					{
						// Transfer Over
						pairValueClean[iPairValueCounter] = pairValueRaw[iCleanCounter];
						iPairValueCounter++;
					}
					iCleanCounter++;
				}
				pairValueClean[iPairValueCounter] = '\0';


#ifdef DEBUG
				sprintf(_debugOutput, "Got a cleaned JSON parameter of '%s', value '%s'", pairNameClean, pairValueClean);
				Serial.println(_debugOutput);
#endif

				if (strcmp(pairNameClean, "registerAddress") == 0)
				{
#ifdef DEBUG
					sprintf(_debugOutput, "This was handled as registerAddress");
					Serial.println(_debugOutput);
#endif
					strcpy(registerAddress, pairValueClean);
				}
				else if (strcmp(pairNameClean, "dataBytes") == 0)
				{
#ifdef DEBUG
					sprintf(_debugOutput, "This was handled as dataBytes");
					Serial.println(_debugOutput);
#endif
					strcpy(dataBytes, pairValueClean);
				}
				else if (strcmp(pairNameClean, "value") == 0)
				{
#ifdef DEBUG
					sprintf(_debugOutput, "This was handled as value");
					Serial.println(_debugOutput);
#endif
					strcpy(value, pairValueClean);
				}
				else if (strcmp(pairNameClean, "watts") == 0)
				{
#ifdef DEBUG
					sprintf(_debugOutput, "This was handled as watts");
					Serial.println(_debugOutput);
#endif
					strcpy(watts, pairValueClean);
				}
				else if (strcmp(pairNameClean, "duration") == 0)
				{
#ifdef DEBUG
					sprintf(_debugOutput, "This was handled as duration");
					Serial.println(_debugOutput);
#endif
					strcpy(duration, pairValueClean);
				}
				else if (strcmp(pairNameClean, "socPercent") == 0)
				{
#ifdef DEBUG
					sprintf(_debugOutput, "This was handled as socPercent");
					Serial.println(_debugOutput);
#endif
					strcpy(socPercent, pairValueClean);
				}
				else if (strcmp(pairNameClean, "start") == 0)
				{
#ifdef DEBUG
					sprintf(_debugOutput, "This was handled as start");
					Serial.println(_debugOutput);
#endif
					strcpy(startPos, pairValueClean);
				}
				else if (strcmp(pairNameClean, "end") == 0)
				{
#ifdef DEBUG
					sprintf(_debugOutput, "This was handled as end");
					Serial.println(_debugOutput);
#endif
					strcpy(endPos, pairValueClean);
				}
			}
		}
	}


	// Carry on?
	if (result == modbusRequestAndResponseStatusValues::preProcessing)
	{
		if (subScription == mqttSubscriptions::readHandledRegister)
		{
			// Check if registerAddress found
			if (!*registerAddress)
			{
#ifdef DEBUG
				sprintf(_debugOutput, "Trying to readHandledRegister without a registerAddress!");
				Serial.println(_debugOutput);
#endif
				result = modbusRequestAndResponseStatusValues::invalidMQTTPayload;
				strcpy(response.statusMqttMessage, MODBUS_REQUEST_AND_RESPONSE_INVALID_MQTT_PAYLOAD_MQTT_DESC);
			}
			else
			{
				// Convert string to a number using base 16.
				registerAddressConverted = strtoul(registerAddress, NULL, 16);

				result = _registerHandler->readHandledRegister(registerAddressConverted, &response);
				if (result == modbusRequestAndResponseStatusValues::readDataRegisterSuccess || result == modbusRequestAndResponseStatusValues::slaveError)
				{
					for (int i = 0; i < response.dataSize; i++)
					{
						sprintf(rawByteForPayload, "%u,", response.data[i]);
						strcat(rawDataForPayload, rawByteForPayload);
					}
					rawDataForPayload[strlen(rawDataForPayload) - 1] = '\0';
				}
			}
		}
		else if (subScription == mqttSubscriptions::readRawRegister)
		{
			if (!*registerAddress || !*dataBytes)
			{
#ifdef DEBUG
				sprintf(_debugOutput, "Trying to readRawRegister without a registerAddress or dataBytes!");
				Serial.println(_debugOutput);
#endif
				result = modbusRequestAndResponseStatusValues::invalidMQTTPayload;
				strcpy(response.statusMqttMessage, MODBUS_REQUEST_AND_RESPONSE_INVALID_MQTT_PAYLOAD_MQTT_DESC);
			}
			else
			{
				registerAddressConverted = strtoul(registerAddress, NULL, 16);

				// Convert string to number using base ten
				registerCount = strtoul(dataBytes, NULL, 10);
				registerCount = registerCount / 2;
				response.registerCount = registerCount;

				result = _registerHandler->readRawRegister(registerAddressConverted, &response);
			}

			if (result == modbusRequestAndResponseStatusValues::readDataRegisterSuccess || result == modbusRequestAndResponseStatusValues::slaveError)
			{
				for (int i = 0; i < response.dataSize; i++)
				{
					sprintf(rawByteForPayload, "%u,", response.data[i]);
					strcat(rawDataForPayload, rawByteForPayload);
				}
				rawDataForPayload[strlen(rawDataForPayload) - 1] = '\0';
			}
		}
		else if (subScription == mqttSubscriptions::writeRawSingleRegister)
		{
			if (!*registerAddress || !*value)
			{
#ifdef DEBUG
				sprintf(_debugOutput, "Trying to writeRawSingleRegister without a registerAddress or value!");
				Serial.println(_debugOutput);
#endif
				result = modbusRequestAndResponseStatusValues::invalidMQTTPayload;
				strcpy(response.statusMqttMessage, MODBUS_REQUEST_AND_RESPONSE_INVALID_MQTT_PAYLOAD_MQTT_DESC);
			}
			else
			{
				registerAddressConverted = strtoul(registerAddress, NULL, 16);
				singleRegisterValueConverted = strtoul(value, NULL, 10);

				result = _registerHandler->writeRawSingleRegister(registerAddressConverted, singleRegisterValueConverted, &response);
			}

			if (result == modbusRequestAndResponseStatusValues::writeSingleRegisterSuccess || result == modbusRequestAndResponseStatusValues::slaveError)
			{
				for (int i = 0; i < response.dataSize; i++)
				{
					sprintf(rawByteForPayload, "%u,", response.data[i]);
					strcat(rawDataForPayload, rawByteForPayload);
				}
				rawDataForPayload[strlen(rawDataForPayload) - 1] = '\0';
			}
		}
		else if (subScription == mqttSubscriptions::writeRawDataRegister)
		{
			if (!*registerAddress || !*dataBytes || !*value)
			{
#ifdef DEBUG
				sprintf(_debugOutput, "Trying to writeRawDataRegister without a registerAddress, dataBytes or value!");
				Serial.println(_debugOutput);
#endif
				result = modbusRequestAndResponseStatusValues::invalidMQTTPayload;
				strcpy(response.statusMqttMessage, MODBUS_REQUEST_AND_RESPONSE_INVALID_MQTT_PAYLOAD_MQTT_DESC);
			}
			else
			{
				registerAddressConverted = strtoul(registerAddress, NULL, 16);
				dataRegisterValueConverted = strtoul(value, NULL, 10);
				registerCount = strtoul(dataBytes, NULL, 10);
				registerCount = registerCount / 2;
				response.registerCount = registerCount;

				result = _registerHandler->writeRawDataRegister(registerAddressConverted, dataRegisterValueConverted, &response);
			}
			if (result == modbusRequestAndResponseStatusValues::writeDataRegisterSuccess || result == modbusRequestAndResponseStatusValues::slaveError)
			{
				for (int i = 0; i < response.dataSize; i++)
				{
					sprintf(rawByteForPayload, "%u,", response.data[i]);
					strcat(rawDataForPayload, rawByteForPayload);
				}
				rawDataForPayload[strlen(rawDataForPayload) - 1] = '\0';
			}
		}
		else if (subScription == mqttSubscriptions::readHandledRegisterAll)
		{
			if(!*startPos || !*endPos)
			{
#ifdef DEBUG
				sprintf(_debugOutput, "Trying to readHandledRegisterAll without a start or end!");
				Serial.println(_debugOutput);
#endif
				result = modbusRequestAndResponseStatusValues::invalidMQTTPayload;
				strcpy(response.statusMqttMessage, MODBUS_REQUEST_AND_RESPONSE_INVALID_MQTT_PAYLOAD_MQTT_DESC);
			}
			else
			{
				startPosConverted = strtoul(startPos, NULL, 10);
				endPosConverted = strtoul(endPos, NULL, 10);

				// Despite a start and end provided, ensure not below zero and not above the array size
				int numberOfRegisters = sizeof(_mqttAllHandledRegisters) / sizeof(struct mqttState);
				uint16_t minPosition = startPosConverted < 0 ? 0 : startPosConverted;
				uint16_t maxPosition = endPosConverted > numberOfRegisters - 1 ? numberOfRegisters - 1 : endPosConverted;

				resultAddToPayload = addToPayload("{\r\n");
				if (resultAddToPayload == modbusRequestAndResponseStatusValues::addedToPayload)
				{
					for (int l = minPosition; l <= maxPosition; l++)
					{
						memcpy_P(&singleRegister.registerAddress, &_mqttAllHandledRegisters[l].registerAddress, 2);
						strcpy_P(singleRegister.mqttName, _mqttAllHandledRegisters[l].mqttName);

						result = addStateInfo(singleRegister.registerAddress, singleRegister.mqttName, l < maxPosition, resultAddToPayload);
						if (resultAddToPayload == modbusRequestAndResponseStatusValues::payloadExceededCapacity)
						{
							// If the response to addStateInfo is payload exceeded get out, We will permit failing registers to carry on and try the next one.
							break;
						}
					}
					if (resultAddToPayload != modbusRequestAndResponseStatusValues::payloadExceededCapacity)
					{
						// Footer, if we haven't already bombed out with payload issues
						resultAddToPayload = addToPayload("}");
					}
				}
			}
		}
		else if ((subScription == mqttSubscriptions::setCharge) || (subScription == mqttSubscriptions::setDischarge))
		{
			// Handles dispatch mode for charge and discharge
			// Code is fundamentally the same, just a different charge power.
			int multiplier = 1;

			//if (strcmp(topic, DEVICE_NAME MQTT_SUB_REQUEST_SET_CHARGE) == 0)
			if (subScription == mqttSubscriptions::setCharge)
			{
#ifdef DEBUG
				sprintf(_debugOutput, "Multiplier for setCharge will be -1");
				Serial.println(_debugOutput);
#endif
				multiplier = -1;
			}
			else
			{
#ifdef DEBUG
				sprintf(_debugOutput, "Multiplier for setDischarge will be 1");
				Serial.println(_debugOutput);
#endif
				multiplier = 1;
			}

			if (!*watts || !*socPercent || !*duration)
			{
#ifdef DEBUG
				sprintf(_debugOutput, "Trying to setCharge or setDischarge without a watts, socPercent or duration!");
				Serial.println(_debugOutput);
#endif
				result = modbusRequestAndResponseStatusValues::invalidMQTTPayload;
				strcpy(response.statusMqttMessage, MODBUS_REQUEST_AND_RESPONSE_INVALID_MQTT_PAYLOAD_MQTT_DESC);
			}
			else
			{
				chargeDischargeWattsConverted = strtoul(watts, NULL, 10);
				batterySocPercentConverted = strtoul(socPercent, NULL, 10);
				durationSecondsConverted = strtoul(duration, NULL, 10);

#ifdef DEBUG
				sprintf(_debugOutput, "Base 10 type-cast values for watts, socPercent and duration are %d, %d and %d respectively", chargeDischargeWattsConverted, batterySocPercentConverted, durationSecondsConverted);
				Serial.println(_debugOutput);
#endif
				// Adjust
				// Charge < 32000, discharge > 32000
				chargeDischargeWattsConverted = 32000 + (chargeDischargeWattsConverted * multiplier);
				batterySocPercentConverted = batterySocPercentConverted / DISPATCH_SOC_MULTIPLIER;

#ifdef DEBUG
				sprintf(_debugOutput, "And adjusted values for watts and socPercent are %d and %d respectively", chargeDischargeWattsConverted, batterySocPercentConverted);
				Serial.println(_debugOutput);
#endif

				// First enable dispatch
				responseDispatch.registerCount = 1;
				resultDispatch = _registerHandler->writeRawDataRegister(REG_DISPATCH_RW_DISPATCH_START, DISPATCH_START_START, &responseDispatch);
				if (resultDispatch != modbusRequestAndResponseStatusValues::writeDataRegisterSuccess)
				{
					sprintf(stateAddition, "{\r\n    \"responseStatus\": \"%s\",\r\n    \"failureDetail\": \"REG_DISPATCH_RW_DISPATCH_START\"\r\n}", responseDispatch.statusMqttMessage);
				}
				else
				{
					delay(RS485_TRIES * 50);
					// Now set power
					responseDispatch.registerCount = 2;
					resultDispatch = _registerHandler->writeRawDataRegister(REG_DISPATCH_RW_ACTIVE_POWER_1, chargeDischargeWattsConverted, &responseDispatch);
					if (resultDispatch != modbusRequestAndResponseStatusValues::writeDataRegisterSuccess)
					{
						sprintf(stateAddition, "{\r\n    \"responseStatus\": \"%s\",\r\n    \"failureDetail\": \"REG_DISPATCH_RW_ACTIVE_POWER_1\"\r\n}", responseDispatch.statusMqttMessage);
					}
					else
					{
						delay(RS485_TRIES * 50);
						// Now set duration
						responseDispatch.registerCount = 2;
						resultDispatch = _registerHandler->writeRawDataRegister(REG_DISPATCH_RW_DISPATCH_TIME_1, durationSecondsConverted, &responseDispatch);
						if (resultDispatch != modbusRequestAndResponseStatusValues::writeDataRegisterSuccess)
						{
							sprintf(stateAddition, "{\r\n    \"responseStatus\": \"%s\",\r\n    \"failureDetail\": \"REG_DISPATCH_RW_DISPATCH_TIME_1\"\r\n}", responseDispatch.statusMqttMessage);
						}
						else
						{
							delay(RS485_TRIES * 50);
							// Now set battery
							responseDispatch.registerCount = 1;
							resultDispatch = _registerHandler->writeRawDataRegister(REG_DISPATCH_RW_DISPATCH_SOC, batterySocPercentConverted, &responseDispatch);
							if (resultDispatch != modbusRequestAndResponseStatusValues::writeDataRegisterSuccess)
							{
								sprintf(stateAddition, "{\r\n    \"responseStatus\": \"%s\",\r\n    \"failureDetail\": \"REG_DISPATCH_RW_DISPATCH_SOC\"\r\n}", responseDispatch.statusMqttMessage);
							}
							else
							{
								delay(RS485_TRIES * 50);
								// Finally set the mode to start
								responseDispatch.registerCount = 1;
								resultDispatch = _registerHandler->writeRawDataRegister(REG_DISPATCH_RW_DISPATCH_MODE, DISPATCH_MODE_STATE_OF_CHARGE_CONTROL, &responseDispatch);
								if (resultDispatch != modbusRequestAndResponseStatusValues::writeDataRegisterSuccess)
								{
									sprintf(stateAddition, "{\r\n    \"responseStatus\": \"%s\",\r\n    \"failureDetail\": \"REG_DISPATCH_RW_DISPATCH_MODE\"\r\n}", responseDispatch.statusMqttMessage);
								}
								else
								{
									if (subScription == mqttSubscriptions::setCharge)
									{
										sprintf(stateAddition, "{\r\n    \"responseStatus\": \"%s\",\r\n    \"failureDetail\": \"\"\r\n}", MODBUS_REQUEST_AND_RESPONSE_SET_CHARGE_SUCCESS_MQTT_DESC);
										result = modbusRequestAndResponseStatusValues::setChargeSuccess;
									}
									else
									{
										sprintf(stateAddition, "{\r\n    \"responseStatus\": \"%s\",\r\n    \"failureDetail\": \"\"\r\n}", MODBUS_REQUEST_AND_RESPONSE_SET_DISCHARGE_SUCCESS_MQTT_DESC);
										result = modbusRequestAndResponseStatusValues::setDischargeSuccess;
									}
									
								}
							}
						}
					}
				}
			}
			resultAddToPayload = addToPayload(stateAddition);
		}

		else if (subScription == mqttSubscriptions::setNormal)
		{
			// Turns off dispatch mode for normal

			responseDispatch.registerCount = 1;
			resultDispatch = _registerHandler->writeRawDataRegister(REG_DISPATCH_RW_DISPATCH_START, DISPATCH_START_STOP, &responseDispatch);
			if (resultDispatch != modbusRequestAndResponseStatusValues::writeDataRegisterSuccess)
			{
				sprintf(stateAddition, "{\r\n    \"responseStatus\": \"%s\",\r\n    \"failureDetail\": \"REG_DISPATCH_RW_DISPATCH_START\"\r\n}", responseDispatch.statusMqttMessage);
			}
			else
			{
				sprintf(stateAddition, "{\r\n    \"responseStatus\": \"%s\",\r\n    \"failureDetail\": \"\"\r\n}", MODBUS_REQUEST_AND_RESPONSE_SET_NORMAL_SUCCESS_MQTT_DESC);
			}
			result = modbusRequestAndResponseStatusValues::setNormalSuccess;
			resultAddToPayload = addToPayload(stateAddition);
		}

	}



	// Set Charge/Discharge/Normal/ReadAll do their own payload constructions.
	// For anything else, construct a standard response based on what we know from the request and the result.
	if (result == modbusRequestAndResponseStatusValues::invalidMQTTPayload || result == modbusRequestAndResponseStatusValues::noMQTTPayload || subScription == mqttSubscriptions::readHandledRegister || subScription == mqttSubscriptions::readRawRegister || subScription == mqttSubscriptions::writeRawDataRegister || subScription == mqttSubscriptions::writeRawSingleRegister)
	{
		// Construct a response based on what we know from above
		resultAddToPayload = addToPayload("{\r\n");
		if (resultAddToPayload == modbusRequestAndResponseStatusValues::addedToPayload)
		{
			sprintf(stateAddition, "    \"responseStatus\": \"%s\",\r\n", response.statusMqttMessage);
			resultAddToPayload = addToPayload(stateAddition);
		}

		// Providing we had a payload and it was valid, we can at least send the registerAdress back to give the user some context
		if (resultAddToPayload == modbusRequestAndResponseStatusValues::addedToPayload && (result != modbusRequestAndResponseStatusValues::noMQTTPayload && result != modbusRequestAndResponseStatusValues::invalidMQTTPayload))
		{
			sprintf(stateAddition, "    \"registerAddress\": \"%s\",\r\n", registerAddress);
			resultAddToPayload = addToPayload(stateAddition);
		}

		// If some kind of result came back from the Alpha (even a slave error) we can give the function code
		if (resultAddToPayload == modbusRequestAndResponseStatusValues::addedToPayload && (result == modbusRequestAndResponseStatusValues::writeDataRegisterSuccess || result == modbusRequestAndResponseStatusValues::slaveError || result == modbusRequestAndResponseStatusValues::writeSingleRegisterSuccess || result == modbusRequestAndResponseStatusValues::readDataRegisterSuccess))
		{
			sprintf(stateAddition, "    \"functionCode\": %d,\r\n", response.functionCode);
			resultAddToPayload = addToPayload(stateAddition);
		}

		// Content returned by a Read Handled request
		if (resultAddToPayload == modbusRequestAndResponseStatusValues::addedToPayload && (result == modbusRequestAndResponseStatusValues::readDataRegisterSuccess && subScription == mqttSubscriptions::readHandledRegister))
		{
			sprintf(stateAddition, "    \"registerName\": \"%s\",\r\n", response.mqttName);
			resultAddToPayload = addToPayload(stateAddition);
			if (resultAddToPayload == modbusRequestAndResponseStatusValues::addedToPayload)
			{
				sprintf(stateAddition, "    \"dataType\": \"%s\",\r\n", response.returnDataTypeDesc);
				resultAddToPayload = addToPayload(stateAddition);
			}
			if (resultAddToPayload == modbusRequestAndResponseStatusValues::addedToPayload)
			{
				switch (response.returnDataType)
				{
				case modbusReturnDataType::character:
				{
					sprintf(stateAddition, "    \"dataValue\": \"%s\",\r\n", response.characterValue);
					resultAddToPayload = addToPayload(stateAddition);
					break;
				}
				case modbusReturnDataType::signedInt:
				{
					sprintf(stateAddition, "    \"dataValue\": %d,\r\n", response.signedIntValue);
					resultAddToPayload = addToPayload(stateAddition);
					break;
				}
				case modbusReturnDataType::unsignedInt:
				{
					sprintf(stateAddition, "    \"dataValue\": %u,\r\n", response.unsignedIntValue);
					resultAddToPayload = addToPayload(stateAddition);
					break;
				}
				case modbusReturnDataType::signedShort:
				{
					sprintf(stateAddition, "    \"dataValue\": %d,\r\n", response.signedShortValue);
					resultAddToPayload = addToPayload(stateAddition);
					break;
				}
				case modbusReturnDataType::unsignedShort:
				{
					sprintf(stateAddition, "    \"dataValue\": %u,\r\n", response.unsignedShortValue);
					resultAddToPayload = addToPayload(stateAddition);
					break;
				}
				default:
				{
					sprintf(stateAddition, "    \"dataValue\": \"\",\r\n");
					resultAddToPayload = addToPayload(stateAddition);
					break;
				}
				}
			}
			if (resultAddToPayload == modbusRequestAndResponseStatusValues::addedToPayload)
			{
				bool addQuote = (response.returnDataType == modbusReturnDataType::character || response.hasLookup);
				sprintf(stateAddition, "    \"formattedDataValue\": %s%s%s,\r\n", addQuote ? "\"" : "", response.dataValueFormatted, addQuote ? "\"" : "");
				resultAddToPayload = addToPayload(stateAddition);
			}

		}


		// Slave error can provide the code back
		if (resultAddToPayload == modbusRequestAndResponseStatusValues::addedToPayload && (result == modbusRequestAndResponseStatusValues::slaveError))
		{
			sprintf(stateAddition, "    \"slaveErrorCode\": %d,\r\n", response.data[0]);
			resultAddToPayload = addToPayload(stateAddition);
		}

		// Again, if some kind of result came back we can expose the raw data bytes for the user to process if they want
		if (resultAddToPayload == modbusRequestAndResponseStatusValues::addedToPayload && (result == modbusRequestAndResponseStatusValues::writeDataRegisterSuccess || result == modbusRequestAndResponseStatusValues::slaveError || result == modbusRequestAndResponseStatusValues::writeSingleRegisterSuccess || result == modbusRequestAndResponseStatusValues::readDataRegisterSuccess))
		{
			sprintf(stateAddition, "    \"rawDataSize\": %u,\r\n", response.dataSize);
			resultAddToPayload = addToPayload(stateAddition);
		}
		if (resultAddToPayload == modbusRequestAndResponseStatusValues::addedToPayload && (result == modbusRequestAndResponseStatusValues::writeDataRegisterSuccess || result == modbusRequestAndResponseStatusValues::slaveError || result == modbusRequestAndResponseStatusValues::writeSingleRegisterSuccess || result == modbusRequestAndResponseStatusValues::readDataRegisterSuccess))
		{
			sprintf(stateAddition, "    \"rawData\": [%s],\r\n", rawDataForPayload);
			resultAddToPayload = addToPayload(stateAddition);
		}
		// Horrible, however it sorts out needing to worry about commas from any of the above statements and is little overhead.
		if (resultAddToPayload)
		{
			sprintf(stateAddition, "    \"end\": \"true\"\r\n}");
			resultAddToPayload = addToPayload(stateAddition);
		}

	}

	if (subScription != mqttSubscriptions::unknown && result != modbusRequestAndResponseStatusValues::notValidIncomingTopic)
	{
		sendMqtt(topicResponse);
	}

	emptyPayload();
	return;
}

// mqttReconnect
/**
	This function reconnects the ESP8266 to the MQTT broker
**/
void Alpha::mqttReconnect()
{
	bool subscribed = false;
	char subscriptionDef[100];

	_mqtt->setServer(MQTT_SERVER, MQTT_PORT);

	// Loop until we're reconnected
	while (true)
	{

		_mqtt->disconnect();		// Just in case.
		delay(200);

#ifdef DEBUG
		Serial.print("Attempting MQTT connection...");
#endif

		updateOLED(false, "Connecting", "MQTT...", _version);
		delay(100);

		// Attempt to connect
		if (_mqtt->connect(DEVICE_NAME, MQTT_USERNAME, MQTT_PASSWORD))
		{
			Serial.println("Connected MQTT, trying to subscribe topics");

			sprintf(subscriptionDef, "%s", DEVICE_NAME MQTT_SUB_REQUEST_READ_HANDLED_REGISTER);
			subscribed = _mqtt->subscribe(subscriptionDef);
			sprintf(subscriptionDef, "%s", DEVICE_NAME MQTT_SUB_REQUEST_READ_RAW_REGISTER);
			subscribed = subscribed && _mqtt->subscribe(subscriptionDef);
			sprintf(subscriptionDef, "%s", DEVICE_NAME MQTT_SUB_REQUEST_SET_CHARGE);
			subscribed = subscribed && _mqtt->subscribe(subscriptionDef);
			sprintf(subscriptionDef, "%s", DEVICE_NAME MQTT_SUB_REQUEST_SET_DISCHARGE);
			subscribed = subscribed && _mqtt->subscribe(subscriptionDef);
			sprintf(subscriptionDef, "%s", DEVICE_NAME MQTT_SUB_REQUEST_SET_NORMAL);
			subscribed = subscribed && _mqtt->subscribe(subscriptionDef);
			sprintf(subscriptionDef, "%s", DEVICE_NAME MQTT_SUB_REQUEST_WRITE_RAW_SINGLE_REGISTER);
			subscribed = subscribed && _mqtt->subscribe(subscriptionDef);
			sprintf(subscriptionDef, "%s", DEVICE_NAME MQTT_SUB_REQUEST_WRITE_RAW_DATA_REGISTER);
			subscribed = subscribed && _mqtt->subscribe(subscriptionDef);
			sprintf(subscriptionDef, "%s", DEVICE_NAME MQTT_SUB_REQUEST_READ_HANDLED_REGISTER_ALL);
			subscribed = subscribed && _mqtt->subscribe(subscriptionDef);

			// Subscribe or resubscribe to topics.
			if (subscribed)
			{
				Serial.println("Subscribed, update runstate");

				// Connected, so ditch out with runstate on the screen
				updateRunstate();
				break;
			}
		
		}

		if (!subscribed)
#ifdef DEBUG
		sprintf(_debugOutput, "MQTT Failed: RC is %d\r\nTrying again in five seconds...", _mqtt->state());
		Serial.println(_debugOutput);
#endif

		// Wait 5 seconds before retrying
		delay(5000);
	}
}

void Alpha::sendMqtt(char *topic)
{
	// Attempt a send
	if (!_mqtt->publish(topic, _mqttPayload))
	{
#ifdef DEBUG
		//sprintf(_debugOutput, "MQTT publish failed to %s", topic);
		Serial.println(_debugOutput);
		Serial.println(_mqttPayload);
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
	emptyPayload();
	return;
}
