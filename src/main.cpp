#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
#include <Arduino.h>
#include <wiring_private.h> // cbi/sbi definition
#include <util/atomic.h>

#include "define.h"
#include "oled.h"
#include "scope_temperature.h"

#define GPSRXPIN 2
#define GPSTXPIN 3

#define TXPIN 1
#define DROPPIN 4

#define LEDPIN 13

// centralizing for debug.  Appears internal pullup is too much, at least on TX.
#define TX_INPUT INPUT
#define DROP_INPUT INPUT

#if defined(UCSRB) && defined(UBRRL)
  volatile uint8_t * const _ucsrb = &UCSRB;
#else
  volatile uint8_t * const _ucsrb = &UCSR0B;
#endif


TinyGPSPlus gps;
SoftwareSerial ss(GPSRXPIN, GPSTXPIN);

unsigned long lastTimeUpdate = 1000;

/*
 * Sample output from my BN-220 GNSS module.  Note the missing GPGGA/GPGSA replaced by GNGGA/GNGSA!
 * 
$GNGGA,192828.00,3028.88525,N,09750.20296,W,1,11,0.86,285.0,M,-24.0,M,,*7D
$GNGSA,A,3,17,28,19,30,01,13,07,,,,,,1.59,0.86,1.33*1D
$GNGSA,A,3,87,88,76,78,,,,,,,,,1.59,0.86,1.33*1F
$GPGSV,4,1,15,01,23,044,16,02,02,206,,03,07,093,,06,27,191,15*7B
$GPGSV,4,2,15,07,19,155,20,13,22,255,27,15,07,281,05,17,68,321,28*7D
$GPGSV,4,3,15,19,55,269,33,22,06,068,18,24,02,322,,28,60,029,29*75
$GPGSV,4,4,15,30,50,168,25,46,41,230,28,51,53,198,*4D
$GLGSV,2,1,07,76,35,126,27,77,73,034,,78,27,331,17,81,08,217,*6D
$GLGSV,2,2,07,86,12,030,,87,72,001,24,88,59,228,25*52
$GNGLL,3028.88525,N,09750.20296,W,192828.00,A,A*62
$GNRMC,192829.00,A,3028.88514,N,09750.20293,W,0.128,,010520,,,A*70
$GNVTG,,T,,M,0.128,N,0.236,K,A*31
*/

// Changing these to use GNSS reports.  Should probably make it either/or 
TinyGPSCustom satellitesInView(gps, "GPGSV", 3);
TinyGPSCustom fix3D(gps, "GPGSA", 2);          // 1 = no fix, 2 = 2D fix, 3 = 3D fix
TinyGPSCustom fixQuality(gps, "GPGGA", 6);     // 0 = invalid, 1 = GPS, 2 = DGPS, etc...
TinyGPSCustom fix3DGNSS(gps, "GNGSA", 2);      // 1 = no fix, 2 = 2D fix, 3 = 3D fix
TinyGPSCustom fixQualityGNSS(gps, "GNGGA", 6); // 0 = invalid, 1 = GPS, 2 = DGPS, etc...

// For more details how convert GPS position into 24 bit format,
// see "NexStar Communication Protocol", section "GPS Commands".
// https://www.nexstarsite.com/download/manuals/NexStarCommunicationProtocolV1.2.zip
const double GPS_MULT_FACTOR = 46603.37778;  // = 2^24 / 360


#define PK_MAX_LEN 12
unsigned char packet[PK_MAX_LEN];   // Is 12 enough? What is the largest expected packet?
enum pk_state { PREAMBLE_WAIT, LENGTH_WAIT, DATA, CKSUM, DONE, VALID };
enum pk_state pkstate;
int pklen;
int pkidx;
int16_t cksum_accumulator;

bool LEDState = false;

Oled oled;
ScopeTemperature scopeTemp;

//Declare methods so I don't have to reorder the file
int getGpsQuality();
void packet_decode(int8_t c);
bool pk_checksum(int8_t target);
inline void cksum_init();
inline void cksum_update(uint8_t b);
inline int8_t cksum_final();
void pk_send(uint8_t dest, uint8_t id, uint8_t byte0, uint8_t byte1 = -1, uint8_t byte2 = -1);
// void pk_send(uint8_t dest, uint8_t id, uint8_t byte0);
// void pk_send(uint8_t dest, uint8_t id, uint8_t byte0, uint8_t byte1);
// void pk_send(uint8_t dest, uint8_t id, uint8_t byte0, uint8_t byte1, uint8_t byte2);

