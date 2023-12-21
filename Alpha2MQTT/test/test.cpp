#include "../src/trigger/trigger.cpp"
#include "unity.h"
#include <ctime>
using namespace std;

void setUp(void) {
  // set stuff up here
}

void tearDown(void) {
  // clean stuff up here
}

void testTriggered(void) {
  int socThreshold = 45;
  int powerThreshold =  2000;

  Trigger* t = new Trigger();
  t->init(socThreshold, powerThreshold,-1, "","","");
  
  TEST_ASSERT_TRUE_MESSAGE(t->isTriggeredSun(46,20001),"Test Triggered energy and power high");

  Trigger* triggerTibber = new Trigger();
  t->init(socThreshold, powerThreshold, 30,"","","");

  TEST_ASSERT_TRUE_MESSAGE(triggerTibber->isTriggeredTibber(29),"Test Triggered Throug tibber");

}

void testNotTriggered(void) {
  int socThreshold = 45;
  int powerThreshold =  2000;

  Trigger* t = new Trigger();
  t->init(socThreshold, powerThreshold,-1,"","","");
  
  TEST_ASSERT_FALSE_MESSAGE(t->isTriggeredSun(50,1999),"Test Not Triggered to low power");
  TEST_ASSERT_FALSE_MESSAGE(t->isTriggeredSun(44,20001),"Test Not Triggered to low threshold");
  TEST_ASSERT_FALSE_MESSAGE(t->isTriggeredSun(44,1999),"Test Not Triggered to low threshold and to low energy");

 Trigger* triggerTibber = new Trigger();
 t->init(socThreshold, powerThreshold, 30,"","","");

  TEST_ASSERT_FALSE_MESSAGE(triggerTibber->isTriggeredTibber(31),"Test not Triggered through tibber");

}


void testStoreTriggerData(void) {
  int socThreshold = 45;
  int powerThreshold =  2000;

  Trigger* t = new Trigger(socThreshold, powerThreshold);

  t->recordData(46,2001,33);
  t->recordData(47,2002,34.0);
  
  time_t now = time(0);
  tm *local_time = localtime(&now);
  int hour = local_time->tm_hour;
  Trigger::history r  = t->getHistory(hour);

  TEST_ASSERT_EQUAL_INT16(2002,r.power);
  TEST_ASSERT_EQUAL_INT16(47,r.soc);
  TEST_ASSERT_EQUAL_FLOAT(34.0,r.price);
}
int runUnityTests(void) {
  UNITY_BEGIN();
  RUN_TEST(testTriggered);
  RUN_TEST(testNotTriggered);
  RUN_TEST(testStoreTriggerData);
  return UNITY_END();
}

int main(){

  runUnityTests();
}