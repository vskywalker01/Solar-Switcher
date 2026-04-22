//Sensor class definition

#ifndef __SENSOR_H__
#define __SENSOR_H__

#include "Arduino.h"
#include <math.h>

#define SAMPLES     50

// The class implements methods for collecting data from the light sensor. It automatically performs an average with past samples in order to remove outlayers. 
// It implements also a function that convers the samples into usable power in Watts (useful in the main program for understanding wich devices should be activated)

class Sensor {
    private:
    //Arduino analog pin used for collecting samples 
    unsigned int sensorPin;                 

    //Values used for the sample fitting 
    float a;                                 
    float b;
    float g;    
    // recorded samples array and pointer 
    unsigned int sensorSamples[SAMPLES]; 
    unsigned int p; 

    public:

    /* Class initialization
     * pin      ->  arduino pin of the sensor 
     * alpha    ->  alpha value used for the fitting function 
     * beta     ->  beta value used for the fitting function 
     */
    Sensor(int pin,float alpha,float beta, float gamma);
    void readSample(); 
    /* getCurrentPower() returns the usable power from the average of samples collected using getCurrentValue(). 
     * It uses a power based on alpha, beta and gamma values estimated empirically by observing how the power distribution is related to differences from measurements in the analog pin. 
     * Power = ((value - g)/a)^b
     * for example: 
     * a -> 366 
     * g -> 0 
     * b -> 9.6 
    */ 
    unsigned int getCurrentPower();

    /* get current value reads a sample from the sensor pin, stores it in the samples array and calculates an average of all samples stored. */ 
    unsigned int getCurrentValue();
    
    /* functions used for setting and getting the coefficients*/ 
    float getAlpha();
    float getBeta();
    float getGamma();

    void setAlpha(float alpha);
    void setBeta(float beta);
    void setGamma(float gamma);
};

#endif 
