Monitoring System in Factory - Hệ Thống Giám Sát Thiết Bị Trong Nhà Máy
==============================
This repository contains a proof-of-concept implementation of a Monitoring 
System in Factory using LoRaWAN technology. It includes a single channel 
LoRaWAN gateway(from ESP8266, and SX1278 tranceiver) and 4 LoRaWAN node
(from ATmega328P and DHT11, DHT22 sensor, PZEM004T).

The code is for testing and development purposes only, and is not meant 
for production usage. 

Engine for gateway is based on code base of Single Channel gateway for RaspberryPI
which is developed by Thomas Telkamp. Code was ported and extended to run
on ESP 8266 mcu and provide RTC, Webserver and DNS services.

Topology for this project : 4 nodes (2 measure humidity and temperature; 2 measure ac energy).
A LoRaWAN gateway pushes data from these 4 nodes to network server TTN. Data then is forwarded 
to IoT Cloud ThingSpeak. Source code, lib and hardware are saved in this repository.

Maintained by Minh Chau (huynhminhchau.k14@gmail.com)

Features
--------
- listen on configurable frequency and spreading factor
- SF7 to SF12
- status updates
- can forward to one server
- DNS support for server lookup
- NTP Support for time sync with internet time servers
- Webserver support (default port 8080)
- ABP Activate
- frequence 433 MHz : 410 - 525 MHz

Not (yet) supported:
- PACKET_PUSH_ACK processing
- SF7BW250 modulation
- FSK modulation
- downstream messages (tx)

Dependencies
------------

- [gBase64][1] library by Adam Rudd is now integrated in the project, no need to install
- [Time][2] library Arduino [documentation][3]
- lmic lib [4]

Connections
-----------
In Altium Designer of this repository


Configuration
-------------

Defaults: gateway, node

- LoRa:   SF7 at 486.3 Mhz
- Server: router.cn.thethings.network, port 1700  (The Things Network: router.cn.thethings.network)

Edit .h file (ESP-sc-gway.h) to change configuration (look for: "Configure these values!").

Please set location, email and description.

License
-------
The source files in this repository are made available under the Arduino IDE
v1.8.13, except for the base64 implementation, that has been
copied from the Semtech Packet Forwader.

[1]: https://github.com/adamvr/arduino-base64
[2]: https://github.com/PaulStoffregen/Time
[3]: http://playground.arduino.cc/code/time
[4]: https://github.com/HeThongGiamSatTrongNhaMay/LuanVanTotNghiep/tree/main/lib
