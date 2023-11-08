#include "oled.h"

char time[40] = "Loading";
char sats[40];
char latlng[40];
char tempHumidity[40];


U8G2_SSD1306_128X32_UNIVISION_1_HW_I2C u8g2(/*rotation*/ U8G2_R0);

void Oled::begin() {
    u8g2.begin();
    u8g2.firstPage();
    do {
      u8g2.setFont(u8g2_font_mozart_nbp_tr);
      u8g2.drawStr(0,24,"Hello, Brent!");
    } while ( u8g2.nextPage() );
}

void Oled::updateTime(TinyGPSPlus gps) {
  memset(time, 0, sizeof(time));

  // snprintf(time, sizeof(time),
  //           ((gps.satellites.value() > 0) ? "%02d/%02d/%02d UTC %02d:%02d:%02d"
  //                                         : "--/--/-- UTC --:--:--"),
  //           gps.date.month(), gps.date.day(), gps.date.year() % 100,
  //           gps.time.hour(), gps.time.minute(), gps.time.second());

  if (!isLocked(gps)) {
    snprintf(time, sizeof(time), "--/--/-- UTC --:--:--");
    return;
  }
  
  if (timeStatus() == timeNotSet || gps.time.age() < 1998) {
    setTime(gps.time.hour(), gps.time.minute(), gps.time.second(), gps.date.day(), gps.date.month(), gps.date.year());
    adjustTime(gps.time.age() / 1000);
  }

  uint8_t timestat;
  if (timeStatus() == timeSet) {
    timestat = 1;
  }
  
  if (timeStatus() == timeNotSet) {
    timestat = 2;
  }

  if (timeStatus() == timeNeedsSync) {
    timestat = 3;
  }
  
  memset(time, 0, sizeof(time));
  snprintf(time, sizeof(time), "%02d/%02d/%02d %02d:%02d:%02d %d",
            month(), day(), year() % 100,
            hour(), minute(), second(), timestat);
}

void Oled::updateSatellite(TinyGPSPlus gps) {
  memset(sats, 0, sizeof(sats));
  if (isLocked(gps)){
    char altBuff[10];
    char satBuff[5];
    dtostrf(gps.altitude.isValid() ? gps.time.age() : 0, 5, 0, altBuff);
    dtostrf(gps.satellites.isValid() ? gps.satellites.value() : 0, 2, 0, satBuff);
    snprintf(sats, sizeof(sats),
            "Sats: %s Alt: %s ",
            satBuff, 
            altBuff);
  } else {
    snprintf(sats, sizeof(sats),
            "%s",
            "Acquiring sats... ");
  }
}

void Oled::updateLatLng(TinyGPSPlus gps) {
  memset(latlng, 0, sizeof(latlng));
  
  if (isLocked(gps)) {
    char latbuff[15];
    dtostrf(fabs(gps.location.lng()), 5, 5, latbuff);
    sprintf(latlng, "Lng: %c%s", 
                    ((gps.location.lng() < 0.0) ? 'W' : 'E'),
                    latbuff);
  } else {
    sprintf(latlng, "...");
  }
}

void Oled::updateTemperature(ScopeTemperature scopeTemp) {
  char tmpH[10];
  char tmpT[10];
  char tmpD[10];
  dtostrf(scopeTemp.getHumidity(), 2, 0, tmpH);
  dtostrf(scopeTemp.getTemperature(), 2, 0, tmpT);
  dtostrf(scopeTemp.getDewpoint(), 2, 0, tmpD);

  char degree = '\xb0';
  char percent = '\x25';
  sprintf(tempHumidity, "%s%cF %s%c dew %s%cF", tmpT, degree, tmpH, percent, tmpD, degree);
}

void Oled::writeToScreen() {
  if (millis() < 2000) {
    return;
  }

  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_mozart_nbp_tr);
    u8g2.drawStr(0, 8, time);
    u8g2.drawStr(0, 16, sats);
    u8g2.drawStr(0, 24, latlng);
    u8g2.drawStr(0, 32, tempHumidity);
    // u8g2.drawBox(0, 0, 128, 32);

  } while ( u8g2.nextPage() );
}

bool Oled::isLocked(TinyGPSPlus gps) {
  return gps.satellites.isValid() && gps.satellites.value() > 0;
}