#include "Arduino.h"
#include "Sensor.h"

Sensor::Sensor(int pin, float alpha, float beta) {
    sensorPin=pin;
    a=alpha;
    b=beta;
}

unsigned int Sensor::getCurrentPower() {
    return (unsigned int) (a*pow(analogRead(sensorPin),b));
}

float Sensor::getAlpha() {
    return a; 
}

float Sensor::getBeta() {
    return b;
} 
void Sensor::setAlpha(float alpha) {
    a=alpha; 
}
void Sensor::setBeta(float beta) {
    b=beta;
}
