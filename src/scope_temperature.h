#ifndef ScopeTemp_H
#define ScopeTemp_H

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

class ScopeTemperature {
public:
    void begin();
    uint8_t getHumidity();
    int getTemperature();
    int getDewpoint();
    bool update();

private:
    int cToF(int c);

};

#endif