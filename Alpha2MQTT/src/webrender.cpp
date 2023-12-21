#include "webrender.h"
#include <map>
#include <string>     // std::string, std::stol
//#include <ArduinoJson.h>
#include <ESP_EEPROM.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <AsyncWebConfig.h>

AsyncWebServer server(80);
AsyncWebConfig webConf;

static Trigger* trigger;

#define ENABLED "enabled"
#define URL "url"
#define KEY "key"
#define PRICE "price"
#define SOCTHRESHHOLD "socthreshold"
#define POWERTHRESHHOLD "powerthreshold"
#define MQTTPAYLOAD "mqttpayload"
#define MQTTPAYLOAD_DISABLE "mqttpayloaddisable"
#define MQTTURL "mqtturl"

const char* PROGMEM params = "["
"{"
"'name':'ssid',"
"'label':'WLAN name',"
"'type':'0',"
"'default':''"
"}"
"]";

// Config Settings 
/**
 String paramsOLD = "[ " 
  "{"
  "'name':'url',"
  "'label':'Tibber Url',"
  "'type':'0',"
  "'default':'https://'"
  "},"
  "{"
  "'name':'enabled',"
  "'label':'Tibber Key',"
  "'type':'0',"
  "'default':'abc'"
  "},"
  "{"
  "'name':'price',"
  "'label':'Tibber Price Threshold',"
  "'type':'2',"
  "'default':'20'"
  "},"
  "{"
  "'name':'socthreshold',"
  "'label':'SOC trigger threshold',"
  "'type':'2',"
  "'default':'70'"
  "},"
  "{"
  "'name':'powerthreshold',"
  "'label':'Power trigger (watts)',"
  "'type':'2',"
  "'default':'2000'"
  "}"
  "]";
**/
const char* PROGMEM index_html = " \
  <html> \
  <style type=\"text/css\"> \
 .bar {  \
  fill: orange; \
  height: 21px; \
  transition: fill .3s ease; \
  cursor: pointer; \
  font-family: Helvetica, sans-serif; \
 } \
</style> \
  <body> \
  <svg class=\"chart\" width=\"800\" height=\"800\" role=\"img\" > \
  <title id=\"title\">A bar chart showing information</title> \
    %table_tr_1% \
    %table_tr_2% \
    %table_tr_3% \
    %table_tr_4% \
    %table_tr_5% \
    %table_tr_6% \
    %table_tr_7% \
    %table_tr_8% \
    %table_tr_9% \
    %table_tr_10% \
    %table_tr_11% \
    %table_tr_12% \
    %table_tr_13% \
    %table_tr_14% \
    %table_tr_15% \
    %table_tr_16% \
    %table_tr_17% \
    %table_tr_18% \
    %table_tr_19% \
    %table_tr_20% \
    %table_tr_21% \
    %table_tr_22% \
    %table_tr_23% \
    %table_tr_24% \
    </svg> </body> </html> \
" ;

String processor(const String& var)
{
  Serial.println(var);
  int pos = var.indexOf("_tr_");
  Serial.println(pos);

  String subString = var.substring(pos+4);
  Serial.println(subString);
  int cnt = subString.toInt();
  Serial.println(cnt);
  int width = cnt*5;
  int ypos= cnt*20;

  return String("<g class=\"bar\"> <rect width=\"") +  String(width) + String("\" height=\"19\" y=\"") + String(ypos) + String("\" ></rect></g>");
 }

void WebRender::handleRoot(AsyncWebServerRequest *request){
  Serial.printf("handleRoot TMP : Start -> Free Heap : %i\n", ESP.getFreeHeap());          
  webConf.handleFormRequest(request);
   if (request->hasParam("SAVE")) {
    uint8_t cnt = webConf.getCount();
    Serial.println("*********** Config SAVE ************");
    for (uint8_t i = 0; i<cnt; i++) {
      Serial.print(webConf.getName(i));
      Serial.print(" = ");
      Serial.println(webConf.values[i]);
    }
    request->redirect("/"); 
  }
 Serial.printf("handleRoot TMP : Start -> End Heap : %i\n", ESP.getFreeHeap());

}

void WebRender::convertConfig(){
  int socThreshold = webConf.getInt(String(SOCTHRESHHOLD).c_str());
  int powerThreshold = webConf.getInt(String(POWERTHRESHHOLD).c_str());
  bool tibberEnabled = webConf.getBool(String(ENABLED).c_str());
  bool tibberPriceThreshold = webConf.getInt(String(PRICE).c_str());
  String mqttUrl = webConf.getString(String(MQTTURL).c_str());
  String mqttEnable = webConf.getString(String(MQTTPAYLOAD).c_str());
  String mqttDisable = webConf.getString(String(MQTTPAYLOAD_DISABLE).c_str());

  Serial.println("*********** Config convert ************");
  Serial.print("tibber Enabled");
  Serial.print(" = ");
  Serial.print(tibberEnabled);
  Serial.print("socThreshold");
  Serial.print(" = ");
  Serial.println(socThreshold);
  Serial.print("powerThreshold");
  Serial.print(" = ");
  Serial.println(powerThreshold);
  if (tibberEnabled) {
      trigger->init(socThreshold, powerThreshold, tibberPriceThreshold, mqttUrl, mqttEnable, mqttDisable);
  }
  else {
    trigger->init(socThreshold, powerThreshold, -1, mqttUrl, mqttEnable, mqttDisable);
  }
}

void WebRender::setTrigger(Trigger* triggerReference){
  trigger = triggerReference;
}

void WebRender::setupWebInterface(){ 
  Serial.println("Start Setup Webinterface  ");

//  webConf.setDescription(params);
  Serial.println("Set Description Done ");

 // webConf.readConfig();
  server.on("/config",handleRoot);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
     Serial.printf("TMP : Start -> Free Heap : %i\n", ESP.getFreeHeap());        
    request->send_P(200, "text/html", index_html, processor);
     Serial.printf("TMP : Start -> End Heap : %i\n", ESP.getFreeHeap());        
  });

  server.begin();  
  Serial.println("Setup Webinterface Done  ");
  //convertConfig();
}



/**
WebRender::Config getConfigData()
{
    WebRender::Config customData;
    //EEPROM.get(eepromstart, customData);
    return customData;
}

void saveconfigtoEE(WebRender::Config MyconfigData)
{
    EEPROM.put(eepromstart, MyconfigData);
    //JumpStart();
    boolean ok2 = EEPROM.commit();
    Serial.println(ok2);
}
**/
