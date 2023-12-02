#include "Definitions.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <PubSubClient.h>
#if defined MP_ESP8266
#include <ESP8266WiFi.h>
#elif defined MP_ESP32
#include <WiFi.h>
#endif
#include "RegisterHandler.h"

class Alpha 
{
    public:

    Alpha(Adafruit_SSD1306 * display, RegisterHandler* registerHandler,PubSubClient* _mqtt);
    
    static void emptyPayload();
    static bool checkTimer(unsigned long *lastRun, unsigned long interval);
    static void updateOLED(bool justStatus, const char* line2, const char* line3, const char* line4);

    static void updateRunstate();
    static int getMaxPayloadSize();
    void setMaxPayloadSize(int maxPayloadSize);

    static modbusRequestAndResponseStatusValues getSerialNumber();
    static modbusRequestAndResponseStatusValues addToPayload(char* addition);
    static  modbusRequestAndResponseStatusValues addStateInfo(uint16_t registerAddress, char* registerName, bool addComma, modbusRequestAndResponseStatusValues& resultAddedToPayload);

    static char* getMqttPayload();
    static void setMqttPayload(char* payload);
    static void sendData();
    static void sendMqtt(char *topic);
    static void sendDataFromAppropriateArray(mqttState* registerArray, int numberOfRegisters, char* topic);
    static void mqttCallback(char* topic, byte* message, unsigned int length);
    static void mqttReconnect();


};