#include <ESP_EEPROM.h>
#include "RTClib.h"
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
// #include <ESP8266WebServer.h>
#include "OneButton.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// Config File (connectifity and pinout)
#include "config.h"

// Tone Pitches
#include "pitches.h"

// Prototypes
void makeTone(const int frequencyHz, const unsigned long durationMs, const unsigned int repeat = 1);

void setup()
{
  // Start
  Serial.begin(115200);
  // #ifndef ESP8266
  //   while (!Serial); // wait for serial port to connect. Needed for native USB
  // #endif

  // init EEPROM
  initEEPROM();

  // init buzzer
  makeTone(NOTE_C5, 500);

  // init relay
  initRelay();

  // init led
  //  pinMode(LED_BUILTIN, OUTPUT);

  // init SmartBtn
  initSmartBtn();

  // init Wifi
  initWifi();

  // init RTC
  initRTC();

  // init NTP
  initNTP();

  // Init DS18B20
  sensorTemperature.begin();

  if (onlineMode == 1)
  {
    // espClient->setInsecure(); // you can use the insecure mode, when you want to avoid the certificates
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    // init OTA
    initOta();

    // init MQTT
    initMqtt();

    // init Server
    // initServer();
  }
}

// void initServer()
// {
//   if (serviceWebServer)
//   {
//     server.on("/restart", []()
//               {
//               server.send(200, "text/plain", "Restarting...");
//               delay(3000);
//               ESP.restart(); });
//     server.begin();
//   }
// }

void initEEPROM()
{
  EEPROM.begin(eepromSize);
  EEPROM.get(0, myDataEEPROM);
  delay(500);
  Serial.println("EEPROM load: ");
  Serial.println(myDataEEPROM.test);
  if (myDataEEPROM.test != 1)
  {
    resetEEPROM();
  }

  if (myDataEEPROM.lightingMode == 2)
  {
    updateDianaLighting();
  }
}

void updateEEPROM()
{
  EEPROM.put(0, myDataEEPROM);
  if (EEPROM.commit())
  {
    Serial.println("EEPROM successfully committed!");
    Serial.println(myDataEEPROM.test);
  }
  else
  {
    Serial.println("ERROR! EEPROM commit failed!");
  }
}

void resetEEPROM()
{
  boolean result = EEPROM.wipe();
  if (result)
  {
    Serial.println("All EEPROM data wiped");
  }
  else
  {
    Serial.println("EEPROM data could not be wiped from flash store");
  }

  dataEEPROM myDataEEPROM;
  Serial.println("EEPROM reseting");
  Serial.println(myDataEEPROM.test);
  EEPROM.put(0, myDataEEPROM);
  boolean ok = EEPROM.commitReset();
  Serial.println((ok) ? "Commit (Reset) OK" : "Commit failed");
  Serial.println(myDataEEPROM.test);
  updateDianaLighting();
}

