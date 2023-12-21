#include <stdio.h>
#include <stdlib.h>
#include <ESPAsyncWebServer.h>

#include <trigger/trigger.h>

class WebRender
{

  #define TIBBENABLEDLEN 2 
  #define TIBBERKEYLEN 64
  #define TIBBERURLLEN 128
  
  public:

    static void setTrigger(Trigger* trigger);
    static void setupWebInterface();
    static void dynaCallback();
    static void handleRoot(AsyncWebServerRequest *request);
    static void convertConfig();
};



