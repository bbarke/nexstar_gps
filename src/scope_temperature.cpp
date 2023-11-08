#include "scope_temperature.h"

Adafruit_BME280 bme;

unsigned status;
float humidity;
float temp;

int dealy = 1000;
unsigned long lastReadMillis = 1000;

void ScopeTemperature::begin() {
    status = bme.begin();
}

float ScopeTemperature::getHumidity() {
    update();
    return humidity;
}

float ScopeTemperature::getTemperature() {
    update();
    return cToF(temp);
}

float ScopeTemperature::getDewpoint() {
    update();
    float k;
     k = log(humidity/100) + (17.62 * temp) / (243.12 + temp);
    return cToF(243.12 * k / (17.62 - k));
}


bool ScopeTemperature::update() {
    if (!status || millis() <= 2000) {
        humidity = 30;
        temp = 0;
        return false;
    }

    if (millis() > lastReadMillis + 2000) {
        humidity = bme.readHumidity();
        temp = bme.readTemperature();
        lastReadMillis = millis();
        return true;
    }
    return false;
}

float ScopeTemperature::cToF(float c) {
    return (c*1.8)+32;
}