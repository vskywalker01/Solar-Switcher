#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include "Arduino.h"
#include <EEPROM.h>

#define DEFAULT_POWER       0
#define LOADS_NUMBER        4
#define DEFAULT_TIMEROFF    10
#define DEFAULT_TIMERON     10
#define DEFAULT_BUZZER      true 

#define MAX_SETTINGS_BOUND  65536

using namespace std;

typedef struct SettingsValues{
    uint32_t checksum;
    unsigned int powers[LOADS_NUMBER];
    bool masks[LOADS_NUMBER];

    unsigned int defaultTimerOff;
    unsigned int defaultTimerOn;

    bool buzzer;
} SettingsValues;

class Settings {
    private: 
    SettingsValues values;

    public:
    Settings();
    
    void loadDefault();
    bool loadSaved(unsigned int address);
    void store(unsigned int address);

    void setPower(unsigned int load, unsigned int value);
    void setTimerOn(unsigned int value);
    void setTimerOff(unsigned int value);
    void setBuzzer(bool value);
    void setMask(unsigned int load, bool value);
    

    unsigned int getPower(unsigned int load);
    unsigned int getTimerOn();
    unsigned int getTimerOff();
    bool getBuzzer();
    bool getMask(unsigned int load);

};


#endif 
