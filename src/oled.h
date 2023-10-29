// MyClass.h
// #ifndef OLED_H
// #define OLED_H
#include <TinyGPSPlus.h>


class Oled {
public:
    Oled();  // Constructor
    void updateOled();
    void updateTime(TinyGPSPlus gps);
    void updateSatellite(TinyGPSPlus gps);

    void updateLatLng(TinyGPSPlus gps);

private:
    void writeToScreen(int row, char* line);
    bool isLocked(TinyGPSPlus gps);
};