void setup() {
  // put your setup code here, to run once:
  pkstate = PREAMBLE_WAIT;
  pklen = 0;
  pkidx = 0;
  Serial.begin(19200);  // Let the serial class initialize everything
                        // The internal bus runs at 19200, not 9600 as in the original code...
  ss.begin(9600);       // Original code didn't start this either...
                        // The BE-220 gps module I bought has a default baud of 38400

  // cbi(*_ucsrb, TXEN0);          // then disable the serial transmitter temporarily  -- should probably do this in a non-interruptable atomic way, but hopefully it won't matter.
  // pinMode(TXPIN, TX_INPUT);     // and make it an input until we need it.
  pinMode(DROPPIN, DROP_INPUT); // Treat this as an open collector output.
  digitalWrite(DROPPIN, 0);     // and go ahead and set output to low

  digitalWrite(LEDPIN, 0);      // LED Off
  pinMode(LEDPIN, OUTPUT);

  oled.begin();
  oled.updateTime(gps);
  oled.updateSatellite(gps);
  oled.updateLatLng(gps);
  scopeTemp.begin();
  oled.updateTemperature(scopeTemp);
}

void loop() {
  // Feed characters from the GPS module into TinyGPS
  bool updated = false;

  while (ss.available()) {
    char c = ss.read();
    // Serial.print(c);

    if (gps.encode(c)) {
      digitalWrite(LEDPIN, 1);
      // Serial.println();
      int interval = 0;
      if (gps.time.isUpdated()) {
        updated = true;
        oled.updateTime(gps);
      }

      if (gps.satellites.isUpdated()) {
        updated = true;
        oled.updateSatellite(gps);
      }

      if (gps.location.isUpdated()) {
        updated = true;
        oled.updateLatLng(gps);
      }

      if (scopeTemp.update()) {
        updated = true;
      }
    }
      digitalWrite(LEDPIN, 0);
  }

  if (scopeTemp.update()) {
    oled.updateTemperature(scopeTemp);
    updated = true;
  }

  if (updated) {
    oled.writeToScreen();
  }

  if (true) {
    return;
  }
    

  // Feed characters from the serial port into the packet decodergit
  while (Serial.available()){
    // packet_decode(Serial.read());
  }

  // Check if packet is valid
  if (pkstate != VALID) return;

//  LEDState = !LEDState;
//  digitalWrite(LEDPIN, LEDState);        // toggle LED for each valid packet seen
  
  // Check that destination is for me
  if (packet[2] != DEV_GPS) {
    pkstate = PREAMBLE_WAIT;
    pkidx = 0;
    pklen = 0;
    return;
  }

  digitalWrite(LEDPIN, 1);        // pulse LED for each packet sent from GPS..

  // It's for me! What's the command?
  uint8_t dest = packet[1];

  // pulling command parser code from ForestTree's fork, since BEBrown's original code never even returned a latitude!  (First thing mount asks for on alignment.)
  switch (packet[3])
  {
    case GPS_LINKED:
    case GPS_TIME_VALID:
      if (getGpsQuality() > 0)
        pk_send(dest, packet[3], 1);
      else
        pk_send(dest, packet[3], 0);
      break;

    case GPS_GET_TIME:
      pk_send(dest, GPS_GET_TIME, gps.time.hour(), gps.time.minute(), gps.time.second());
      break;

    case GPS_GET_HW_VER:
      pk_send(dest, GPS_GET_HW_VER, GPS_HW_VER);
      break;

    case GPS_GET_YEAR:
      pk_send(dest, GPS_GET_YEAR, gps.date.year() >> 8, gps.date.year() & 0xff);
      break;

    case GPS_GET_DATE:
      pk_send(dest, GPS_GET_DATE, gps.date.month(), gps.date.day());
      break;

    case GPS_GET_LAT: {
        int32_t lat = (int32_t) (gps.location.lat() * GPS_MULT_FACTOR);
        uint8_t* latBytePtr = (uint8_t*)&lat;
        pk_send(dest, GPS_GET_LAT, latBytePtr[2], latBytePtr[1], latBytePtr[0]);
        break;
      }

    case GPS_GET_LONG: {
        int32_t lng = (int32_t) (gps.location.lng() * GPS_MULT_FACTOR);
        uint8_t* lngBytePtr = (uint8_t*)&lng;
        pk_send(dest, GPS_GET_LONG, lngBytePtr[2], lngBytePtr[1], lngBytePtr[0]);
        break;
      }

    case GPS_GET_SAT_INFO: {
        String satellitesInViewString(satellitesInView.value());
        pk_send(dest, GPS_GET_SAT_INFO, satellitesInViewString.toInt(), gps.satellites.value());
        break;
      }

    case GPS_GET_RCVR_STATUS:
      break;

    case GPS_GET_COMPASS:
      break;

    case GPS_GET_VER:
      pk_send(dest, GPS_GET_VER, 0, 1);  // Version 0.1
      break;
  }

  digitalWrite(LEDPIN, 0);        // pulse LED for each packet sent from GPS..

  pkstate = PREAMBLE_WAIT;
  pklen = 0;
  pkidx = 0;
}

