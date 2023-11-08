#ifndef Oled_H
#define Oled_H
#include <TinyGPSPlus.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include "scope_temperature.h"
#include <TimeLib.h>

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