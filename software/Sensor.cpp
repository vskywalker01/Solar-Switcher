#include "Arduino.h"
#include "Sensor.h"

Sensor::Sensor(int pin, float alpha, float beta) {
    sensorPin=pin;
    a=alpha;
    b=beta;
    p=0;

    //Initializing sample array with 0
    for (unsigned int i=0;i<SAMPLES;i++) {
        sensorSamples[i]=0;
    }
}

unsigned int Sensor::getCurrentPower() {
    //processing the average 
    unsigned int value=0;
    for (unsigned int i=0;i<SAMPLES;i++) {
        value+=sensorSamples[i];
    }
    //returning the power from the function. 
    return (unsigned int) (a*pow(value,b));
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

unsigned int Sensor::getCurrentValue() {
    //updating the last values 
    sensorSamples[p++]=analogRead(sensorPin);
    p=p%SAMPLES;

    //processing the average
    unsigned int value=0;
    for (unsigned int i=0;i<SAMPLES;i++) {
        value+=sensorSamples[i];
    }
    return value/SAMPLES;
}