// Handle both GNSS amd GPS format strings
int getGpsQuality() {
  String quality(fixQualityGNSS.value());
  if (quality.length()) {
    return quality.toInt();
  }
  quality = fixQuality.value();
  if (quality.length()) {
    return quality.toInt();
  }  
  return -1;
}

void packet_decode(int8_t c)
{
  // Serial.write(c);  // Don't see any need to echo this.
  switch (pkstate)
  {
    case PREAMBLE_WAIT:
    if (c == 0x3b) {
      pkstate = LENGTH_WAIT;
    }
    break;

    case LENGTH_WAIT:
    if (c < PK_MAX_LEN) {
      pklen = c;
      packet[0] = c;
      pkidx = 1;
      pkstate = DATA;
    }
    else
      pkstate = PREAMBLE_WAIT;
    break;

    case DATA:
    packet[pkidx] = c;
    pkidx++;
    if (pkidx == pklen + 1)
      pkstate = CKSUM;
    break;

    case CKSUM:
    if (pk_checksum(c))
      pkstate = VALID;
    else
      pkstate = PREAMBLE_WAIT;
    break;
  }
  //Serial.write(pkstate);  // What the heck?!!
}

bool pk_checksum(int8_t target)
{
  int sum = 0;
  for (int i = 0; i <= pklen; i++) sum += packet[i];
  //Serial.write(sum & 0xff);
  int8_t chk = (-sum) & 0xff;
  return (target == chk);
}

inline void cksum_init()
{
  cksum_accumulator = 0;
}

inline void cksum_update(uint8_t b)
{
  cksum_accumulator += b;
}

inline int8_t cksum_final()
{
  return (-cksum_accumulator) & 0xff;
}

void pk_send(uint8_t dest, uint8_t id, uint8_t byte0, uint8_t byte1 = -1, uint8_t byte2 = -1) {
  sbi(*_ucsrb, TXEN0); // enable the serial transmitter  -- should probably do this in a non-interruptable atomic way, but hopefully it won't matter.
  pinMode(DROPPIN, OUTPUT);  // set drop pin low
  digitalWrite(DROPPIN, 0);  // Doesn't seem to remember bit setting from earlier...

  cksum_init();
  
  // Send preamble
  Serial.write(0x3b);
  
  // Send length
  uint8_t length = 4 + (byte1 != -1) + (byte2 != -1);
  cksum_update(length);
  Serial.write(length);

  // Send src
  cksum_update(DEV_GPS);
  Serial.write((uint8_t)DEV_GPS);

  // Send dest
  cksum_update(dest);
  Serial.write(dest);

  // Send message id
  cksum_update(id);
  Serial.write(id);

  // Send data bytes
  cksum_update(byte0);
  Serial.write(byte0);

  if (byte1 != -1) {
    cksum_update(byte1);
    Serial.write(byte1);
  }

  // Send byte2 if it exists
  if (byte2 != -1) {
    cksum_update(byte2);
    Serial.write(byte2);
  }

  // Send checksum
  Serial.write(cksum_final());

  Serial.flush();  // shouldn't return until AFTER the last byte is actually shifted out, not just out of the buffer
  cbi(*_ucsrb, TXEN0); // disable the serial transmitter  -- hopefully goes back to being an input
  pinMode(TXPIN, TX_INPUT);    // but it may not be, so here...
  pinMode(DROPPIN, DROP_INPUT);  // release drop pin since we're done transmitting
}

// // Send a 1-byte response
// void pk_send(uint8_t dest, uint8_t id, uint8_t byte0)
// {
//   sbi(*_ucsrb, TXEN0); // enable the serial transmitter  -- should probably do this in a non-interruptable atomic way, but hopefully it won't matter.
//   pinMode(DROPPIN, OUTPUT);  // set drop pin low
//   digitalWrite(DROPPIN, 0);  // Doesn't seem to remember bit setting from earlier...

