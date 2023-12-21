#include "trigger.h"
#include <Arduino.h> 

using namespace std;

    Trigger::Trigger()
    {

    }
    
    void Trigger::init(int socThreshold, int powerThreshold, int tibberPrice, String mqttUrl,
                        String mqttEnable, String mqttDisable)
    {
        this->socThreshold = socThreshold;
        this->powerThreshold = powerThreshold;
        this->tibberPriceThreshold = tibberPrice;
        this->lastCheckhour = 0 ;
    }

    void Trigger::recordData(int soc, int power, double tibberPrice)
    {
       time_t now = time(0);
       tm *local_time = localtime(&now);
       int hour = local_time->tm_hour;

       if (hour < lastCheckhour){
        // dayswitch
        clearData();    
       }

        this->histElements[hour].isTriggeredSun = this->isTriggeredSun(soc,power);
        if (this->tibberPriceThreshold != -1 ){
            this->histElements[hour].isTriggeredTibber = this->isTriggeredTibber(tibberPrice);
        }
        this->histElements[hour].power = power;
        this->histElements[hour].soc= soc;
        this->histElements[hour].price= tibberPrice;
        
        lastCheckhour = hour;
    }


    void Trigger::clearData(){
        for (int y=0; y< 24 ; y++){
            this->histElements[y].isTriggeredSun=false;
            this->histElements[y].isTriggeredTibber=false;
            this->histElements[y].power=0;
            this->histElements[y].price=0;   
            this->histElements[y].soc=0;   
        }
    }

    bool Trigger::isTriggeredSun(int soc, int power){
        return soc>this->socThreshold && power > powerThreshold ;
    }

    bool Trigger::isTriggeredTibber(int currentPrice){
    return currentPrice < tibberPriceThreshold;
    }

    Trigger::history Trigger::getHistory(int hour){
        return this->histElements[hour];
    }
