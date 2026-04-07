#include "Arduino.h"
#include "Relays.h"

Relays::Relays(unsigned int relays) {
    if (relays==0) return;
    number=relays;
    counters= new unsigned int [number];
    status= new counterStatus [number];
    changedFlags= new bool[number];
    for (unsigned int r=0;r<number;r++) {
        counters[r]=0;
        status[r]=OFF;
        changedFlags[r]=false;
    }
}
Relays::~Relays() {
    if (number>0) {
        delete [] counters;
        delete [] status;
        delete [] changedFlags;
    }
}

void Relays::setCount(unsigned int relay, unsigned int value,counterStatus direction) {
    counters[relay]=value;
    status[relay]=direction;
}

unsigned int Relays::getCount(unsigned int relay) {
    return counters[relay];
}
counterStatus Relays::getDirection(unsigned int relay) {
    return status[relay];
}

void Relays::updateStatus() {
    for (unsigned int r=0;r<number;r++) {
        switch (status[r]) {
            case START:
                if (counters[r]>0) {
                    counters[r]--;
                } else {
                    status[r]=ON;
                    changedFlags[r]=true;
                }
            break;
            case STOP:
                if (counters[r]>0) {
                    counters[r]--;
                } else {
                    status[r]=OFF;
                    changedFlags[r]=true;
                }
            break;
        }
    }
}

bool Relays::getChanged(unsigned int relay) {
    return changedFlags[relay];
}
void Relays::clearFlag(unsigned int relay) {
    changedFlags[relay]=false;
}
