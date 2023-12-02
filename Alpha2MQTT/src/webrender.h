#include <stdio.h>
#include <stdlib.h>
#include <ESPAsyncWebServer.h>

class WebRender
{

  #define TIBBENABLEDLEN 2 
  #define TIBBERKEYLEN 64
  #define TIBBERURLLEN 128
  

  public:
    static void setupWebInterface();
    static void dynaCallback();
    static void handleRoot(AsyncWebServerRequest *request);
  
};