void initRTC()
{
  if (serviceRtc)
  {

    if (!rtc.begin())
    {
      Serial.println("Couldn't find RTC");
      Serial.flush();
      while (1)
        delay(10);
    }

    if (!rtc.isrunning())
    {
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

    now = checkRTC();
    // When time needs to be re-set on a previously configured device, the
    // following line sets the RTC to the date & time this sketch was compiled
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
}
void initNTP()
{
  if (onlineMode == 1)
  {
    if (serviceNtp)
    {
      timeClient.begin();
      // Set offset time in seconds to adjust for your timezone, for example:
      // GMT +1 = 3600
      // GMT +8 = 28800
      // GMT -1 = -3600
      // GMT 0 = 0
      timeClient.setTimeOffset(myTimezone * 3600);
      timeClient.update();
      autoAdjustTime();
    }
  }
}

void autoAdjustTime()
{
  if (onlineMode == 1)
  {
    Serial.println("RTC :");
    Serial.println(now.hour());
    Serial.println("NTP :");
    Serial.println(timeClient.getHours());
    if (now.hour() != timeClient.getHours())
    {
      Serial.println("Let's set the time!");

      unsigned long epochTime = timeClient.getEpochTime();
      // Get a time structure
      struct tm *ptm = gmtime((time_t *)&epochTime);
      int monthDay = ptm->tm_mday;
      int month = ptm->tm_mon + 1;
      int year = ptm->tm_year + 1900;

      rtc.adjust(DateTime(year, month, monthDay, timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds()));

      now = checkRTC();
      Serial.println("RTC :");
      Serial.println(now.hour());
      Serial.println("NTP :");
      Serial.println(timeClient.getHours());
    }
  }
}

void toggledRelay(int index, int state)
{
  if (serviceRelay)
  {
    int i = index;
    if (myDataEEPROM.relayState[i] != state)
    {
      if (state == 1)
      {
        digitalWrite(pinRelay[i], LOW); // turn on the relay
        Serial.println(" Turn On");
      }
      else
      {
        digitalWrite(pinRelay[i], HIGH); // turn off the relay
        Serial.println(" Turn Off");
      }
      myDataEEPROM.relayState[i] = state;
      updateEEPROM();
      Serial.print("Relay ");
      Serial.print(i);
      Serial.print(" State ");
      Serial.print(myDataEEPROM.relayState[i]);
      Serial.println("");
      makeTone(NOTE_G5, 1000);
      if (i == 0)
      {
        publishMqtt(topic.relay0, (myDataEEPROM.relayState[i]) ? "ON" : "OFF");
      }
      if (i == 1)
      {
        publishMqtt(topic.relay1, (myDataEEPROM.relayState[i]) ? "ON" : "OFF");
      }
    }
  }
}

void initSmartBtn()
{
  if (serviceOneButton)
  {
    pinMode(pinSmartBtn, INPUT_PULLUP);

    // force on Lamp
    smartBtn.attachClick(toggledForceOn);

    // restart OpenSA
    smartBtn.attachLongPressStop(
        []()
        { ESP.restart(); });
  }
}

void toggledForceOn()
{
  if (stateForceOn == 0)
  {
    forceOnStart = nowMillis;
    stateForceOn = 1;
    Serial.println("BTN Turn On");
  }
  else
  {
    forceOnStart = 0;
    stateForceOn = 0;
    Serial.println("BTN Turn Off");
  }

  Serial.print("State ");
  Serial.print(stateForceOn);
  Serial.println("");
}

void lampController()
{
  if (stateForceOn == 1 && nowMillis - forceOnStart > myDataEEPROM.forceOnTimeout * 60 * 1000)
  {
    Serial.println("Lamp : Time out");
    stateForceOn = 0;
  }
  if (stateForceOn == 0)
  {
    if (myDataEEPROM.lightingMode == 1)
    {
      longLighing();
    }
    if (myDataEEPROM.lightingMode == 2)
    {
      dianaLighting();
    }
  }
  else
  {
    toggledRelay(0, 1);
  }
}

void longLighing()
{
  int hours = now.hour();
  if (hours >= myDataEEPROM.relay0HoursStart && hours < myDataEEPROM.relay0HoursEnd)
  {
    toggledRelay(0, 1);
  }
  else
  {
    toggledRelay(0, 0);
  }
}

void dianaLighting() // @Todo : Not Working
{
  int hours = now.hour();
  bool onSchedule = false;
  for (int i = 0; i < myDataEEPROM.lightingDianaCountSchedule; i++)
  {
    if (hours >= myDataEEPROM.lightingDianaScheduleStart[i] && hours < myDataEEPROM.lightingDianaScheduleEnd[i])
    {
      onSchedule = true;
      break;
    }
  }
  if (onSchedule)
  {
    toggledRelay(0, 1);
  }
  else
  {
    toggledRelay(0, 0);
  }
}

void updateDianaLighting()
{
  int nextStart = myDataEEPROM.lightingDianaStart;
  int i = 0;
  while (nextStart < 24)
  {
    Serial.println(i);
    myDataEEPROM.lightingDianaScheduleStart[i] = nextStart;
    Serial.println(myDataEEPROM.lightingDianaScheduleStart[i]);
    myDataEEPROM.lightingDianaScheduleEnd[i] = nextStart + myDataEEPROM.lightingDianaDuration;
    Serial.println(myDataEEPROM.lightingDianaScheduleEnd[i]);
    nextStart = myDataEEPROM.lightingDianaScheduleEnd[i] + (myDataEEPROM.lightingDianaDuration - 1);
    Serial.println(nextStart);
    i++;
    myDataEEPROM.lightingDianaCountSchedule = i;
    Serial.println(myDataEEPROM.lightingDianaCountSchedule);
    Serial.println("========");
  }

  updateEEPROM();
}

void filterController()
{
  if (myDataEEPROM.filterMode == 1)
  {
    alwaysOnFilter();
  }
  if (myDataEEPROM.filterMode == 2)
  {
    schaduledFilter();
  }
}

void schaduledFilter()
{
  int hours = now.hour();
  if (hours >= myDataEEPROM.relay1HoursStart && hours < myDataEEPROM.relay1HoursEnd)
  {
    toggledRelay(1, 1);
  }
  else
  {
    toggledRelay(1, 0);
  }
}

void alwaysOnFilter()
{
  toggledRelay(1, 1);
}

void initRelay()
{
  if (serviceRelay)
  {
    for (int i = 0; i < relays; i++)
    {
      pinMode(pinRelay[i], OUTPUT); // set pinMode to OUTPUT
      if (myDataEEPROM.relayState[i] == 1)
      {
        digitalWrite(pinRelay[i], LOW); // turn on the relay
      }
      else
      {
        digitalWrite(pinRelay[i], HIGH); // turn off the relay
      }

      Serial.print("Relay ");
      Serial.print(i);
      Serial.print("State ");
      Serial.print(myDataEEPROM.relayState[i]);
      Serial.println("");
    }
  }
}

DateTime checkRTC()
{
  DateTime now = rtc.now();

  // Serial.print(now.year(), DEC);
  // Serial.print('/');
  // Serial.print(now.month(), DEC);
  // Serial.print('/');
  // Serial.print(now.day(), DEC);
  // Serial.print(" (");
  // Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  // Serial.print(") ");
  // Serial.print(now.hour(), DEC);
  // Serial.print(':');
  // Serial.print(now.minute(), DEC);
  // Serial.print(':');
  // Serial.print(now.second(), DEC);
  // Serial.println();

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

  // Serial.println();

  return now;
}

void checkSchedule()
{
  if (serviceRtc)
  {
    if (nowMillis - lastCheckSchedule > myDataEEPROM.checkScheduleInterval * 1000) // Check every seconds
    {
      lastCheckSchedule = nowMillis;
      now = checkRTC();
      // lampController or relay0
      lampController();
      filterController();
    }
  }
}

void initWifi()
{
  // We start by connecting to a WiFi network
  Serial.println();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  checkWiFi();
}

void checkWiFi()
{
  if (onlineMode == 0 && nowMillis - lastCheckWifi > nextWiFiReconnect)
  {
    onlineMode = 1;
  }

  if (onlineMode == 1)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.print("Connecting to ");
      Serial.println(ssid);
    }

    lastCheckWifi = nowMillis;
    while (WiFi.status() != WL_CONNECTED)
    {
      nowMillis = millis();
      Serial.print(".");
      delay(1000);
      if (nowMillis - lastCheckWifi > checkWiFiTimeout * 1000)
      {
        offlineMode();
        break;
      }
    }
  }
}

