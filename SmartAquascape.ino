#include <EEPROM.h>
#include "RTClib.h"

RTC_DS1307 rtc;

//Config File (connectifity and pinout)
#include "config.h"


void setup()
{
  // Start
  Serial.begin(115200);
  // #ifndef ESP8266
  //   while (!Serial); // wait for serial port to connect. Needed for native USB
  // #endif


  //init EEPROM
  EEPROM.begin(eepromSize);

  //init relay
  initRelay();
  toggledRelay(1, 0);
  //init ForceOnBtn
  initForceOnBtn();
  //init RTC
  initRTC();
  //init led
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH);
}

void initForceOnBtn() {
  pinMode(pinForceOnBtn, INPUT_PULLUP);
}

void initRelay()
{
  for (int i = 0; i < relays; i++)
  {
    pinMode(pinRelay[i], OUTPUT); // set pinMode to OUTPUT
    stateRelay[i] = EEPROM.read(eepromAddressRelay[i]);
    if (stateRelay[i] == 1)
    {
      digitalWrite(pinRelay[i], LOW); //turn on the relay
    }
    else
    {
      stateRelay[i] = 0;
      EEPROM.write(eepromAddressRelay[i], stateRelay[i]);
      EEPROM.commit();
      digitalWrite(pinRelay[i], HIGH); //turn on the relayoff the relay
    }

    Serial.print("Relay ");
    Serial.print(i);
    Serial.print("State ");
    Serial.print(stateRelay[i]);
    Serial.println("");
  }
}

void initRTC()
{
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    Serial.print("DS1307 configured Time=");
    Serial.print(__TIME__);
    
    Serial.print(", Date=");
    Serial.println(__DATE__);
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  // When time needs to be re-set on a previously configured device, the
  // following line sets the RTC to the date & time this sketch was compiled
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));

}

void toggledRelay(int index, int state)
{
  int i = index;
  if (stateRelay[i] != state)
  {
    if (state == 1)
    {
      stateRelay[i] = state;
      EEPROM.write(eepromAddressRelay[i], stateRelay[i]);
      EEPROM.commit();
      digitalWrite(pinRelay[i], LOW); //turn on the relay
      Serial.print("Relay ");
      Serial.print(i);
      Serial.println(" Turn On");
    }
    else
    {
      stateRelay[i] = state;
      EEPROM.write(eepromAddressRelay[i], stateRelay[i]);
      EEPROM.commit();
      digitalWrite(pinRelay[i], HIGH); //turn off the relay
      Serial.print("Relay ");
      Serial.print(i);
      Serial.println(" Turn Off");
    }
  }

  Serial.print("State ");
  Serial.print(stateRelay[i]);
  Serial.println("");
}

void toggledForceOnBtn()
{
  if (stateForceOnBtn == 0)
  {
    stateForceOnBtn = 1;
    Serial.println("BTN Turn On");
  }
  else
  {
    stateForceOnBtn = 0;
    Serial.println("BTN Turn Off");
  }

  Serial.print("State ");
  Serial.print(stateForceOnBtn);
  Serial.println("");
}

DateTime checkRTC()
{
    DateTime now = rtc.now();

    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" (");
    Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
    Serial.print(") ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();

    // Serial.print(" since midnight 1/1/1970 = ");
    // Serial.print(now.unixtime());
    // Serial.print("s = ");
    // Serial.print(now.unixtime() / 86400L);
    // Serial.println("d");

    // calculate a date which is 7 days, 12 hours, 30 minutes, and 6 seconds into the future
    // DateTime future (now + TimeSpan(7,12,30,6));

    // Serial.print(" now + 7d + 12h + 30m + 6s: ");

    // Serial.print(future.year(), DEC);
    // Serial.print('/');
    // Serial.print(future.month(), DEC);
    // Serial.print('/');
    // Serial.print(future.day(), DEC);
    // Serial.print(' ');
    // Serial.print(future.hour(), DEC);
    // Serial.print(':');
    // Serial.print(future.minute(), DEC);
    // Serial.print(':');
    // Serial.print(future.second(), DEC);
    // Serial.println();

    Serial.println();

    return now;
}

void loop()
{
  //DONT USE DELAY()
  nowMillis = millis();

  int pushForceOnBtn = digitalRead(pinForceOnBtn);

  if (nowMillis - lastPushButton > 3 * 1000 && pushForceOnBtn == 0) {
    lastPushButton = nowMillis;
    toggledForceOnBtn();
  }

  if (nowMillis - lastCheck > 1 * 1000) //Check every seconds
  {
    lastCheck = nowMillis;
    now = checkRTC();
    if(stateForceOnBtn == 0) {
      int hours = now.hour();
      if(hours >= 12 && hours < 20) {
        Serial.println("Lamp Photosynthesis : ON");
        toggledRelay(0, 1);
      } else {
        Serial.println("Lamp Photosynthesis : off");
        toggledRelay(0, 0);
      }
    } else {
      Serial.println("Lamp Photosynthesis : ON");
      toggledRelay(0, 1);
    }
  }
  //DONT USE DELAY()
}
