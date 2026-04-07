#include "Settings.h"
#include "Arduino.h"

using namespace std;

Settings::Settings() {
    loadDefault();
}

bool Settings::loadSaved(unsigned int address) {  
    EEPROM.get(address,values);
    uint16_t checksum = 0;
    for (unsigned int i=0;i<LOADS_NUMBER;i++) {
        checksum += (uint32_t) values.powers[i];
        checksum += (uint32_t) values.masks[i];
    }
    checksum += (uint32_t) values.defaultTimerOff; 
    checksum += (uint32_t) values.defaultTimerOn; 
    checksum += (uint32_t) values.buzzer;
    if (checksum != values.checksum) {
        loadDefault();
        return false; 
    } 
    return true;
}

void Settings::store(unsigned int address) {
    uint16_t checksum = 0;
    for (unsigned int i=0;i<LOADS_NUMBER;i++) {
        checksum += (uint32_t) values.powers[i];
        checksum += (uint32_t) values.masks[i];
    }
    checksum += (uint32_t) values.defaultTimerOff; 
    checksum += (uint32_t) values.defaultTimerOn; 
    checksum += (uint32_t) values.buzzer;
    values.checksum=checksum;
    EEPROM.put(address,values);
}

void Settings::loadDefault() {
    for (unsigned int n=0;n<LOADS_NUMBER;n++) {
      values.powers[n]=DEFAULT_POWER;
      values.masks[n]=false;
    }
    values.defaultTimerOff=DEFAULT_TIMEROFF;
    values.defaultTimerOn=DEFAULT_TIMERON;
    values.buzzer=DEFAULT_BUZZER;
}

void Settings::setPower(unsigned int load,unsigned int value) {
    if (value<MAX_SETTINGS_BOUND && load<LOADS_NUMBER) {
        values.powers[load]=value;
    }
}

void Settings::setTimerOn(unsigned int value) {
    values.defaultTimerOn=value;
}
void Settings::setTimerOff(unsigned int value) {
    values.defaultTimerOff=value;
}
void Settings::setBuzzer(bool value) {
    values.buzzer=value;
}

unsigned int Settings::getPower(unsigned int load) {
    if (load<LOADS_NUMBER) {
        return values.powers[load];
    }
    return 0;
}
unsigned int Settings::getTimerOn() {
    return values.defaultTimerOn;
}
unsigned int Settings::getTimerOff() {
    return values.defaultTimerOff;
}
bool Settings::getBuzzer() {
    return values.buzzer;
}


void Settings::setMask(unsigned int load,bool value) {
    if (load<LOADS_NUMBER) {
        values.masks[load]=value;
    }
}

bool Settings::getMask(unsigned int load) {
    if (load<LOADS_NUMBER) {
        return values.masks[load];
    }
    return false;
}
