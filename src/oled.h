#ifndef Oled_H
#define Oled_H
#include <TinyGPSPlus.h>
#include <Arduino.h>
#include <U8g2lib.h>


class Oled {
public:
    void begin();
    void updateOled();
    void updateTime(TinyGPSPlus gps);
    void updateSatellite(TinyGPSPlus gps);

    void updateLatLng(TinyGPSPlus gps);

// private:
    void writeToScreen();
    bool isLocked(TinyGPSPlus gps);
};

#endif