void offlineMode()
{
  onlineMode = 0;
  makeTone(NOTE_A5, 1000, 3);
  nextWiFiReconnect = lastCheckWifi + (checkWiFiReconnect * 1000 * 60);
}

void initMqtt()
{
  if (serviceMqtt)
  {
    // connecting to a mqtt broker
    mqttCore.setServer(mqttBroker, mqttPort);
    mqttCore.setCallback(callback);
  }
}

void checkMqtt()
{
  if (serviceMqtt)
  {
    lastCheckMqtt = nowMillis;
    // Loop until we're reconnected
    while (!mqttCore.connected())
    {
      nowMillis = millis();
      // digitalWrite(LED_BUILTIN, LOW);
      String client_id = "OpenSA-client-";
      client_id += String(WiFi.macAddress());
      Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
      if (mqttCore.connect(client_id.c_str(), mqttUsername, mqttPassword))
      {
        Serial.println("broker connected");

        Serial.println("subscribe to :");
        Serial.println(topic.main);
        Serial.println((mqttCore.subscribe(topic.main)) ? "OK" : "failed");

        // publish
        publishMqtt(topic.hello, "OpenSA MQTT Ready");
      }
      else
      {
        Serial.print("failed with state ");
        Serial.print(mqttCore.state());
        delay(1000);
      }
      // digitalWrite(LED_BUILTIN, HIGH);
      Serial.print(".");
      delay(1000);
      if (nowMillis - lastCheckMqtt > checkMqttTimeout * 1000)
      {
        makeTone(NOTE_A5, 1000, 2);
        serviceMqtt = false;
        break;
      }
    }
    mqttCore.loop(); // handle callback
  }
}

