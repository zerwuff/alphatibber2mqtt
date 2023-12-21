
#ifndef TRIGGER_H
#define TRIGGER_H
#include <Arduino.h> 
#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>     // std::string, std::stol

class Trigger {

    int socThreshold;
    int powerThreshold;
    int tibberPriceThreshold;

    int lastCheckhour;


public:

    struct history {
        bool isTriggeredSun;
        bool isTriggeredTibber;
        int soc;
        int power;
        double price;
    } hist ;

    history histElements[24];

    Trigger();
    ~Trigger();

    void init(int socThreshold, int powerThreshold, int tibberPrice, String mqttUrl,
                        String mqttEnable, String mqttDisable);
 
    history getHistory(int hour);
    void recordData(int soc, int power, double tibberPrice);
    bool isTriggeredSun(int soc, int power);
    bool isTriggeredTibber(int curentPrice);
    void clearData();

};

#endif
