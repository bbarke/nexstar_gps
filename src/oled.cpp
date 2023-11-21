#include "oled.h"

char time[40];
char sats[40];
char latlng[40];
char tempHumidity[40];

#define I2C_ADDRESS 0x3C

SSD1306AsciiAvrI2c screen;
bool firstClear = false;

void Oled::begin() {
  screen.begin(&Adafruit128x32, I2C_ADDRESS);
  screen.setFont(Cooper19);
  screen.print("Hello, Brent!");
  screen.setFont(Stang5x7);
}

void Oled::updateTime(TinyGPSPlus gps) {
  memset(time, 0, sizeof(time));

  if (!isLocked(gps)) {
    snprintf(time, sizeof(time), "--/--/-- UTC --:--:--");
    return;
  }

  snprintf(time, sizeof(time), "%02d/%02d/%02d UTC %02d:%02d:%02d",
          gps.date.month(), gps.date.day(), gps.date.year() % 100,
          gps.time.hour(), gps.time.minute(), gps.time.second());
}

void Oled::updateSatellite(TinyGPSPlus gps) {
  memset(sats, 0, sizeof(sats));

  if (isLocked(gps)){
    char altBuff[10];
    char satBuff[5];
    dtostrf(gps.altitude.isValid() ? gps.altitude.feet() : 0, 5, 0, altBuff);
    dtostrf(gps.satellites.isValid() ? gps.satellites.value() : 0, 2, 0, satBuff);
    snprintf(sats, sizeof(sats),
            "Sats: %s Alt: %sft",
            satBuff, 
            altBuff);
  } else {
    snprintf(sats, sizeof(sats),
            "%s",
            "Acquiring satellites... ");
  }
}

void Oled::updateLatLng(TinyGPSPlus gps) {
  memset(latlng, 0, sizeof(latlng));
  
  if (isLocked(gps)) {
    char latbuff[15];
    char lngbuff[15];
                 
    dtostrf(fabs(gps.location.lat()), 5, 5, latbuff);
    dtostrf(fabs(gps.location.lng()), 5, 5, lngbuff);
    sprintf(latlng, "%c%s %c%s", 
                    ((gps.location.lat() < 0.0) ? 'S' : 'N'),
                    latbuff,
                    ((gps.location.lng() < 0.0) ? 'W' : 'E'),
                    lngbuff);
  } else {
    sprintf(latlng, "");
  }
}

void Oled::updateTemperature(ScopeTemperature scopeTemp) {
  char tmpH[10];
  char tmpT[10];
  char tmpD[10];
  dtostrf(scopeTemp.getHumidity(), 2, 0, tmpH);
  dtostrf(scopeTemp.getTemperature(), 3, 0, tmpT);
  dtostrf(scopeTemp.getDewpoint(), 3, 0, tmpD);

  char percent = '%';
  sprintf(tempHumidity, "%sF %s%cH dew%sF", tmpT, tmpH, percent, tmpD);
}

void Oled::writeToScreen() {
  if (millis() < 2000) {
    return;
  }

  if (!firstClear) {
    firstClear = true;
    screen.clear();
  }

  screen.home();
  screen.println(time);
  screen.println(sats);
  screen.println(latlng);
  screen.println(tempHumidity);
}

bool Oled::isLocked(TinyGPSPlus gps) {
  return gps.satellites.isValid() && gps.satellites.value() > 0;
}