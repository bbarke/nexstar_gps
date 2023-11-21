#include "scope_temperature.h"

Adafruit_BME280 bme;

bool status = false;
float humidity = 10;
float temp = -40;

int dealy = 1000;
unsigned long lastReadMillis = 1000;
// #define WIRE Wire

void ScopeTemperature::begin() {

    while (!status) {
        status = bme.begin(BME280_ADDRESS_ALTERNATE);
    }
}

uint8_t ScopeTemperature::getHumidity() {
    return humidity;
}

float ScopeTemperature::getTemperature() {
    return cToF(temp);
}

float ScopeTemperature::getDewpoint() {
    float k;
    k = log(humidity/100) + (17.62 * temp) / (243.12 + temp);
    return cToF(243.12 * k / (17.62 - k));
    // return bme.readAltitude(1013.25) * 3.28084;
}


bool ScopeTemperature::update() {
    // Wait a bit before we pull the temperature
    if (status == 0 || millis() <= 2000) {
        return false;
    }

    // Only update the temperature every 5 seconds
    if (millis() > lastReadMillis + 5000) {
        humidity = bme.readHumidity();
        temp = bme.readTemperature();
        lastReadMillis = millis();
        return true;
    }
    return false;
}

float ScopeTemperature::cToF(float c) {
    return (32.0F + (c*1.8));
}