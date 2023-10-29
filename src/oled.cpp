#include "oled.h"
#include <TinyGPSPlus.h>
#include <Arduino.h>
#include <U8g2lib.h>


const int UTC_ROW = 7;
const int SAT_ROW = 15;
const int LAT_LNG_ROW = 23;
// Temperature? Humidity? Altitude?



// U8G2_SSD1305_128X32_NONAME_1_SW_I2C(/*rotation*/ U8G2_R0, /*clock*/ D1, /*data*/ D2) [page buffer, size = 128 bytes]
// [full framebuffer, size = 512 bytes]
U8G2_SSD1305_128X32_NONAME_F_SW_I2C u8g2(/*rotation*/ U8G2_R0, /*clock*/ A5, /*data*/ A4);

Oled::Oled() {
  u8g2.begin();
}

void Oled::updateTime(TinyGPSPlus gps)
{
    char line[50];
    snprintf(line, sizeof(line),
             ((gps.satellites.value() > 0) ? "%02d/%02d/%02d UTC %02d:%02d:%02d"
                                           : "--/--/-- UTC --:--:--"),
             gps.date.month(), gps.date.day(), gps.date.year() % 100,
             gps.time.hour(), gps.time.minute(), gps.time.second());
    writeToScreen(UTC_ROW, line);
}

void Oled::updateSatellite(TinyGPSPlus gps) {
  char line[50];
  if (isLocked(gps)){
    snprintf(line, sizeof(line), "   # Sats: %02d", 
              gps.satellites.value());
  } else {
    snprintf(line, sizeof(line), "%s", "Acquiring sats...");
  }
  writeToScreen(SAT_ROW, line);
}

void Oled::updateLatLng(TinyGPSPlus gps) {
  char line[50];
  if (isLocked(gps)){
    snprintf(line, sizeof(line), "Lat: %c%9s Lng: %c%9s", 
        ((gps.location.lat() < 0.0) ? 'S' : 'N'),
        String(fabs(gps.location.lat()), 5).c_str(),
        ((gps.location.lng() < 0.0) ? 'W' : 'E'),
        String(fabs(gps.location.lng()), 5).c_str()
      );
  } else {
    snprintf(line, sizeof(line), "...");
  }
  writeToScreen(LAT_LNG_ROW, line);
}

void Oled::writeToScreen(int row, char* line) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_squeezed_b7_tn);
    u8g2.drawStr(0, row, line);
    u8g2.sendBuffer();
}

bool Oled::isLocked(TinyGPSPlus gps) {
  return gps.satellites.isValid() && gps.satellites.value() > 0;
}