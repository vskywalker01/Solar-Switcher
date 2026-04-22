#include "Arduino.h"
#include "Sensor.h"

Sensor::Sensor(int pin, float alpha, float beta, float gamma) {
    sensorPin=pin;
    a=alpha;
    b=beta;
    g=gamma;
    p=0;

    //Initializing sample array with 0
    for (unsigned int i=0;i<SAMPLES;i++) {
        sensorSamples[i]=0;
    }
}


unsigned int Sensor::getCurrentPower() {
    //returning the power from the function. 
    float value = getCurrentValue(); 
    //computing power function using coefficients
    if (value < g) return 0; 
    return (unsigned int) pow((value-g)/a,b);
}

void Sensor::readSample() {
    sensorSamples[p++]=analogRead(sensorPin);
    p=p%SAMPLES;
}

float Sensor::getAlpha() {
    return a; 
}

float Sensor::getBeta() {
    return b;
}

float Sensor::getGamma() {
    return g;
}

void Sensor::setAlpha(float alpha) {
    a=alpha; 
}
void Sensor::setBeta(float beta) {
    b=beta;
}

void Sensor::setGamma(float gamma) {
    g=gamma;
}

unsigned int Sensor::getCurrentValue() {
    //processing the average
    unsigned int value=0;
    for (unsigned int i=0;i<SAMPLES;i++) {
        value+=sensorSamples[i];
    }
    return value/SAMPLES;
}
