Hệ Thống Giám Sát Thiết Bị Trong Nhà Máy - Monitoring System in Factory
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

Not (yet) supported:
- PACKET_PUSH_ACK processing
- SF7BW250 modulation
- FSK modulation
- downstream messages (tx)

Added features if you're using [WeMos-Lora][3] Shield as gateway
- 2 On board RGB LED for visual 
- 1 SSD1306 I2C OLED connector
- 1 On board push button

Assembled WeMos-Lora Shield

<img src="https://raw.githubusercontent.com/hallard/WeMos-Lora/master/WeMos-Lora-top-assembled.jpg" alt="Top">    
<img src="https://raw.githubusercontent.com/hallard/WeMos-Lora/master/WeMos-Lora-bot-assembled.jpg" alt="Bottom">    


Dependencies
------------

- [gBase64][7] library by Adam Rudd is now integrated in the project, no need to install
- [Time][5] library Arduino [documentation][6]
- [NeoPixelBus][4] library is you're using [WeMos Lora][3] Shield as gateway

Connections
-----------
See [things4u][8] in the [hardware][9] section for building and connection instructions
See [WeMos-Lora][3] github if you're using WeMos Lora Shield as gateway


Configuration
-------------

Defaults:

- LoRa:   SF7 at 868.1 Mhz
- Server: 54.229.214.112, port 1700  (The Things Network: croft.thethings.girovito.nl)
  or directly croft.thethings.girovito.nl

Edit .h file (ESP-sc-gway.h) to change configuration (look for: "Configure these values!").

Please set location, email and description.

License
-------
The source files in this repository are made available under the Eclipse
Public License v1.0, except for the base64 implementation, that has been
copied from the Semtech Packet Forwader.


[2]: https://hallard.me
[3]: https://github.com/hallard/WeMos-Lora
[4]: https://github.com/Makuna/NeoPixelBus
[5]: https://github.com/PaulStoffregen/Time
[6]: http://playground.arduino.cc/code/time
[7]: https://github.com/adamvr/arduino-base64
[8]: http://things4u.github.io
[9]: http://things4u.github.io/HardwareGuide/hardware_guide.html
