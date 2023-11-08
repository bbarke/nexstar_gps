#ifndef ScopeTemp_H
#define ScopeTemp_H

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

class ScopeTemperature {
public:
    void begin();
    float getHumidity();
    float getTemperature();
    float getDewpoint();
    bool update();

private:
    float cToF(float c);

};

#endif