//   cksum_init();
//   // Send preamble
//   Serial.write(0x3b);
//   // Send length 4
//   cksum_update(0x04);
//   Serial.write(0x04);
//   // Send src
//   cksum_update(DEV_GPS);
//   Serial.write((uint8_t)DEV_GPS);
//   // Send dest
//   cksum_update(dest);
//   Serial.write(dest);
//   // Send message id
//   cksum_update(id);
//   Serial.write(id);
//   // Send byte0
//   cksum_update(byte0);
//   Serial.write(byte0);
//   // Send checksum
//   Serial.write(cksum_final());

//   Serial.flush();  // shouldn't return until AFTER the last byte is actually shifted out, not just out of the buffer
//   cbi(*_ucsrb, TXEN0); // disable the serial transmitter  -- hopefully goes back to being an input
//   pinMode(TXPIN, TX_INPUT);    // but it may not be, so here...
//   pinMode(DROPPIN, DROP_INPUT);  // release drop pin since we're done transmitting
  
// }

// // Send a 2-byte response
// void pk_send(uint8_t dest, uint8_t id, uint8_t byte0, uint8_t byte1)
// {
//   sbi(*_ucsrb, TXEN0); // enable the serial transmitter  -- should probably do this in a non-interruptable atomic way, but hopefully it won't matter.
//   pinMode(DROPPIN, OUTPUT);  // set drop pin low
//   digitalWrite(DROPPIN, 0);  // Doesn't seem to remember bit setting from earlier...

//   cksum_init();
//   // Send preamble
//   Serial.write(0x3b);
//   // Send length 5
//   cksum_update(0x05);
//   Serial.write(0x05);
//   // Send src
//   cksum_update(DEV_GPS);
//   Serial.write((uint8_t)DEV_GPS);
//   // Send dest
//   cksum_update(dest);
//   Serial.write(dest);
//   // Send message id
//   cksum_update(id);
//   Serial.write(id);
//   // Send byte0
//   cksum_update(byte0);
//   Serial.write(byte0);
//   // Send byte1
//   cksum_update(byte1);
//   Serial.write(byte1);
//   // Send checksum
//   Serial.write(cksum_final());

//   Serial.flush();  // shouldn't return until AFTER the last byte is actually shifted out, not just out of the buffer
//   cbi(*_ucsrb, TXEN0); // disable the serial transmitter  -- hopefully goes back to being an input
//   pinMode(TXPIN, TX_INPUT);    // but it may not be, so here...
//   pinMode(DROPPIN, DROP_INPUT);  // release drop pin since we're done transmitting
// }

// // Send a 3-byte response
// void pk_send(uint8_t dest, uint8_t id, uint8_t byte0, uint8_t byte1, uint8_t byte2)
// {
//   sbi(*_ucsrb, TXEN0); // enable the serial transmitter  -- should probably do this in a non-interruptable atomic way, but hopefully it won't matter.
//   pinMode(DROPPIN, OUTPUT);  // set drop pin low
//   digitalWrite(DROPPIN, 0);  // Doesn't seem to remember bit setting from earlier...

//   cksum_init();
//   // Send preamble
//   Serial.write(0x3b);
//   // Send length 6
//   cksum_update(0x06);
//   Serial.write(0x06);
//   // Send src
//   cksum_update(DEV_GPS);
//   Serial.write((uint8_t)DEV_GPS);
//   // Send dest
//   cksum_update(dest);
//   Serial.write(dest);
//   // Send message id
//   cksum_update(id);
//   Serial.write(id);
//   // Send byte0
//   cksum_update(byte0);
//   Serial.write(byte0);
//   // Send byte1
//   cksum_update(byte1);
//   Serial.write(byte1);
//   // Send byte2
//   cksum_update(byte2);
//   Serial.write(byte2);
//   // Send checksum
//   Serial.write(cksum_final());

//   Serial.flush();  // shouldn't return until AFTER the last byte is actually shifted out, not just out of the buffer
//   cbi(*_ucsrb, TXEN0); // disable the serial transmitter  -- hopefully goes back to being an input
//   pinMode(TXPIN, TX_INPUT);    // but it may not be, so here...
//   pinMode(DROPPIN, DROP_INPUT);  // release drop pin since we're done transmitting
  
// }
