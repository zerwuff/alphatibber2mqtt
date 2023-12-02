#include "webrender.h"
#include <map>
#include <string>     // std::string, std::stol
//#include <ArduinoJson.h>
#include <ESP_EEPROM.h>
#include <ESPAsyncWebServer.h>
#include <dynaHTML.h>
#include <ESPAsyncTCP.h>
#include <AsyncWebConfig.h>

AsyncWebServer server(80);
AsyncWebConfig webConf;

std::map<int,String> mapData;

// Config Settings 
String params = "["
  "{"
  "'name':'enabled',"
  "'label':'Tibber Enabled',"
  "'type':"+String(INPUTCHECKBOX)+","
  "'default':'1'"
  "},"   
  "{"
  "'name':'url',"
  "'label':'Tibber Url',"
  "'type':"+String(INPUTTEXT)+","
  "'default':'https://'"
  "},"
  "{"
  "'name':'key',"
  "'label':'Tibber Key',"
  "'type':"+String(INPUTTEXT)+","
  "'default':'abc'"
  "},"
  "{"
  "'name':'price',"
  "'label':'Tibber Price Threshold (Cnt)',"
  "'type':"+String(INPUTNUMBER)+","
  "'default':'20'"
  "},"
   "{'name':'mqtturl',"
  "'label':'MQTT URL',"
  "'type':"+String(INPUTTEXT)+","
  "'default':'mqtt://user@pass:'"
  "},"      
   "{'name':'mqttpayload',"
  "'label':'MQTT PAYLOAD ENABLE',"
  "'type':"+String(INPUTTEXTAREA)+","
  "'default':'{ Json Message}'"
  "}"      
  "]";


const char* index_html = " \
  <html><body> \
  <table> \
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
    </table> \
  </body> </html> \
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
  
  return String("<tr> index position: " + String(cnt) + "</tr>");
 }

void WebRender::handleRoot(AsyncWebServerRequest *request){
  webConf.handleFormRequest(request);
   if (request->hasParam("SAVE")) {
    uint8_t cnt = webConf.getCount();
    Serial.println("*********** Config ************");
    for (uint8_t i = 0; i<cnt; i++) {
      Serial.print(webConf.getName(i));
      Serial.print(" = ");
      Serial.println(webConf.values[i]);
    }
  }
}

void WebRender::setupWebInterface(){ 
  webConf.setDescription(params);
  webConf.readConfig();
  Serial.println("Setup Webinterface  ");
  server.on("/config",handleRoot);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  server.begin();  
  Serial.println("Setup Webinterface Done  ");

}


/** 
void WebRender::setupWebInterfaceOld(AsyncWebServer* server){ 
   server->on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        saveMessage=false;
        if (request->hasParam(PARAM_SAVEMESSAGE)) {
          saveMessage=true;
        }
        request->send_P(200, "text/html", html_form, formprocessor);
    });
  
    // Send a POST request to <IP>/post with a form field message set to <message>
   
    server->on("/enableTibber", HTTP_POST, [](AsyncWebServerRequest *request){
        String message;
        config.tibberEnabled = false ;
         if (request->hasParam(PARAM_ENABLE_TIBBER, true)) {
          message = request->getParam(PARAM_ENABLE_TIBBER, true)->value();
          String val = String(request->getParam(PARAM_ENABLE_TIBBER, true)->value());
          std::string str_message = String(val).c_str();
          if (str_message.compare(std::string(PARAM_ENABLE_TIBBER))==0){
            config.tibberEnabled = true;
          } 
        } 
        
      request->redirect("/?savemessage=true");
    });
/**
    server->on("/", HTTP_POST, [](AsyncWebServerRequest *request){
        String message;

       if (request->hasParam(PARAM_TIBBER_USERNAME, true)) {
          message = request->getParam(PARAM_TIBBER_USERNAME, true)->value();
          config.tibberUsername = String(message);            
        } 
        else if (request->hasParam(PARAM_TIBBER_URL, true)) {
          message = request->getParam(PARAM_TIBBER_URL, true)->value();
          config.tibberUrl = String(message);            
        }
        else if (request->hasParam(PARAM_TIBBER_TRHRESH, true)) {
          message = request->getParam(PARAM_TIBBER_TRHRESH, true)->value();
          String messageStr = String(message);
          config.tibberThresholdCnts = std::stol(messageStr.c_str());            
        }        
        request->redirect("/?savemessage=true");
    });
**/ 
 //   server->onNotFound(notFound);

   // server->begin();

    //Serial.println("setup webServer is done");
//}

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