void publishMqtt(const char *topic, const char *message)
{
  if (serviceMqtt)
  {
    mqttCore.publish(topic, message);
  }
}
void publishDeviceConfig()
{
  if (serviceMqtt)
  {
    Serial.println("Sending device configuration...");
    publishMqtt(topic.forceOnTimeout, String(myDataEEPROM.forceOnTimeout).c_str());
    publishMqtt(topic.relay0, (myDataEEPROM.relayState[0]) ? "ON" : "OFF");
    publishMqtt(topic.relay1, (myDataEEPROM.relayState[1]) ? "ON" : "OFF");
    publishMqtt(topic.temperatureMonitor, (myDataEEPROM.temperatureMonitor) ? "ON" : "OFF");
    publishMqtt(topic.temperatureInterval, String(myDataEEPROM.temperatureInterval).c_str());
    publishLightConfig();
    publishFilterConfig();

    publishDeviceInfos();

    Serial.println("Done");
  }
}

void publishDeviceInfos()
{
  checkTemperature();
  publishMqtt(topic.hello, String("IP = " + WiFi.localIP().toString()).c_str());
  publishTimeInfo();
  publishLightInfo();
}

void publishFilterConfig()
{
  publishMqtt(topic.filterMode, String(myDataEEPROM.filterMode).c_str());
  if (myDataEEPROM.filterMode == 2)
  {
    publishMqtt(topic.relay1Start, String(myDataEEPROM.relay1HoursStart).c_str());
    publishMqtt(topic.relay1End, String(myDataEEPROM.relay1HoursEnd).c_str());
  }
}

void publishLightConfig()
{
  publishMqtt(topic.lightingMode, String(myDataEEPROM.lightingMode).c_str());
  if (myDataEEPROM.lightingMode == 1)
  {
    publishMqtt(topic.relay0Start, String(myDataEEPROM.relay0HoursStart).c_str());
    publishMqtt(topic.relay0End, String(myDataEEPROM.relay0HoursEnd).c_str());
  }
  if (myDataEEPROM.lightingMode == 2)
  {
    publishMqtt(topic.lightingDianaStart, String(myDataEEPROM.lightingDianaStart).c_str());
    publishMqtt(topic.lightingDianaDuration, String(myDataEEPROM.lightingDianaDuration).c_str());
  }
}

void publishLightInfo()
{
  if (myDataEEPROM.lightingMode == 2)
  {
    String dianaSchedule = String(myDataEEPROM.lightingDianaCountSchedule + " DS S:E = ");
    for (int i = 0; i < myDataEEPROM.lightingDianaCountSchedule; i++)
    {
      dianaSchedule += String(myDataEEPROM.lightingDianaScheduleStart[i]) + ":" + String(myDataEEPROM.lightingDianaScheduleEnd[i]) + ", ";
    }
    publishMqtt(topic.hello, dianaSchedule.c_str());
  }
}

