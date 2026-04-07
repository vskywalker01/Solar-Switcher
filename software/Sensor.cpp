#include "Arduino.h"
#include "Sensor.h"

Sensor::Sensor(int pin, unsigned int base, unsigned int increment) {
    sensorPin=pin;
    c=increment;
    b=base;
}

unsigned int Sensor::getCurrentPower() {
    return b+(digitalRead(sensorPin)*c);
}
