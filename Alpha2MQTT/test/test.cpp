#include <ArduinoUnit.h>
#include "../trigger/trigger.h"

class MyTest : public Test
{
public:
  uint16_t when;
  MyTest(const char *name) 
  : Test(name)
  {
    when = random(100,200);
  }

  void loop()
  {
    if (millis() >= when) 
    {
      assertLess(random(100),50);
      pass(); // if assertion is ok
    }
  }  
};

