#ifndef Oled_H
#define Oled_H
#include <TinyGPSPlus.h>
#include <Arduino.h>
#include <SPI.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiAvrI2c.h"
#include "scope_temperature.h"

class Oled {
public:
    void begin();
    void updateOled();
    void updateTime(TinyGPSPlus gps);
    void updateSatellite(TinyGPSPlus gps);
    void updateLatLng(TinyGPSPlus gps);
    void updateTemperature(ScopeTemperature scopeTemp);

// private:
    void writeToScreen();
    bool isLocked(TinyGPSPlus gps);
};

#endif