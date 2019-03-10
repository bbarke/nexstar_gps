# SkySync GPS clone

A project to build an open source Celestron SkySync GPS compatible unit.

Forked and inspired by fantastic project https://github.com/bebrown/nexstar_gps.

## Goals

Construct a SkySync-compatible unit for attaching a GPS to Celestron mounts that don't already have a GPS, such as the CGEM or AVX.

## Parts

To build this, you will need:

* A GPS module, such as the uBlox NEO-6M. Commonly used by hoobyists for their quadcopters.
  Available for about $16.

* A small Arduino, such as the Pro Mini (5V version). Clones available for about $3.

* A TTL serial-to-USB adapter for programming the Arduino (if it does not have an on-board
  USB port already). Available for about $3.

## Construction

See [schematics diagram](skysync_gps_clone.png). 

## Libraries

You will need to download and install the TinyGPS++ library http://arduiniana.org/libraries/tinygpsplus/.

Also needed is the SoftwareSerial library, which is built in to the Arduino IDE.

## Additional info

See this links for more info about Celestron mounts, AUX port, wiring and communication protocol:

https://www.nexstarsite.com/download/manuals/NexStarCommunicationProtocolV1.2.zip
https://sites.google.com/site/wayneholder/nexstar-direct-telescope-mount-control
https://www.nexstarsite.com/download/CelestronAS_PCinterface.pdf
http://www.paquettefamily.ca/nexstar/NexStar_AUX_Commands_10.pdf

## Attention

Be attentive during construction and testing your device!!! You can easy destroy the motherboard in your astro mount.

