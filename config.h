//Config EEPROM
const int eepromSize = 512;

//Config RELAY
const int relays = 2;
const int pinRelay[relays] = {
    14, //relay0
    12, //relay1
};
int eepromAddressRelay[relays] = {
    0, //relay0
    1, //relay1
};
int stateRelay[relays];

// Force ON button
int stateForceOnBtn = 0;
const int pinForceOnBtn = 13;


//millis - time to wait for
long nowMillis = 0;
long lastCheck = 0;
long lastPushButton = 0;

//RTC
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
DateTime now = DateTime();
