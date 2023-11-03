# NexStar GPS replacement

A project to build a replacement GPS for Celestron's NexStar GPS and CPC telescopes.  Can also be used for creating an AUX port based GPS like the Celestron CN16 or SkySync GPS.

Forked and inspired by fantastic project https://github.com/bebrown/nexstar_gps, https://github.com/ForestTree/nexstar_gps, and https://github.com/LordBeowulf/nexstar_gps.

This firmware version disables the serial output when not in use allowing the Celestron open collector common serial buss to operate correctly without additional circuitry.  It also implements the busy/drop line missing from other implementations.  Finally, it adds support for GNSS receivers like the Beitian BE-220 or uBlox NEO-7M in additon to standard GPS receivers.

This specific branch uses the platform.io (https://platformio.org/) plugin for visual studio. It also adds an OLED screen along with a temperature/humidity sensor (DHT22). 

## Parts

To build this, you will need:

* A GPS module, such as the Beitian BE-220 or the uBlox NEO-6M/7M. Commonly used by hoobyists for their quadcopters.
  Available for about $16.

* A small Arduino, such as the Pro Mini (5V version) or Nano (shown here). Clones available for about $3.

* A TTL serial-to-USB adapter for programming the Arduino (if it does not have an on-board
  USB port already). Available for about $3.

## Construction

See this [schematic diagram](https://github.com/LordBeowulf/nexstar_gps/blob/master/Celestron%20Arduino%20Nano%20GPS.pdf) for the NexStar GPS module.  See this [schematic diagram](https://github.com/LordBeowulf/nexstar_gps/blob/master/Celestron%20Arduino%20Nano%20GPS%20for%20AUX%20port.pdf) for the AUX port version. 

## Libraries

You will need to download and install the TinyGPS++ library http://arduiniana.org/libraries/tinygpsplus/.

Also needed is the SoftwareSerial library, which is built in to the Arduino IDE.

## Additional info

See these links for more info about Celestron mounts, AUX port, wiring and communication protocol:

https://www.nexstarsite.com/download/manuals/NexStarCommunicationProtocolV1.2.zip
https://sites.google.com/site/wayneholder/nexstar-direct-telescope-mount-control
https://www.nexstarsite.com/download/CelestronAS_PCinterface.pdf
http://www.paquettefamily.ca/nexstar/NexStar_AUX_Commands_10.pdf

This thread on Cloudy Nights details the development history of various versions.
https://www.cloudynights.com/topic/567421-homebrew-celestron-compatible-gps/

## Attention

Be attentive during construction and testing your device!!! You can easy destroy the motherboard in your astro mount.