void publishTimeInfo()
{
  String timeNow;
  timeClient.update();
  now = checkRTC();
  timeNow += String("RTC = " + String(now.hour()) + " | ");
  timeNow += String("NTP = " + String(timeClient.getHours()) + " | ");
  publishMqtt(topic.hello, timeNow.c_str());
}

// MQTT Subscription Handlers
void callback(char *cbtopic, byte *message, unsigned int length)
{
  Serial.print("Arrived in topic: ");
  Serial.println(cbtopic);
  Serial.print("Message:");
  String messageTemp;
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
  Serial.println("-----------------------");

  // handle relay 0
  if (String(cbtopic) == topic.relay0)
  {
    if (messageTemp == "T")
    {
      toggledForceOn();
    }
  }

  if (String(cbtopic) == topic.filterMode)
  {
    if (messageTemp.toInt() == 1 || messageTemp.toInt() == 2)
    {
      if (myDataEEPROM.filterMode != messageTemp.toInt())
      {
        myDataEEPROM.filterMode = messageTemp.toInt();
        updateEEPROM();
      }
    }
  }

  if (String(cbtopic) == topic.lightingMode)
  {
    if (messageTemp.toInt() == 1 || messageTemp.toInt() == 2)
    {
      if (myDataEEPROM.lightingMode != messageTemp.toInt())
      {
        myDataEEPROM.lightingMode = messageTemp.toInt();
        updateEEPROM();
      }
    }
  }

  if (String(cbtopic) == topic.lightingDianaStart)
  {
    if (messageTemp.toInt() >= 0 && messageTemp.toInt() <= 24)
    {
      if (myDataEEPROM.lightingDianaStart != messageTemp.toInt())
      {
        myDataEEPROM.lightingDianaStart = messageTemp.toInt();
        updateDianaLighting();
      }
    }
  }

  if (String(cbtopic) == topic.lightingDianaDuration)
  {
    if (messageTemp.toInt() >= 2 && messageTemp.toInt() <= 6)
    {
      if (myDataEEPROM.lightingDianaDuration != messageTemp.toInt())
      {
        myDataEEPROM.lightingDianaDuration = messageTemp.toInt();
        updateDianaLighting();
      }
    }
  }

  if (String(cbtopic) == topic.relay0Start)
  {
    if (messageTemp.toInt() >= 0 && messageTemp.toInt() <= 24)
    {
      if (myDataEEPROM.relay0HoursStart != messageTemp.toInt())
      {
        myDataEEPROM.relay0HoursStart = messageTemp.toInt();
        updateEEPROM();
      }
    }
  }

  if (String(cbtopic) == topic.relay0End)
  {
    if (messageTemp.toInt() >= 0 && messageTemp.toInt() <= 24)
    {
      if (myDataEEPROM.relay0HoursEnd != messageTemp.toInt())
      {
        myDataEEPROM.relay0HoursEnd = messageTemp.toInt();
        updateEEPROM();
      }
    }
  }

  if (String(cbtopic) == topic.relay1Start)
  {
    if (messageTemp.toInt() >= 0 && messageTemp.toInt() <= 24)
    {
      if (myDataEEPROM.relay1HoursStart != messageTemp.toInt())
      {
        myDataEEPROM.relay1HoursStart = messageTemp.toInt();
        updateEEPROM();
      }
    }
  }

  if (String(cbtopic) == topic.relay1End)
  {
    if (messageTemp.toInt() >= 0 && messageTemp.toInt() <= 24)
    {
      if (myDataEEPROM.relay1HoursEnd != messageTemp.toInt())
      {
        myDataEEPROM.relay1HoursEnd = messageTemp.toInt();
        updateEEPROM();
      }
    }
  }

  if (String(cbtopic) == topic.temperatureInterval)
  {
    if (messageTemp.toInt() >= 1 && messageTemp.toInt() <= 60)
    {
      if (myDataEEPROM.temperatureInterval != messageTemp.toInt())
      {
        myDataEEPROM.temperatureInterval = messageTemp.toInt();
        updateEEPROM();
      }
    }
  }

  if (String(cbtopic) == topic.temperatureMonitor)
  {
    if (messageTemp == "T")
    {
      toggledTemperatureMonitor();
    }
  }

  if (String(cbtopic) == topic.forceOnTimeout)
  {
    if (myDataEEPROM.forceOnTimeout != messageTemp.toInt())
    {
      myDataEEPROM.forceOnTimeout = messageTemp.toInt();
      updateEEPROM();
    }
  }

  if (String(cbtopic) == topic.commands)
  {
    if (messageTemp == "get-config")
    {
      publishDeviceConfig();
    }
    if (messageTemp == "get-infos")
    {
      publishDeviceInfos();
    }
    if (messageTemp == "time-adjust")
    {
      autoAdjustTime();
      publishTimeInfo();
    }
    if (messageTemp == "restart-now")
    {
      publishMqtt(topic.hello, "Restarting Now");
      ESP.restart();
    }
    if (messageTemp == "reset-eeprom")
    {
      resetEEPROM();
      publishDeviceConfig();
    }
  }
}

