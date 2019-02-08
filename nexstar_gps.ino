#include <TinyGPS++.h>
#include <SoftwareSerial.h>

#include "define.h"

#define RXPIN 3
#define TXPIN 2

const int LED_COMM_ACTIVE = LED_BUILTIN;   // =13
const int LED_GPS_STATE = 12;

TinyGPSPlus gps;
SoftwareSerial ss(RXPIN, TXPIN);

TinyGPSCustom satellitesInView(gps, "GPGSV", 3);
TinyGPSCustom fixQuality(gps, "GPGGA", 6);  // 0 = invalid, 1 = GPS, 2 = DGPS, etc...
int fixQualityInt;

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

int ledStateGps = LOW;
int ledStateCommActive = LOW;
unsigned long previousMillis = 0;
long blink_interval = 0;

void setup() {
  // put your setup code here, to run once:
  pkstate = PREAMBLE_WAIT;
  pklen = 0;
  pkidx = 0;

  Serial.begin(19200, SERIAL_8N2);
  ss.begin(9600);
  pinMode(LED_GPS_STATE, OUTPUT);
  pinMode(LED_COMM_ACTIVE, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:

  // Feed characters from the GPS module into TinyGPS
  while (ss.available())
    gps.encode(ss.read());

  // Feed characters from the serial port into the packet decoder
  while (Serial.available())
    packet_decode(Serial.read());

  fixQualityInt = getGpsQuality(fixQuality.value());

  // GPS quality LED indication
  if (fixQualityInt > 0) {
    // GPS fixed, LED lights continuously
    digitalWrite(LED_GPS_STATE, true);
  } else {
    if (fixQualityInt == -1) {
      // data from GPS module missing, LED fast blinking
      blink_interval = 50;
    } else {
      // GPS not fixed, LED slow blinking
      blink_interval = 250;
    }
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= blink_interval) {
      // save the last time you blinked the LED
      previousMillis = currentMillis;

      ledChangeStateGps();
    }
  }

  // Check if packet is valid
  if (pkstate != VALID) return;

  // Check that destination is for me
  if (packet[2] != DEV_GPS) {
    pkstate = PREAMBLE_WAIT;
    pkidx = 0;
    pklen = 0;
    return;
  }

  ledChangeStateCommActive();

  // It's for me! What's the command?
  uint8_t dest = packet[1];
  switch (packet[3])
  {
    case GPS_LINKED:
    case GPS_TIME_VALID:
      if (fixQualityInt > 0)
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

  pkstate = PREAMBLE_WAIT;
  pklen = 0;
  pkidx = 0;
}

int getGpsQuality(const char* value) {
  String quality(value);
  if (quality.length() == 0) {
    return -1;
  } else {
    return quality.toInt();
  }
}

void packet_decode(int8_t c)
{
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
}

bool pk_checksum(int8_t target)
{
  int sum = 0;
  for (int i = 0; i <= pklen; i++) sum += packet[i];
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

// Send a 1-byte response
void pk_send(uint8_t dest, uint8_t id, uint8_t byte0)
{
  delayMicroseconds(500);
  cksum_init();
  // Send preamble
  Serial.write(0x3b);
  // Send length 4
  cksum_update(0x04);
  Serial.write(0x04);
  // Send src
  cksum_update(DEV_GPS);
  Serial.write((uint8_t)DEV_GPS);
  // Send dest
  cksum_update(dest);
  Serial.write(dest);
  // Send message id
  cksum_update(id);
  Serial.write(id);
  // Send byte0
  cksum_update(byte0);
  Serial.write(byte0);
  // Send checksum
  Serial.write(cksum_final());
}

// Send a 2-byte response
void pk_send(uint8_t dest, uint8_t id, uint8_t byte0, uint8_t byte1)
{
  delayMicroseconds(500);
  cksum_init();
  // Send preamble
  Serial.write(0x3b);
  // Send length 5
  cksum_update(0x05);
  Serial.write(0x05);
  // Send src
  cksum_update(DEV_GPS);
  Serial.write((uint8_t)DEV_GPS);
  // Send dest
  cksum_update(dest);
  Serial.write(dest);
  // Send message id
  cksum_update(id);
  Serial.write(id);
  // Send byte0
  cksum_update(byte0);
  Serial.write(byte0);
  // Send byte1
  cksum_update(byte1);
  Serial.write(byte1);
  // Send checksum
  Serial.write(cksum_final());
}

// Send a 3-byte response
void pk_send(uint8_t dest, uint8_t id, uint8_t byte0, uint8_t byte1, uint8_t byte2)
{
  delayMicroseconds(500);
  cksum_init();
  // Send preamble
  Serial.write(0x3b);
  // Send length 6
  cksum_update(0x06);
  Serial.write(0x06);
  // Send src
  cksum_update(DEV_GPS);
  Serial.write((uint8_t)DEV_GPS);
  // Send dest
  cksum_update(dest);
  Serial.write(dest);
  // Send message id
  cksum_update(id);
  Serial.write(id);
  // Send byte0
  cksum_update(byte0);
  Serial.write(byte0);
  // Send byte1
  cksum_update(byte1);
  Serial.write(byte1);
  // Send byte2
  cksum_update(byte2);
  Serial.write(byte2);
  // Send checksum
  Serial.write(cksum_final());
}

void ledChangeStateGps() {
  if (ledStateGps == LOW) {
    ledStateGps = HIGH;
  } else {
    ledStateGps = LOW;
  }
  digitalWrite(LED_GPS_STATE, ledStateGps);
}

void ledChangeStateCommActive() {
  if (ledStateCommActive == LOW) {
    ledStateCommActive = HIGH;
  } else {
    ledStateCommActive = LOW;
  }
  digitalWrite(LED_COMM_ACTIVE, ledStateCommActive);
}
