#include "oled.h"

const int UTC_ROW = 7;
const int SAT_ROW = 15;
const int LAT_LNG_ROW = 23;
char time[25] = "Loading";
char sats[40];
char latlng[25];


// Temperature? Humidity? Altitude?



// U8G2_SSD1305_128X32_NONAME_1_SW_I2C(/*rotation*/ U8G2_R0, /*clock*/ D1, /*data*/ D2) [page buffer, size = 128 bytes]
// [full framebuffer, size = 512 bytes]
// U8G2_SSD1305_128X32_NONAME_1_HW_I2C u8g2(/*rotation*/ U8G2_R0, /*clock*/ A5, /*data*/ A4);
// U8G2_SSD1306_128X32_UNIVISION_1_HW_I2C u8g2(/*rotation*/ U8G2_R0, /*clock*/ A5, /*data*/ A4);
U8G2_SSD1306_128X32_UNIVISION_1_HW_I2C u8g2(/*rotation*/ U8G2_R0);
// U8X8_SSD1306_128X32_UNIVISION_HW_I2C u8g2

void Oled::begin() {
    u8g2.begin();
    // writeToScreen();
}

void Oled::updateTime(TinyGPSPlus gps) {
  memset(time, 0, sizeof(time));
  snprintf(time, sizeof(time),
            ((gps.satellites.value() > 0) ? "%02d/%02d/%02d UTC %02d:%02d:%02d"
                                          : "--/--/-- UTC --:--:--"),
            gps.date.month(), gps.date.day(), gps.date.year() % 100,
            gps.time.hour(), gps.time.minute(), gps.time.second());
}

void Oled::updateSatellite(TinyGPSPlus gps) {
  memset(sats, 0, sizeof(sats));
  if (isLocked(gps)){
    char altBuff[10];
    char satBuff[4];
    dtostrf(gps.altitude.isValid() ? gps.altitude.feet() : 0, 5, 0, altBuff);
    dtostrf(gps.satellites.isValid() ? gps.satellites.value() : 0, 2, 0, satBuff);
    snprintf(sats, sizeof(sats),
            "Sats: %s Alt: %s ",
            satBuff, 
            altBuff);
    // String tmp = "Sats: " + 500;
    // String tmp = "Sats: "
    // tmp.toCharArray(sats, sizeof(sats));
    
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

void Oled::writeToScreen() {
    // u8g2.clearBuffer();
    // u8g2.setFont(u8g2_font_squeezed_b7_tn);
    // u8g2.drawStr(0, row, "hi");
    // u8g2.sendBuffer();
    int8_t height = u8g2.getDisplayHeight() / 4;

    u8g2.firstPage();
    do {
      u8g2.setFont(u8g2_font_mozart_nbp_tr);
      u8g2.drawStr(0, 8, time);
      u8g2.drawStr(0, 16, sats);
      // u8g2.drawStr(5, 16, String(gps.altitude.isValid() ? gps.altitude.feet() : 100, 1).c_str());
      u8g2.drawStr(0, 24, latlng);

    } while ( u8g2.nextPage() );
}

bool Oled::isLocked(TinyGPSPlus gps) {
  return gps.satellites.isValid() && gps.satellites.value() > 0;
}