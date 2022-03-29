# Opensource Smart Aquascape

## _Next Level Aquascape Base On ESP8266_

This is a Aquascape management device.

- 👨‍💻 C++
- 🛠 Arduino IDE
- 💧 Water
- 🌱 Plant
- 🐟 Fish
- ⚡ Power

## Features

- Auto Offline Mode
- OTA Update
- One Smart Button
- ~~Web Controll~~
- MQTT Controll
- RTC
- NTP
- EEPROM Storage
- 2 Channel AC (relay)
- Temperature Monitor
- Buzzer/Tone


### Channel Configuration

1. Lamp Controll
   - Schaduled (One time long lighting)
   - [Diana Walstad](https://acquariofilia.org/english-articles/af-interviews-diana-walstad/5/) (Periodic Short Light / Low Tech Tank)
   - Force On with Timer
2. Filter Controll
	- Always ON
	- Schaduled Filter

## How To Use

### Prepare Hardware Supported

- ESP8266
- RTC DS1307
- DS18B20 Temperature Sensor
- 1 Push Button
- Relay module 2 channel
- 2 Female Plug AC
- Some AC Wire
- Some Jumper Wire

### Wiring
![Smart Aquascape ESP8266 Wiring](https://github.com/arinadi/Smart-Aquascape-ESP8266/blob/main/SmartAquascapeWiring.jpg?raw=true)

### Flashing
1. rename config.h.example to config.h
2. change configuration following your own
3. Upload with Arduino IDE via USB after that you can use OTA Upload