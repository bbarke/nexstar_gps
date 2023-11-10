#include "scope_temperature.h"

Adafruit_BME280 bme;

bool status = false;
uint8_t humidity;
int temp;

int dealy = 1000;
unsigned long lastReadMillis = 1000;
// #define WIRE Wire

void ScopeTemperature::begin() {

    while (!status) {
        status = bme.begin(BME280_ADDRESS_ALTERNATE);
        Serial.println(status);
    }
}

uint8_t ScopeTemperature::getHumidity() {
    update();
    return humidity;
}

int ScopeTemperature::getTemperature() {
    update();
    return cToF(temp);
}

int ScopeTemperature::getDewpoint() {
    update();
    // float k;
    // k = log(humidity/100) + (17.62 * temp) / (243.12 + temp);
    // return cToF(243.12 * k / (17.62 - k));
    return 1;
}


bool ScopeTemperature::update() {
    if (status == 0 || millis() <= 2000) {
        Serial.print("Bad temp status ");
        Serial.println(status);
        Serial.println(millis());
        humidity = 1;
        temp = -2;
        return false;
    }

    if (millis() > lastReadMillis + 2000) {
        Serial.println("pulled details");
        // humidity = bme.readHumidity();
        temp = bme.getTemperatureSensor();
        temp = 5;
        lastReadMillis = millis();
        return true;
    }
    Serial.println("Not ready");
    return false;
}

int ScopeTemperature::cToF(int c) {
    // return (c*1.8)+32;
    return c;
}