void initOta()
{
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(otaHostname);

  // No authentication by default
  ArduinoOTA.setPassword(otaPassword);

  ArduinoOTA.onStart([]()
                     {
                       String type;
                       if (ArduinoOTA.getCommand() == U_FLASH)
                       {
                         type = "sketch";
                       }
                       else
                       { // U_FS
                         type = "filesystem";
                       }

                       // NOTE: if updating FS this would be the place to unmount FS using FS.end()
                       Serial.println("Start updating " + type); });
  ArduinoOTA.onEnd([]()
                   { Serial.println("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });
  ArduinoOTA.onError([](ota_error_t error)
                     {
                       Serial.printf("Error[%u]: ", error);
                       if (error == OTA_AUTH_ERROR)
                       {
                         Serial.println("Auth Failed");
                       }
                       else if (error == OTA_BEGIN_ERROR)
                       {
                         Serial.println("Begin Failed");
                       }
                       else if (error == OTA_CONNECT_ERROR)
                       {
                         Serial.println("Connect Failed");
                       }
                       else if (error == OTA_RECEIVE_ERROR)
                       {
                         Serial.println("Receive Failed");
                       }
                       else if (error == OTA_END_ERROR)
                       {
                         Serial.println("End Failed");
                       } });
  ArduinoOTA.begin();
}

void checkTemperature()
{
  if (serviceDallasTemperature)
  {
    if (myDataEEPROM.temperatureMonitor == 1)
    {
      if (nowMillis - lastCheckTemperature > myDataEEPROM.temperatureInterval * 1000)
      {
        lastCheckTemperature = nowMillis;
        if (onlineMode == 1)
        {
          sensorTemperature.requestTemperatures();
          float temperatureC = sensorTemperature.getTempCByIndex(0);
          char tempString[8];
          dtostrf(temperatureC, 1, 2, tempString);
          Serial.println(temperatureC);
          publishMqtt(topic.temperatureData, tempString);
        }
      }
    }
  }
}

void toggledTemperatureMonitor()
{
  if (serviceDallasTemperature)
  {
    if (myDataEEPROM.temperatureMonitor == 0)
    {
      myDataEEPROM.temperatureMonitor = 1;
    }
    else
    {
      myDataEEPROM.temperatureMonitor = 0;
    }

    updateEEPROM();

    Serial.print("toggledTemperatureMonitor ");
    Serial.print(stateForceOn);
    Serial.println("");
  }
}

void makeTone(const int frequencyHz, const unsigned long durationMs, const unsigned int repeat)
{
  if (serviceBuzzer)
  {
    for (int i = 0; i < repeat; i++)
    {
      tone(pinBuzzer, frequencyHz);
      delay(durationMs);
      noTone(pinBuzzer);
      if (repeat > 1)
      {
        delay(durationMs / 2);
      }
    }
  }
}

void loop()
{
  nowMillis = millis();
  checkWiFi();
  if (onlineMode == 1)
  {
    ArduinoOTA.handle();
    checkMqtt();
  }
  // DONT USE DELAY()

  // keep watching the push button:
  smartBtn.tick();
  checkTemperature();
  checkSchedule();

  // DONT USE DELAY()
}
